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

error_t download_targets()
{
    fota_image_t *fw_item;
    fota_ecu_t *ecu;
    error_t ret;

    for (ecu = get_ecu_list(); ecu != NULL; ecu = ecu->next) {
        
        for (fw_item = ecu->updates; fw_item != NULL; fw_item = fw_item->next) {
            
            if (ecu->campaign > ECU_CAMPAIGN_TRANS_DL) 
                continue;
            else if (ecu->campaign < ECU_CAMPAIGN_TRANS_DL) 
                break;
    
            ret = fota_conn_dl(REPO_DIRECT, REPO_TARGETS_DR REPO_DL_RELATIVE, fw_item->fname);
            if (ret != ERR_OK) {
                break;
            }

            ret = fota_conn_dl(REPO_IMAGE, REPO_TARGETS_IR REPO_DL_RELATIVE, fw_item->fname);
            if (ret != ERR_OK)
                break;
        } 
        
        if ((ecu->updates != NULL) && (fw_item == NULL)) {
            fota_update_ecu_campaign(ecu, ECU_CAMPAIGN_TRANS_VERIFY, 0);
            update_campaign_error(fw_item, CAMPAIGN_ERR_NONE);
        }

        if (fw_item != NULL) {
            update_campaign_error(ecu, CAMPAIGN_ERR_TRANS);
        }
    }

    return ERR_OK;
}

error_t verify_targets(void)
{
    fota_image_t *fw_item;
    fota_ecu_t *ecu;
    error_t ret = ERR_NOK;
    const char* path = repo_get_dr_dl_image_path();
    for (ecu = get_ecu_list(); ecu != NULL; ecu = ecu->next)
    {
        fw_item = ecu->updates;
        if (fw_item != NULL)
        {
            if (ecu->campaign == ECU_CAMPAIGN_TRANS_VERIFY)
            {

                if (verify_image(path, ecu->updates) != ERR_OK)
                {
                    return ERR_NOK;
                }
            }
        }
    }
      update_ecu_campaign_list(ECU_CAMPAIGN_TRANS);
      repo_targets_dl_2_verified();
      ret = ERR_OK;
    
    return ret;
}

void trans_cb(const char *fname, int progress)
{
    char display[64];
    static display_cnt = 0;

    display_cnt++;
    if ((display_cnt & 31) == 0) {
        snprintf(display, 64, "Transfering file %s ...", fname);
#if (LCD_ENABLE == ON)
        setInstalling1file_EScreen(progress, display);
#else
        printf_it(LOG_INFO, "Transfering file %s ... [%d finished]", fname, progress);
#endif
        display_cnt = 0;
    }
}

error_t transfer_targets()
{
    fota_image_t *fw_item;
    fota_ecu_t *ecu;
    char fname[10][LENGTH_DIR_PATH];
    int32_t index = 0;

    for (ecu = get_ecu_list(); ecu != NULL; ecu = ecu->next) {
        fw_item = ecu->updates;

        if (fw_item != NULL) {
            if (ecu ->campaign > ECU_CAMPAIGN_TRANS)
                continue;
            else if (ecu ->campaign < ECU_CAMPAIGN_TRANS)
                break;

            if (ecu->fota_state != SECU_STATUS_IDLE) {
                printf_it(LOG_ERROR, "Remote ECU %s state is not IDLE: %d", ecu->hw_id, ecu->fota_state);
                break;
            }

            printf_it(LOG_TRACE, "transfer metadata and images to ECU %s", ecu->hw_id);

            index = 0;
            get_verified_metadata_fname(REPO_DIRECT, "root", fname[index++]);

            get_verified_metadata_fname(REPO_DIRECT, "timestamp", fname[index++]);
            
            get_verified_metadata_fname(REPO_DIRECT, "snapshot", fname[index++]);
            
            get_verified_metadata_fname(REPO_DIRECT, "targets", fname[index++]);

#if 0
            snprintf(fname[index++], LENGTH_DIR_PATH, "%s%s", REPO_TARGETS_DR REPO_VERI_RELATIVE, "firmware.bin");   

#else
            while (fw_item != NULL) {
            /* targets file */
                snprintf(fname[index++], LENGTH_DIR_PATH, "%s%s", REPO_TARGETS_DR REPO_VERI_RELATIVE, fw_item->fname);   

                fw_item = fw_item->next;
            }
#endif

            /* start transfer all files */
            if (fota_conn_transfer_files(ecu, index, fname, trans_cb) != ERR_OK)
                break; 

            update_campaign_error(ecu, CAMPAIGN_ERR_NONE);

            fota_update_ecu_campaign(ecu, ECU_CAMPAIGN_PROC_APPROVE, 0);

        }
    }

    if (ecu != NULL) {
        update_campaign_error(fw_item, CAMPAIGN_ERR_TRANS);
        return ERR_NOK;
    }

    return ERR_OK;
}

