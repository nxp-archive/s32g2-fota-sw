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
#include "../uds/fota_uds.h"
#include "fota_conn.h"
#include "campaign.h"

#if (LCD_ENABLE == ON)
#include "../touch_panel/cmd.h"
#endif


error_t activate_ecus()
{
    fota_image_t *fw;
    fota_ecu_t *ecu;
    fota_ecu_t *primary = NULL;

    for (ecu = get_ecu_list(); ecu != NULL; ecu = ecu->next) {
        fw = ecu->updates;
        if (fw != NULL) {
            if (ecu->campaign > ECU_CAMPAIGN_ACTIVE_CMD)
                continue;
            else if (ecu->campaign < ECU_CAMPAIGN_ACTIVE_CMD)
                break;

            if (ecu->fota_state != SECU_STATUS_PROCESSED) {
                printf_it(LOG_ERROR, "Remote ECU %s status is not PROCESSED: %d", 
                    ecu->hw_id, ecu->fota_state);
                break;
            }

            if (ecu->is_primary == 1) {
                primary = ecu;
                continue;
            }

            printf_it(LOG_TRACE, "activate secu %s", ecu->hw_id);
            /*
            todo: it is assumed that the ECU geting the activation command will not precess if 
              there is nothing to be activated!
            */
            if (fota_conn_cmd_activate(ecu) != ERR_OK) {
                 printf_it(LOG_ERROR, "send activation command to ecu %s failed", ecu->hw_id);
                 break;
            }

            update_campaign_error(ecu, CAMPAIGN_ERR_NONE);
            fota_update_ecu_campaign(ecu, ECU_CAMPAIGN_ACTIVE_POLL, 0);
            
        }
    }

     if (ecu != NULL) {
        update_campaign_error(ecu, CAMPAIGN_ERR_ACTIVE);
        return ERR_NOK;
     }

     if (primary != NULL) {
        printf_it(LOG_TRACE, "activate pecu %s", primary->hw_id);
        
        if (fota_conn_cmd_activate(primary) != ERR_OK) {
            printf_it(LOG_ERROR, "send activation command to ecu %s failed", primary->hw_id);
            update_campaign_error(primary, CAMPAIGN_ERR_ACTIVE);
            return ERR_NOK;
        }
        update_campaign_error(primary, CAMPAIGN_ERR_NONE);
        fota_update_ecu_campaign(primary, ECU_CAMPAIGN_ACTIVE_POLL, 0);
     }

     return ERR_OK;
}

error_t waiting_for_activated(int32_t *timeout)
{
    fota_image_t *fw;
    fota_ecu_t *ecu;
    uint8_t progress, status;
    int32_t retry = 10;
    int32_t unfinished_num;

    while (retry > 0) {
        unfinished_num = 0;

        for (ecu = get_ecu_list(); ecu != NULL; ecu = ecu->next) {

            fw = ecu->updates;
            if (fw != NULL) {
                
                if (ecu->campaign > ECU_CAMPAIGN_ACTIVE_POLL) 
                    continue;
                else if (ecu->campaign < ECU_CAMPAIGN_ACTIVE_POLL) 
                    break;
                
                if (fota_conn_cmd_get_status(ecu, &status, &progress) != ERR_OK) {
                     printf_it(LOG_ERROR, "get status of ecu %s failed", ecu->hw_id);
                     break;
                }
    
                ecu->fota_state = status;
                
                if (status == SECU_STATUS_ACTIVATED) 
                {
                    update_campaign_error(ecu, CAMPAIGN_ERR_NONE);
                    fota_update_ecu_campaign(ecu, ECU_CAMPAIGN_CHECK, 0);
                } else {
                    unfinished_num++;
                    update_campaign_error(ecu, CAMPAIGN_ERR_ACTIVE);
                }

            
            }
       }        

        if ((unfinished_num == 0) && (ecu == NULL)) {
            break;
        }

        retry--;
        sleep(1);
    }

    if (retry > 0)
        *timeout = 0;
    else
        *timeout = 1;

    return ERR_OK;
}

