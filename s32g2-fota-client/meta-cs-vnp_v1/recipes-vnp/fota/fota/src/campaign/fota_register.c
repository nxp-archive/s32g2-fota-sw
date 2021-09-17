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
#include "fota_conn.h"
#include "../repo/repo.h"
#include "campaign.h"

void check_ecu_fw_state(int32_t *unregistered)
{
    fota_ecu_t *ecu_list;
    fota_ecu_t *item;
    int32_t unregist = 0;

    ecu_list = get_ecu_list();

    for (item = ecu_list; item != NULL; item = item->next) {
        if (item->fw.status == FW_STATUS_UNREGISTERED) {
            unregist++;
        }
    }

    *unregistered = unregist;
}

static void check_register_from_repo(int32_t *is_vehicle_registered)
{
    if (fota_conn_get_dl_url(NULL) != ERR_OK)
        *is_vehicle_registered = 0;
    else 
        *is_vehicle_registered = 1;
}

static error_t transfer_init_metadata(void)
{
    /* todo: transfer the initial metadata files to SECU */
    fota_ecu_t *ecu_list;
    fota_ecu_t *item;
    char path[128];

    // for debug
    return ERR_OK;
    
#if 0
    ecu_list = get_ecu_list();

    for (item = ecu_list; item != NULL; item = item->next) {
        if (item->fw.status == FW_STATUS_UNREGISTERED) {
            snprintf(path, 128, "%s%s", repo_get_dr_curr_md_path(), "root.der");
            if (uds_transfer_files(item->addr, item->addr_ext, path) != ERR_OK)
                break;

            snprintf(path, 128, "%s%s", repo_get_dr_curr_md_path(), "timestamp.der");
            if (uds_transfer_files(item->addr, item->addr_ext, path) != ERR_OK)
                break;

            snprintf(path, 128, "%s%s", repo_get_dr_curr_md_path(), "snapshot.der");
            if (uds_transfer_files(item->addr, item->addr_ext, path) != ERR_OK)
                break;

            snprintf(path, 128, "%s%s", repo_get_dr_curr_md_path(), "targets.der");
            if (uds_transfer_files(item->addr, item->addr_ext, path) != ERR_OK)
                break;

            snprintf(path, 128, "%s%s", repo_get_ir_curr_md_path(), "root.der");
            if (uds_transfer_files(item->addr, item->addr_ext, path) != ERR_OK)
                break;

            snprintf(path, 128, "%s%s", repo_get_ir_curr_md_path(), "timestamp.der");
            if (uds_transfer_files(item->addr, item->addr_ext, path) != ERR_OK)
                break;

            snprintf(path, 128, "%s%s", repo_get_ir_curr_md_path(), "snapshot.der");
            if (uds_transfer_files(item->addr, item->addr_ext, path) != ERR_OK)
                break;

            snprintf(path, 128, "%s%s", repo_get_ir_curr_md_path(), "targets.der");
            if (uds_transfer_files(item->addr, item->addr_ext, path) != ERR_OK)
                break;
        }
    }
    if (item == NULL)
        return ERR_OK;
    else
        return ERR_NOK;
#endif
}

void* fota_register_thread(void *arg)
{
    mqd_t mq_campaign_host, mq_regi_host;
    int32_t mq_len;
    int8_t mq_host_buf[64];
	int8_t mq_client_buf[64];
    error_t ret;
    char *pstr;
    int32_t tmp;
    int32_t unregistered = 0;
    int32_t is_vehicle_registered = 0;
    int32_t retry = 2;

    /* open the queue */
	if ((mq_campaign_host = mq_open("/mq_fota_campaign_host", O_WRONLY)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_fota_campaign_host fail");

    if ((mq_regi_host = mq_open("/mq_fota_factory_host", O_RDONLY)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_fota_sync_host fail");

    printf_it(LOG_TRACE, "FOTA register thread starts");

    while(true) {
        
        /* waiting for msg */
		if ((mq_len = mq_receive(mq_regi_host, mq_host_buf, sizeof(mq_host_buf), NULL)) < 0) {
			printf_it(LOG_ERROR, "fota_campaign_main mq_receive error");
            continue;
		}

        printf_it(LOG_INFO, "Get message:%s", mq_host_buf);

        if ((pstr = strstr(mq_host_buf, "register:start")) != NULL) {
            printf_it(LOG_TRACE, "start to register");

            /* collect ECU manifest and build vehicle manifest file */
            printf_it(LOG_TRACE, "start to sync vehicle manifest data");
            if (sync_vehicle_manifest() != ERR_OK) {
                continue;
            }

            /* make sure the factory metadata is consistent with current running version */
            if (check_factory_metadata_consistent() != ERR_OK) {
                printf_it(LOG_ERROR, "Initial metadata is not consistent with ECU current fw version");
                continue;
            }

            /* check if all ECU are registered, if not, try to register it */
            check_ecu_fw_state(&unregistered);

            printf_it(LOG_TRACE, "%d ECUs are not registered", unregistered);
            
            while ((unregistered > 0) && (retry > 0)) {
                /*
                printf_it(LOG_TRACE, "start to upload vehicle manifest to register %d", retry);
                if (upload_vehicle_manifest() != ERR_OK) {
                    retry--;
                    continue;
                }
                */
                /* check with repo if register is finished, by polling geting the download url
                    for DRS, if register is successful, download url will be feedbacked per requesting
                        if register is not successful, download url will not available
                    */
                check_register_from_repo(&is_vehicle_registered);    
                if (is_vehicle_registered > 0) {
                    printf_it(LOG_TRACE, "vehicle is registered");
                    break;
                }

                sleep(5);
                
                retry--;
            }

            /* transfer the initial metadata to ECU */
            if (is_vehicle_registered > 0) {
                /* transfer the metadata files in "current" fold to ECU 
                    After the SECU getting the initial metadata file, it's expected to set 
                    its siftware state to FW_STATUS_PRESENT.
                    */
                printf_it(LOG_TRACE, "transfer init metadata files to ECUs");
                if (transfer_init_metadata() != ERR_OK) {
                    continue;
                }
            }

            if (is_vehicle_registered > 0)
                snprintf(mq_client_buf, sizeof(mq_client_buf), 
                    "%s", "register:done");
                    
            if (mq_send(mq_campaign_host, mq_client_buf, sizeof(mq_client_buf), 0) < 0) {
                printf_it(LOG_ERROR, "mq_send error");
                continue;
            }
        }
    }

    err_return:
    
        return;
}

