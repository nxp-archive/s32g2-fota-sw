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
#include "../http_client/http_client.h"
#include "fota_conn.h"
#include "campaign.h"

#if (LCD_ENABLE == ON)
#include "../touch_panel/cmd.h"
#endif

error_t sync_vehicle_manifest(void)
{
    fota_ecu_t *ecu_list;
    fota_ecu_t *item;
    char path[LENGTH_DIR_PATH];

    ecu_list = get_ecu_list();

    for (item = ecu_list; item != NULL; item = item->next) {
        /*
            get manifest file from SECU, store the file at REPO_MANIFEST
              with name manifest.[ecu serial].der
        */
//        printf_it(LOG_DEBUG,"ECU->serial:%s",item->serial);
//        printf_it(LOG_DEBUG,"ECU->hw_id:%s",item->hw_id);
        snprintf(path, LENGTH_DIR_PATH, REPO_MANIFEST "manifest." "%s", item->serial);
        if (uds_read_ecu_manifest(path, item->addr, item->addr_ext) != ERR_OK) {
            break;
        }

        if (parse_ecu_manifest_file(item, path) != ERR_OK) {
            /* file parse error */
            break;
        }
    }

    if (item == NULL) {
        /* get all ECU manifest file successfully, then 
           build vehicle manifest file */
        printf_it(LOG_TRACE, "start to build vehicle manifest file");
        return build_vehicle_manifest(ecu_list);
    } else {
        return ERR_NOK;
    }
}

error_t upload_vehicle_manifest(uint8_t *vin, uint8_t *primary_ecu_esrial, uint8_t* manifest_filename)
{
    /* via xml-rpc send the vehicle manifest to DIRECTOR seriver
    * medthod:submit_vehicle_manifest
    * param1:vin
    * param2:primary_ecu_esrial
    * param3:signed_vehicle_manifest  
	
    */
	return fota_conn_upload(vin, primary_ecu_esrial,manifest_filename);

}


void* fota_sync_thread(void *arg)
{
    mqd_t mq_campaign_host, mq_sync_host;
    int32_t mq_len;
    int8_t mq_host_buf[128];
	int8_t mq_client_buf[64];

    error_t ret;
    char *pstr;
    int32_t tmp;

    /* open the queue */
   
	if ((mq_campaign_host = mq_open("/mq_fota_campaign_host", O_WRONLY)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_fota_campaign_host fail");

    if ((mq_sync_host = mq_open("/mq_fota_sync_host", O_RDONLY)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_fota_sync_host fail");

    printf_it(LOG_TRACE, "FOTA sync thread starts");

    while(true) {

        /* waiting for msg */
		if ((mq_len = mq_receive(mq_sync_host, mq_host_buf, sizeof(mq_host_buf), NULL)) < 0) {
			printf_it(LOG_ERROR, "fota_campaign_main mq_receive errormq_len ：%d",mq_len);
            continue;
		}

        printf_it(LOG_INFO, "Get message:%s", mq_host_buf);

        if ((pstr = strstr(mq_host_buf, "sync:start")) == NULL)
            continue;

#if (LCD_ENABLE == ON)
        updateAllScreenString(NULL, "Sync");
        clearAllLogText();

        setLogText(fotademo_e_screen_id, "Collect vehicle manifest...");
#endif

        printf_it(LOG_TRACE, "start to sync vehicle manifest data");

        // collect ECU manifest and build vehicle manifest file         
        if (sync_vehicle_manifest() != ERR_OK) {
            snprintf(mq_client_buf, sizeof(mq_client_buf), 
                "%s", "sync:error.manifest");
            goto send_response;
        }

        /* upload vehicle manifest */
        int8_t *__vin = get_factory_vin();
        int8_t *__primary_ecu_esrial = get_factory_primary_serial();
        int8_t *__vehicle_manifest = repo_get_manifest_path();

#if (LCD_ENABLE == ON)
        setLogText(fotademo_e_screen_id, "Send manifest data to server...");
#endif
        if (upload_vehicle_manifest(__vin,__primary_ecu_esrial,__vehicle_manifest) != ERR_OK) {
            snprintf(mq_client_buf, sizeof(mq_client_buf), 
                "%s", "sync:error.upload");
            goto send_response;
        }
        
#if (LCD_ENABLE == ON)
                setLogText(fotademo_e_screen_id, "Check for updating...");
#endif

        printf_it(LOG_TRACE, "start download and verify update metadata files");
        if (download_verify_metadata() != ERR_OK) {
            snprintf(mq_client_buf, sizeof(mq_client_buf), 
                "%s", "sync:error.verify");
            goto send_response;
        }
        
        if (is_updates_available() == 0) {
            printf_it(LOG_TRACE, "updates list is empty, no update is available");

#if (LCD_ENABLE == ON)
            setLogText(fotademo_e_screen_id, "No update is available");
#endif

            snprintf(mq_client_buf, sizeof(mq_client_buf), 
                "%s", "sync:no_update");
            goto send_response;
        }
        
        snprintf(mq_client_buf, sizeof(mq_client_buf), 
            "%s", "sync:done");

send_response:
        if (mq_send(mq_campaign_host, mq_client_buf, sizeof(mq_client_buf), 0) < 0) {
            printf_it(LOG_ERROR, "mq_send error：%s",mq_client_buf);
        }
    }

err_return:
        return;
}