void* fota_activate_thread(void *arg)
{
    mqd_t mq_campaign_host, mq_acti_host;
    int32_t mq_len;
    int8_t mq_host_buf[64];
	int8_t mq_client_buf[64];
    error_t ret;
    char *pstr;
    int32_t tmp;

    int32_t status;
    int32_t timeout;

    /* open the queue */
	if ((mq_campaign_host = mq_open("/mq_fota_campaign_host", O_WRONLY)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_fota_campaign_host fail");

    if ((mq_acti_host = mq_open("/mq_fota_acti_host", O_RDONLY)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_fota_acti_host fail");

    printf_it(LOG_TRACE, "FOTA activation thread starts");

    while(true) {
        
        /* waiting for msg */
		if ((mq_len = mq_receive(mq_acti_host, mq_host_buf, sizeof(mq_host_buf), NULL)) < 0) {
			printf_it(LOG_ERROR, "fota_campaign_main mq_receive error");
		}

        printf_it(LOG_INFO, "Get message:%s", mq_host_buf);

        if ((pstr = strstr(mq_host_buf, "acti:approve")) != NULL) {
            printf_it(LOG_TRACE, "start to approve activation");

//            printf_update_list();
#if (LCD_ENABLE == ON)
            /*
                #1. trigger approval for process via UI
                #2. waiting for approval results from user (approved/timeout/decline)
            */
            updateAllScreenString(NULL, "Pend Activiation");
            switchScreen(activate_e_screen_id);
            setLogText(activate_e_screen_id, "Waiting for user approval...");
            if (setActivate_EScreen()) {
                update_ecu_campaign_list(ECU_CAMPAIGN_ACTIVE_CMD);
            } else {
                switchScreen(fotademo_e_screen_id);

                snprintf(mq_client_buf, sizeof(mq_client_buf), 
                    "%s", "actiApprove:cancel");
                goto send_response_;
            }
            
#elif (INTEGRATION_TEST == ON)
            update_ecu_campaign_list(ECU_CAMPAIGN_ACTIVE_CMD);
#else
            printf_it(LOG_INFO, "Waiting for activation approval. Please enter 'y' to continue...");
            while(1) {
                if ('y' == getchar())
                    break;
            }

            update_ecu_campaign_list(ECU_CAMPAIGN_ACTIVE_CMD);
#endif

            snprintf(mq_client_buf, sizeof(mq_client_buf), 
                "%s", "actiApprove:done");
                    
            if (mq_send(mq_campaign_host, mq_client_buf, sizeof(mq_client_buf), 0) < 0) {
                printf_it(LOG_ERROR, "mq_send error");
                continue;
            }
        }
        else if ((pstr = strstr(mq_host_buf, "acti:activate")) != NULL) {
            printf_it(LOG_TRACE, "start to activate");
            /*
                #0. check ecu status
            */
            if (get_ecu_updating_status() != ERR_OK) {
                snprintf(mq_client_buf, sizeof(mq_client_buf), 
                    "%s", "acti:error.activate");
                goto send_response_;
            }

            print_ecu_campaign();

#if (LCD_ENABLE == ON)
            updateAllScreenString(NULL, "Activating");
            setLogText(activate_e_screen_id, "Send activation command");
            sleep(1);
#endif
            /*
                #1. send activate command to ECUs
             */
             printf_it(LOG_TRACE, "send activation command to ECU");
             if (activate_ecus() != ERR_OK) {
                 snprintf(mq_client_buf, sizeof(mq_client_buf), 
                     "%s", "acti:error.activate");
                 goto send_response_;
             }

#if (LCD_ENABLE == ON)
            setLogText(activate_e_screen_id, "Wait for ECU to be activated");
#endif

             /*
                #2. waiting for all normal updating ECUs are activated
             */
             printf_it(LOG_TRACE, "wait for ECU to be activated");
             waiting_for_activated(&timeout);

             if (timeout > 0) {
                printf_it(LOG_ERROR, "activation is timeout");
                
#if (LCD_ENABLE == ON)
                setLogText(activate_e_screen_id, "ECU activation timeout!");
#endif

             
                snprintf(mq_client_buf, sizeof(mq_client_buf), 
                     "%s", "acti:error.timeout");
                 goto send_response_;
             }
             
#if (LCD_ENABLE == ON)
            setLogText(activate_e_screen_id, "ECU activation done");
            sleep(1);
#endif

            printf_it(LOG_ERROR, "activation is finished");
            snprintf(mq_client_buf, sizeof(mq_client_buf), 
                "%s", "acti:done");

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

