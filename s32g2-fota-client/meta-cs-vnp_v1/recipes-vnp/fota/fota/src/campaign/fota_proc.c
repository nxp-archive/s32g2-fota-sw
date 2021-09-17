/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ftw.h>
#include <mqueue.h>
#include <fcntl.h>
#include <errno.h>
#include "fotav.h"

#include "../init/fota_config.h"
#include "../uds/fota_uds.h"
#include "../repo/repo.h"
#include "fota_conn.h"
#include "campaign.h"
#if (LCD_ENABLE == ON)
#include "../touch_panel/cmd.h"
#endif


extern error_t get_ecu_updating_status();

error_t proccess_ecu()
{
    fota_image_t *fw;
    fota_ecu_t *ecu;

    for (ecu = get_ecu_list(); ecu != NULL; ecu = ecu->next) {
        fw = ecu->updates;
        if (fw != NULL) {
            
            if (ecu->campaign > ECU_CAMPAIGN_PROC_CMD)
                continue;
            else if (ecu->campaign < ECU_CAMPAIGN_PROC_CMD)
                break;

            if (ecu->fota_state != SECU_STATUS_TRANSFERED) {
                printf_it(LOG_ERROR, "Remote ECU %s status is not TRANSFERED: %d", 
                    ecu->hw_id, ecu->fota_state);
                break;
            }

            printf_it(LOG_TRACE, "send process command to ECU %s", ecu->hw_id);
            
            if (fota_conn_cmd_process(ecu) != ERR_OK) {
                printf_it(LOG_ERROR, "send process command to ecu %s failed", ecu->hw_id);
                break;
            }
            
            fota_update_ecu_campaign(ecu, ECU_CAMPAIGN_PROC_POLL, 0);
            update_campaign_error(ecu, CAMPAIGN_ERR_NONE);
        }
    }

    if (ecu != NULL) {
        update_campaign_error(ecu, CAMPAIGN_ERR_PROC);
        return ERR_NOK;
    }

    return ERR_OK;
}

error_t wait_process_done(int32_t *timeout)
{
    fota_image_t *fw;
    fota_ecu_t *ecu;
    int32_t retry;
    int32_t ecu_num, done_num;
    uint8_t status, progress;
    int8_t display_buf[64];

    retry = 10;
    while (retry > 0) {
        ecu_num = 0;
        done_num = 0;

        for (ecu = get_ecu_list(); ecu != NULL; ecu = ecu->next) {
            fw = ecu->updates;
            if (fw != NULL) {
            
                if (ecu->campaign > ECU_CAMPAIGN_PROC_POLL)
                    continue;
                else if (ecu->campaign < ECU_CAMPAIGN_PROC_POLL)
                    break;

                printf_it(LOG_TRACE, "poll process progress of ecu %s", ecu->hw_id);
                
                if (fota_conn_cmd_get_status(ecu, &status, &progress) != ERR_OK) {
                    printf_it(LOG_ERROR, "check state of ecu %s failed", ecu->hw_id);
                    break;
                }
                ecu->fota_state = status;

#if (LCD_ENABLE == ON)
                snprintf(display_buf, 64, "Start Flashing ECU %s...", ecu->hw_id);
                setInstalling1file_EScreen(progress, display_buf);
#endif

                ecu_num++;
            
                if (((status == SECU_STATUS_PROCESSING) && (progress == 100))
                    || (status == SECU_STATUS_PROCESSED)) 
                {
                    update_campaign_error(ecu, CAMPAIGN_ERR_NONE);
                    fota_update_ecu_campaign(ecu, ECU_CAMPAIGN_ACTIVE_APPROVE, 0);
                    done_num++;
#if (LCD_ENABLE == ON)
                    snprintf(display_buf, 64, "Flashing ECU %s was done!", ecu->hw_id);
                    setInstalling1file_EScreen(100, display_buf);
#endif

                } else {
                    update_campaign_error(fw, CAMPAIGN_ERR_PROC);
                }
            }
        }

        if ((ecu_num == done_num) && (ecu == NULL)) {
            break;  /* while(...) */
        }
        
        sleep(1);
        retry--;
    };

    if (retry > 0)
        *timeout = 0;
    else 
        *timeout = 1;

    return ERR_OK;
}


