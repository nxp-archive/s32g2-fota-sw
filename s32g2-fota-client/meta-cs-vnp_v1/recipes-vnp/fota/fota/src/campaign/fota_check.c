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
extern void update_display_demo(void);
#endif


error_t get_ecu_updating_status()
{
    fota_image_t *fw;
    fota_ecu_t *ecu;
    uint8_t progress, status;

    for (ecu = get_ecu_list(); ecu != NULL; ecu = ecu->next) {
        fw = ecu->updates;
        if (fw != NULL) {
            if (fota_conn_cmd_get_status(ecu, &status, &progress) == ERR_OK) {
                ecu->fota_state = status;
            
            } else {
                printf_it(LOG_ERROR, "get status of ecu %s failed", ecu->hw_id);
            }
            
        }
    }

    if (ecu != NULL)
        return ERR_NOK;

    return ERR_OK;
}

void check_installation(int32_t *rollback)
{
    fota_image_t *fw;
    fota_ecu_t *ecu;
    int32_t consistent = 0;
    int32_t cancel = 0;
    campaign_err error_campaign;

    for (ecu = get_ecu_list(); ecu!= NULL; ecu = ecu->next) {
        fw = ecu->updates;
        if (fw != NULL) {
            if (ecu->err_state != CAMPAIGN_ERR_NONE) {
                /* todo: check dependency */
                cancel = 1;

                error_campaign = ecu->err_state;
            }

            if (ecu->campaign == ECU_CAMPAIGN_CHECK) {
                check_manifest_against_metadata(ecu, &consistent);

                if (consistent == 0) {
                    ecu->err_state = CAMPAIGN_ERR_ACTIVE;
                    error_campaign = ecu->err_state;
                    cancel = 1;
                } else {
                    ecu->err_state = CAMPAIGN_ERR_NONE;
                }
            }
        }
    }

    *rollback = cancel;

    /* error propagation: if there is one ECU failed to install update, cancel all */
    if (cancel > 0) {
        for (ecu = get_ecu_list(); ecu != NULL; ecu = ecu->next) {
            if (ecu->updates != NULL)
                ecu->err_state = error_campaign;
        }
    }
}

error_t finish_installation(fota_image_t *fw)
{
    fota_ecu_t *ecu;

    ecu = (fota_ecu_t *)fw->ecu_target;

    return fota_conn_cmd_finish(ecu);
}

error_t cancel_installation(fota_image_t *fw)
{
    fota_ecu_t *ecu;

    ecu = (fota_ecu_t *)fw->ecu_target;

    return uds_cmd_cancel(ecu->addr, ecu->addr_ext);
}

error_t rollback_installation(fota_image_t *fw)
{
    fota_ecu_t *ecu;

    ecu = (fota_ecu_t *)fw->ecu_target;

    return uds_cmd_rollback(ecu->addr, ecu->addr_ext);
}

error_t waiting_campaign_idle()
{
    fota_image_t *fw;
    fota_ecu_t *ecu;
    uint8_t progress, status;
    int32_t ecu_num = 0, done_num = 0;
    int32_t retry = 10;

    while(retry > 0) {
        for (ecu = get_ecu_list(); ecu!= NULL; ecu = ecu->next) {
            fw = ecu->updates;
            if (fw != NULL) {

                if (ecu->campaign != ECU_CAMPAIGN_CHECK)
                    continue;

                if (fota_conn_cmd_get_status(ecu, &status, &progress) != ERR_OK) {
                     printf_it(LOG_ERROR, "get activation status of ecu %s failed", ecu->hw_id);
                     break;
                }

                ecu_num++;

                ecu->fota_state = status;

                if (status == SECU_STATUS_IDLE)
                {
                    printf_dbg(LOG_INFO, "ECU %s status changes to IDLE", ecu->hw_id);
                    
                    ecu->err_state = CAMPAIGN_ERR_NONE;
                    fota_update_ecu_campaign(ecu, ECU_CAMPAIGN_IDLE, 0);
                    done_num++;
                }
            }
        }
        
        if ((ecu_num == done_num) && (ecu == NULL)) {
            break;
        }

        if (retry == 0) 
            break;

        retry--;
        sleep(1);
    }

    if (retry > 0)
        return ERR_OK;
    else
        return ERR_NOK;

}


/* bring ECU campaign state to IDLE */
error_t finish_ecus()
{
    fota_image_t *fw;
    fota_ecu_t *ecu;
    error_t ret;

    for (ecu = get_ecu_list(); ecu!= NULL; ecu = ecu->next) {
        fw = ecu->updates;
        if (fw != NULL) {

            if (ecu->err_state == CAMPAIGN_ERR_NONE) {

                if (ecu->campaign != ECU_CAMPAIGN_CHECK)
                    continue;
                    
                ret = finish_installation(fw);
                printf_it(LOG_INFO, "ECU %s finished update successfully", ecu->hw_id);

            } else if ((ecu->err_state == CAMPAIGN_ERR_TRANS) || 
                        (ecu->err_state == CAMPAIGN_ERR_PROC)) {

                if (ecu->campaign < ECU_CAMPAIGN_TRANS) {
                    fota_update_ecu_campaign(ecu, ECU_CAMPAIGN_CHECK, 0);
                    continue;
                }
                        
                printf_it(LOG_ERROR, "ECU %s failed to install update: %d, canceling", ecu->hw_id, (int32_t)ecu->err_state);
                ret = cancel_installation(fw);  /* TODO: not supported */

            } else if (ecu->err_state == CAMPAIGN_ERR_ACTIVE) {

                if (ecu->campaign < ECU_CAMPAIGN_ACTIVE_CMD) {
                    fota_update_ecu_campaign(ecu, ECU_CAMPAIGN_CHECK, 0);
                    continue;
                }
                
                printf_it(LOG_ERROR, "ECU %s failed to install update: %d, rollbacking", ecu->hw_id, (int32_t)ecu->err_state);

                ret = rollback_installation(fw);    /* TODO: not supported */
            }

            if (ret == ERR_OK) 
                fota_update_ecu_campaign(ecu, ECU_CAMPAIGN_CHECK, 0);
            else
                break;
        }
    }

    if (ecu != NULL)
        return ERR_NOK;

    return waiting_campaign_idle();
}