error_t force_ecu_ota_idle(void)
{
    fota_image_t *fw;
    fota_ecu_t *ecu;
    uint8_t progress, status;
    int32_t retry = 10;
    int32_t not_idle = 0;

    while (retry > 0) {
        not_idle = 0;
        for (ecu = get_ecu_list(); ecu != NULL; ecu = ecu->next) {
            fw = ecu->updates;
            if (fw != NULL) {
                if (fota_conn_cmd_get_status(ecu, &status, &progress) == ERR_OK) {
                    ecu->fota_state = status;

                    if (status != SECU_STATUS_IDLE) {
                        fota_conn_cmd_idle(ecu);
                        not_idle++;
                    } else {
                        continue;
                    }
                
                } else {
                    not_idle++;
                    printf_it(LOG_ERROR, "get status of ecu %s failed, retry", ecu->hw_id);
                }
                
            }
        }

        if (not_idle == 0) break;

        retry--;
        sleep(1); // retry after 1s
    }

    if (not_idle == 0)
        return ERR_OK;
    else
        return ERR_NOK;
}


void* fota_transfer_thread(void *arg)
{
    mqd_t mq_campaign_host, mq_trans_host;
    int32_t mq_len;
    int8_t mq_host_buf[64];
	int8_t mq_client_buf[64];
    error_t ret;
    char *pstr;
    int32_t tmp;
    int8_t string_updating_list[1024];

    /* open the queue */
	if ((mq_campaign_host = mq_open("/mq_fota_campaign_host", O_WRONLY)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_fota_campaign_host fail");

    if ((mq_trans_host = mq_open("/mq_fota_trans_host", O_RDONLY)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_fota_sync_host fail");

    printf_it(LOG_TRACE, "FOTA transfer thread starts");

    while(true) {
        
        /* waiting for msg */
		if ((mq_len = mq_receive(mq_trans_host, mq_host_buf, sizeof(mq_host_buf), NULL)) < 0) {
			printf_it(LOG_ERROR, "fota_campaign_main mq_receive error");
		}

        printf_it(LOG_INFO, "Get message:%s", mq_host_buf);

        if ((pstr = strstr(mq_host_buf, "trans:approve")) != NULL) {
//            printf_it(LOG_TRACE, "start to approve transfer");

            /* if there are updates available */
            if (is_updates_available() == 0) {
                continue;
            }

            printf_update_list(string_updating_list, sizeof(string_updating_list));

            update_ecu_campaign_list(ECU_CAMPAIGN_TRANS_APPROVE);
#if (LCD_ENABLE == ON)
            /* todo: approve process
                #1. trigger approve of downloading via UI
                #2. waiting for approve results (approved/timeout/decline)
            */
            updateAllScreenString(NULL, "Pending");
            setLogText(request_e_screen_id, "Waiting for user's approval...");

            switchScreen(request_e_screen_id);
            if (setRequest_EScreen(string_updating_list)) {
                update_ecu_campaign_list(ECU_CAMPAIGN_TRANS_DL);
            } else {
                switchScreen(fotademo_e_screen_id);

                snprintf(mq_client_buf, sizeof(mq_client_buf), 
                    "%s", "transApprove:cancel");
                goto _send_response_;
            }
#elif (INTEGRATION_TEST == ON)
            update_ecu_campaign_list(ECU_CAMPAIGN_TRANS_DL);
#else
            printf_it(LOG_INFO, "\nWaiting for approval of transfer.Please enter 'y' to continue...");
            while(1) {
                if ('y' == getchar())
                    break;
            }

            update_ecu_campaign_list(ECU_CAMPAIGN_TRANS_DL);
#endif

            snprintf(mq_client_buf, sizeof(mq_client_buf), 
                "%s", "transApprove:done");

            if (mq_send(mq_campaign_host, mq_client_buf, sizeof(mq_client_buf), 0) < 0) {
                printf_it(LOG_ERROR, "mq_send error");
                continue;
            }
        }
        else if ((pstr = strstr(mq_host_buf, "trans:transfer")) != NULL) {
//            printf_it(LOG_TRACE, "start to transfer");
            /*
                todo: download and verify downloaded images
                #1. update campaign json file
                #2. download image files in updates_fw_list
                #3. uptane full verification
                #4. transfer images to ECUs
            */

#if (LCD_ENABLE == ON)
            updateAllScreenString(NULL, "Downloading");
            switchScreen(downloading1file_screen_id);
            setLogText(downloading1file_screen_id, "Start download metadata files");
#endif
            
            printf_it(LOG_TRACE, "start to download images");
            if (download_targets() != ERR_OK) {
                printf_it(LOG_ERROR, "downloading targets failed");

                snprintf(mq_client_buf, sizeof(mq_client_buf), 
                    "%s", "trans:error.download");
            
                goto _send_response_;
            }

#if (LCD_ENABLE == ON)
            updateAllScreenString(NULL, "Verifying");
            switchScreen(verifying1file_e_screen_id);
            setVerifying1file_EScreen(0, "Start verify metadata and images...");
#endif

            printf_it(LOG_TRACE, "start to verify downloaded images");
            if (verify_targets() != ERR_OK) {
                snprintf(mq_client_buf, sizeof(mq_client_buf), 
                    "%s", "trans:error.verify");

                empty_update_ecu_list();
                
                goto _send_response_;

            }

#if (LCD_ENABLE == ON)
            sleep(1);
            setVerifying1file_EScreen(100, "Start verify metadata and images... Done!");
#endif
            /* check ECU OTA state, if the OTA state is not IDLE, try to bring back it to IDLE */
            if (force_ecu_ota_idle() != ERR_OK) {
                snprintf(mq_client_buf, sizeof(mq_client_buf), 
                    "%s", "trans:error.trans");
            
                goto _send_response_;
            }

            print_ecu_campaign();

#if (LCD_ENABLE == ON)
            updateAllScreenString(NULL, "Tranfering");
            switchScreen(installing1file_e_screen_id);
            setInstalling1file_EScreen(0, "Transfering metadata and images...");
            setLogText(installing1file_e_screen_id, "Verified OK! Start to transfer metadata to install");
#endif

            printf_it(LOG_TRACE, "start to transfer metadata and image to SECU");
            if (transfer_targets() != ERR_OK) {
                snprintf(mq_client_buf, sizeof(mq_client_buf), 
                    "%s", "trans:error.trans");
            
                goto _send_response_;

            }

#if (LCD_ENABLE == ON)
            setInstalling1file_EScreen(100, "Transfering metadata and images... Done");
            sleep(1);
#endif

            snprintf(mq_client_buf, sizeof(mq_client_buf), 
                "%s", "trans:done");

    _send_response_:                    
            if (mq_send(mq_campaign_host, mq_client_buf, sizeof(mq_client_buf), 0) < 0) {
                printf_it(LOG_ERROR, "mq_send error");
                continue;
            }
        }
    }

err_return:
    
        return;
}