void* fota_process_thread(void *arg)
{
    mqd_t mq_campaign_host, mq_proc_host;
    int32_t mq_len;
    int8_t mq_host_buf[64];
	int8_t mq_client_buf[64];
    error_t ret;
    char *pstr;
    int32_t tmp;

    fota_image_t *fw;
    fota_ecu_t *ecu;
    int32_t timeout;

    /* open the queue */
	if ((mq_campaign_host = mq_open("/mq_fota_campaign_host", O_WRONLY)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_fota_campaign_host fail");

    if ((mq_proc_host = mq_open("/mq_fota_proc_host", O_RDONLY)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_fota_sync_host fail");

    printf_it(LOG_TRACE, "FOTA process thread starts");

    while(true) {
        
        /* waiting for msg */
		if ((mq_len = mq_receive(mq_proc_host, mq_host_buf, sizeof(mq_host_buf), NULL)) < 0) {
			printf_it(LOG_ERROR, "fota_campaign_main mq_receive error");
		}

        printf_it(LOG_INFO, "Get message:%s", mq_host_buf);

        if ((pstr = strstr(mq_host_buf, "proc:approve")) != NULL) {
            printf_it(LOG_TRACE, "start to approve process");

//            printf_update_list();
#if (LCD_ENABLE == ON)
            /*
                #1. trigger approval for process via UI
                #2. waiting for approval results from user (approved/timeout/decline)
            */
            setInstalling1file_EScreen(0, "Start Flashing...");
            sleep(1);
            update_ecu_campaign_list(ECU_CAMPAIGN_PROC_CMD);
#elif (INTEGRATION_TEST == ON)
            printf_it(LOG_TRACE, "process is approved");
            update_ecu_campaign_list(ECU_CAMPAIGN_PROC_CMD);
#else
            printf_it(LOG_INFO, "\n Waiting for installation. Please enter 'y' to continue...");
            while(1) {
                if ('y' == getchar())
                    break;
            }

            printf_it(LOG_TRACE, "process is approved");
            update_ecu_campaign_list(ECU_CAMPAIGN_PROC_CMD);
#endif
            snprintf(mq_client_buf, sizeof(mq_client_buf), 
                "%s", "procApprove:done");
                    
            if (mq_send(mq_campaign_host, mq_client_buf, sizeof(mq_client_buf), 0) < 0) {
                printf_it(LOG_ERROR, "mq_send error");
                continue;
            }
        }
        else if ((pstr = strstr(mq_host_buf, "proc:process")) != NULL) {
            printf_it(LOG_TRACE, "start to process");

            /* 
                #0. check ecu status
            */
            if (get_ecu_updating_status() != ERR_OK) {
                snprintf(mq_client_buf, sizeof(mq_client_buf), 
                    "%s", "proc:error.proc");
                goto send_response_;
            }

            print_ecu_campaign();

#if (LCD_ENABLE == ON)
            updateAllScreenString(NULL, "Installing");
            setLogText(installing1file_e_screen_id, "Send install command...");
            sleep(1);
#endif
            /*
                #1. send process command to ECUs
            */
            if (proccess_ecu() != ERR_OK) {
                snprintf(mq_client_buf, sizeof(mq_client_buf), 
                    "%s", "proc:error.proc");
                goto send_response_;
            }

            /*
                #2. polling process status progress %
            */
            wait_process_done(&timeout);

            if (timeout == 1) {
                snprintf(mq_client_buf, sizeof(mq_client_buf), 
                    "%s", "proc:error.timeout");
                goto send_response_;
            }

            
            snprintf(mq_client_buf, sizeof(mq_client_buf), 
                "%s", "proc:done");

send_response_:   
            if (mq_send(mq_campaign_host, mq_client_buf, sizeof(mq_client_buf), 0) < 0) {
                printf_it(LOG_ERROR, "mq_send error");
                continue;
            }
        }
    }

    err_return:
    
        return;
}