void* fota_check_thread(void *arg)
{
    mqd_t mq_campaign_host, mq_chek_host;
    int32_t mq_len;
    int8_t mq_host_buf[64];
	int8_t mq_client_buf[64];
    error_t ret;
    char *pstr;
    int32_t tmp;

    int32_t finished = 0;
    int32_t timeout = 0;
    int32_t need_rollback = 0;
    int32_t need_reset = 0;

    /* open the queue */
	if ((mq_campaign_host = mq_open("/mq_fota_campaign_host", O_WRONLY)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_fota_campaign_host fail");

    if ((mq_chek_host = mq_open("/mq_fota_chek_host", O_RDONLY)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_fota_sync_host fail");

    printf_it(LOG_TRACE, "FOTA check thread starts");

    while(true) {
        
        /* waiting for msg */
		if ((mq_len = mq_receive(mq_chek_host, mq_host_buf, sizeof(mq_host_buf), NULL)) < 0) {
			printf_it(LOG_ERROR, "fota_campaign_main mq_receive error");
		}

        printf_it(LOG_INFO, "Get message:%s", mq_host_buf);

        if ((pstr = strstr(mq_host_buf, "check:check")) != NULL) {
            printf_it(LOG_TRACE, "start to check");

#if (LCD_ENABLE == ON)
            updateAllScreenString(NULL, "Checking");
            sleep(1);
#endif
            /*
                all ECUs are in Activated state?
                send finish command
            */
            /*
                #1. request ECU manifest and verify it is the expected version
                #2. perform *finish* operation for normal ECUs
                #3. perform *cancel* operation for ECUs (campaign state before ACTIVATED) with error
                #4. perform *roolback* operation for ECUs (campaign state ACTIVATED) with error
            */
#if (LCD_ENABLE == ON)
            setLogText(activate_e_screen_id, "Perform checking... Sync ECU manifest");
            sleep(1);
#endif

            if (get_ecu_updating_status() != ERR_OK) {
                snprintf(mq_client_buf, sizeof(mq_client_buf), 
                    "%s", "check:error.retry");
                goto send_response_;
            }

            print_ecu_campaign();
        
            printf_it(LOG_TRACE, "sync vehicle manifest data");
            if (sync_vehicle_manifest() != ERR_OK) {
                printf_it(LOG_ERROR, "sync vehicle manifest error");
                snprintf(mq_client_buf, sizeof(mq_client_buf), 
                    "%s", "check:error.retry");
                goto send_response_;
            }

            check_installation(&need_rollback);

            print_ecu_campaign();

#if (LCD_ENABLE == ON)
            setLogText(activate_e_screen_id, "Check is done! Finish updating...");
            sleep(1);
#endif

            /* 
                #1. check and perform finish
            */
            printf_it(LOG_TRACE, "start to finish installation");
            if (finish_ecus() != ERR_OK) {
                snprintf(mq_client_buf, sizeof(mq_client_buf), 
                    "%s", "check:error.finish");
                goto send_response_;
            }

#if (LCD_ENABLE == ON)
            if (need_rollback > 0) {
                updateAllScreenString(NULL, "Rollback");
                switchScreen(failed_e_screen_id);
            } else {
                updateAllScreenString(NULL, "Done");
                switchScreen(successful_e_screen_id);
            }
#endif

            if (is_updates_available() != 0) {
#if (UPDATE_TEST == OFF)
                repo_mv_current_2_previous();
                repo_mv_verified_2_current();
                repo_targets_current_2_previous();
                repo_targets_verified_2_current();
#endif                
                parse_metadata_init();
                update_ecu_fw_info();
                build_vehicle_manifest(get_ecu_list());

                need_reset = need_reset_after_update();
            }
            
            printf_it(LOG_TRACE, "finish is done, clean updates_fw_list");

            empty_update_ecu_list();

            print_ecu_campaign();

            update_display_demo();
#if (LCD_ENABLE == ON)
            sleep(3);
#endif

#if (PRIMARY_ECU_RESET == ON)
            if (need_reset > 0) {
                switchScreen(reseting_e_screen_id);
                pl_system("reboot");
                sleep(60);
            }
#endif

#if (INTEGRATION_TEST == ON)
#if (UPDATE_TEST == OFF)
            exit(EXIT_SUCCESS);
#endif
#endif

            snprintf(mq_client_buf, sizeof(mq_client_buf), 
                "%s", "check:done");
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

