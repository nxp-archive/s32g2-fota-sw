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

void* fota_rollback_thread(void *arg)
{
    mqd_t mq_campaign_host, mq_rollb_host;
    int32_t mq_len;
    int8_t mq_host_buf[64];
	int8_t mq_client_buf[64];
    error_t ret;
    char *pstr;
    int32_t tmp;

    /* open the queue */
	if ((mq_campaign_host = mq_open("/mq_fota_campaign_host", O_WRONLY)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_fota_campaign_host fail");

    if ((mq_rollb_host = mq_open("/mq_fota_rollback_host", O_RDONLY)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_fota_rollback_host fail");

    printf_it(LOG_TRACE, "FOTA rollback thread starts");

    while(true) {
        
        /* waiting for msg */
		if ((mq_len = mq_receive(mq_rollb_host, mq_host_buf, sizeof(mq_host_buf), NULL)) < 0) {
			printf_it(LOG_ERROR, "fota_campaign_main mq_receive error");
		}

        printf_it(LOG_INFO, "Get message:%s", mq_host_buf);

        if ((pstr = strstr(mq_host_buf, "rollb:rollback")) != NULL) {
            printf_it(LOG_TRACE, "start to sync");

            /**/

        

            snprintf(mq_client_buf, sizeof(mq_client_buf), 
                "%s", "rollb:done");
                    
            if (mq_send(mq_campaign_host, mq_client_buf, sizeof(mq_client_buf), 0) < 0) {
                printf_it(LOG_ERROR, "mq_send error");
                continue;
            }
        }
    }

    err_return:
    
        return;
}

