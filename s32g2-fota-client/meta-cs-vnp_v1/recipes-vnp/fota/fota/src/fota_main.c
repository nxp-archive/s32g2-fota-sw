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
#include "campaign/campaign.h"
#include <sys/inotify.h>

#if (LCD_ENABLE == ON)
#include "touch_panel/uart.h"
#include "touch_panel/cmd.h"
extern void update_display_demo(void);
#endif

pthread_t fota_campaign_tid;
pthread_t fota_factory_tid;
pthread_t fota_sync_tid;
pthread_t fota_trans_tid;
pthread_t fota_proc_tid;
pthread_t fota_acti_tid;
pthread_t fota_check_tid;
pthread_t fota_rollback_tid;

#define MQ_SEND(command, host)    do { \
            snprintf(mq_client_buf, sizeof(mq_client_buf), "%s", command); \
            if (mq_send(host, mq_client_buf, sizeof(mq_client_buf), 0) < 0) { \
			    printf_it(LOG_ERROR, "mq_send %s error", command); } } while(0)

void* fota_campaign_main(void *arg)
{
    mqd_t mq_host, mq_client, mq_factory_host, mq_sync_host;
    mqd_t mq_trans_host, mq_proc_host, mq_acti_host, mq_chek_host, mq_rollback_host;
    int32_t mq_len;
    int8_t mq_host_buf[64];
	int8_t mq_client_buf[64];
    error_t ret;
    char *pstr;
    int32_t tmp;

    int32_t registered = 0; /* if the vehicle is registered */
    int32_t campaign = CAMPAIGN_STATE_IDLE;

    int32_t trans_retry = 4;
    int32_t proc_retry = 4;
    int32_t acti_retry = 4;
    int32_t check_retry = 4;

    /* open the queue */
	if ((mq_host = mq_open("/mq_fota_campaign_host", O_RDONLY)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_fota_campaign_host fail");

    if ((mq_client = mq_open("/mq_fota_campaign_client", O_WRONLY)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_fota_campaign_client fail");

    if ((mq_factory_host = mq_open("/mq_fota_factory_host", O_WRONLY)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_fota_factory_host fail");

    if ((mq_sync_host = mq_open("/mq_fota_sync_host", O_WRONLY)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_fota_sync_host fail");

    if ((mq_trans_host = mq_open("/mq_fota_trans_host", O_WRONLY)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_fota_trans_host fail");

    if ((mq_proc_host = mq_open("/mq_fota_proc_host", O_WRONLY)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_fota_proc_host fail");

    if ((mq_acti_host = mq_open("/mq_fota_acti_host", O_WRONLY)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_fota_acti_host fail");

    if ((mq_chek_host = mq_open("/mq_fota_chek_host", O_WRONLY)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_fota_chek_host fail");

    if ((mq_rollback_host = mq_open("/mq_fota_rollback_host", O_WRONLY)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_fota_rollback_host fail");

    printf_it(LOG_TRACE, "FOTA main thread starts");

    /* #1: waiting for start msg */
	if ((mq_len = mq_receive(mq_host, mq_host_buf, sizeof(mq_host_buf), NULL)) < 0) {
		printf_it(LOG_ERROR, "fota_campaign_main mq_receive error");
	}

    printf_it(LOG_INFO, " fota_campaign_main Get message:%s", mq_host_buf);

    if ((pstr = strstr(mq_host_buf, "start fota")) == NULL) {
        snprintf(mq_client_buf, sizeof(mq_client_buf), 
			"error: not support command");

        /* response msg */
        printf_it(LOG_INFO, " mq_send mq_host_buf message:%s", mq_host_buf);
		if (mq_send(mq_client, mq_client_buf, sizeof(mq_client_buf), 0) < 0) {
			printf_it(LOG_ERROR, "mq_send error");
		}
        
        goto err_return;
    }

    /* #2: system init */
    printf_it(LOG_TRACE, "start Initialize system");
   
	if (fota_system_init(&registered) != ERR_OK) {
		printf_it(LOG_ERROR, "system init fail");
		goto err_return;
	}

    printf_it(LOG_TRACE, "Initialize system done: registered?%d",registered);

    if (registered == 0) {

        fota_update_vehicle_campaign(CAMPAIGN_STATE_REG, &campaign);

        MQ_SEND("register:start", mq_factory_host);
    }

    /* #3: check campaign state */
    else if (registered == 1) {
        printf_it(LOG_TRACE, "The vehicle is registered, getting campaign state");
        if (fota_get_vehicle_campaign_state(&tmp) == ERR_OK) {
            fota_update_vehicle_campaign((vehicle_campaign_state)tmp, &campaign);
        }
        printf_it(LOG_TRACE, "campaign state:%d", (int32_t)campaign);

        if (campaign > CAMPAIGN_STATE_SYNC)
            parse_verified_metadata();
        
        fota_get_ecu_campaign_state();
        printf_it(LOG_DEBUG, "fota_get_ecu_campaign_state");
        switch (campaign) {
            case CAMPAIGN_STATE_SYNC:
                printf_it(LOG_TRACE, "CAMPAIGN_STATE_SYNC");
                MQ_SEND("sync:start", mq_sync_host);
                break;
            case CAMPAIGN_STATE_TRANS_APPROVE:
                MQ_SEND("trans:approve", mq_trans_host);
                printf_it(LOG_TRACE, "CAMPAIGN_STATE_TRANS_APPROVE");
                break;
            case CAMPAIGN_STATE_TRANS:
                MQ_SEND("trans:transfer", mq_trans_host);
                printf_it(LOG_TRACE, "CAMPAIGN_STATE_TRANS");
                break;
            case CAMPAIGN_STATE_PROC_APPROVE:
                MQ_SEND("proc:approve", mq_proc_host);
                printf_it(LOG_TRACE, "CAMPAIGN_STATE_PROC_APPROVE");
                break;
            case CAMPAIGN_STATE_PROC:
                printf_it(LOG_TRACE, "CAMPAIGN_STATE_PROC");
                MQ_SEND("proc:process", mq_proc_host);
                break;
            case CAMPAIGN_STATE_ACTIVE_APPROVE:
                printf_it(LOG_TRACE, "CAMPAIGN_STATE_ACTIVE_APPROVE");
                MQ_SEND("acti:approve", mq_acti_host);
                break;
            case CAMPAIGN_STATE_ACTIVE:
                printf_it(LOG_TRACE, "CAMPAIGN_STATE_ACTIVE");
                MQ_SEND("acti:activate", mq_acti_host);
                break;
            case CAMPAIGN_STATE_CHECK:
                printf_it(LOG_TRACE, "CAMPAIGN_STATE_CHECK");
                MQ_SEND("check:check", mq_chek_host);
                break;
            default:
                break;
        }
    }
    

    while(true) {
        printf_it(LOG_TRACE, "campaign state:%d", (int32_t)campaign);

        if (campaign == CAMPAIGN_STATE_IDLE) {

#if (LCD_ENABLE == ON)
            update_display_demo();
#endif
            /* updating cycle */
            sleep(FOTA_UPDATING_CYCLE_S);

            trans_retry = 4;
            proc_retry = 4;
            acti_retry = 4;
            check_retry = 4;

            printf_it(LOG_TRACE, "start FOTA updating cycle");

            fota_update_vehicle_campaign(CAMPAIGN_STATE_SYNC, &campaign);
            
            MQ_SEND("sync:start", mq_sync_host);
        }
        
        /* waiting for msg */
		if ((mq_len = mq_receive(mq_host, mq_host_buf, sizeof(mq_host_buf), NULL)) < 0) {
			printf_it(LOG_ERROR, "fota_campaign_main mq_receive error");
		}

        printf_it(LOG_INFO, "Get message:%s", mq_host_buf);

        if ((pstr = strstr(mq_host_buf, "register:done")) != NULL) {
//            printf_it(LOG_TRACE, "register is done");
            
            registered = 1;

            if (campaign == CAMPAIGN_STATE_REG)
                fota_update_vehicle_campaign(CAMPAIGN_STATE_IDLE, &campaign);
        }

        if (campaign == CAMPAIGN_STATE_REG)
            continue;
        
        if ((pstr = strstr(mq_host_buf, "sync:done")) != NULL) {
            printf_it(LOG_TRACE, "sync is done");

            if (campaign == CAMPAIGN_STATE_SYNC)
                fota_update_vehicle_campaign(CAMPAIGN_STATE_TRANS_APPROVE, &campaign);
                        
            MQ_SEND("trans:approve", mq_trans_host);
        }
        if ((pstr = strstr(mq_host_buf, "sync:no_update")) != NULL) {
            if ((campaign == CAMPAIGN_STATE_SYNC) /*&& (!g_fota_fw_available_list)*/)
                fota_update_vehicle_campaign(CAMPAIGN_STATE_IDLE, &campaign);
        }
        else if ((pstr = strstr(mq_host_buf, "sync:error")) != NULL) {
            printf_it(LOG_TRACE, "sync failed, wait for retry");
#if (LCD_ENABLE == ON)
            updateAllScreenString(NULL, "Sync error");
#endif

            sleep(FOTA_UPDATING_CYCLE_S);
            if ((campaign == CAMPAIGN_STATE_SYNC))
                fota_update_vehicle_campaign(CAMPAIGN_STATE_IDLE, &campaign);
        }
        else if ((pstr = strstr(mq_host_buf, "transApprove:done")) != NULL) {
            printf_it(LOG_TRACE, "transfer approval is done");
            
            if (campaign == CAMPAIGN_STATE_TRANS_APPROVE) {
                fota_update_vehicle_campaign(CAMPAIGN_STATE_TRANS, &campaign);
            }

            MQ_SEND("trans:transfer", mq_trans_host);
        }
        else if ((pstr = strstr(mq_host_buf, "transApprove:cancel")) != NULL) {
            printf_it(LOG_TRACE, "transfer approval is cancel");
            
            fota_update_vehicle_campaign(CAMPAIGN_STATE_IDLE, &campaign);
            continue;
        }
        else if ((pstr = strstr(mq_host_buf, "trans:error.verify")) != NULL) {
            printf_it(LOG_TRACE, "verification error, drop the updates");

            fota_update_vehicle_campaign(CAMPAIGN_STATE_IDLE, &campaign);
            continue;
        }
        else if ((pstr = strstr(mq_host_buf, "trans:error")) != NULL) {
#if (LCD_ENABLE == ON)
            updateAllScreenString(NULL, "Transfer error");
#endif

            sleep(FOTA_UPDATING_CYCLE_S);

            trans_retry--;
            if (trans_retry < 0) {
                
#if (INTEGRATION_TEST == ON)
                exit(EXIT_FAILURE);
#endif
                fota_update_vehicle_campaign(CAMPAIGN_STATE_CHECK, &campaign);

                MQ_SEND("check:check", mq_chek_host);
            
                continue;
            }

            MQ_SEND("trans:transfer", mq_trans_host);

        }
        else if ((pstr = strstr(mq_host_buf, "trans:done")) != NULL) {
            printf_it(LOG_TRACE, "transfer is done");
            
            if (campaign == CAMPAIGN_STATE_TRANS) {
                fota_update_vehicle_campaign(CAMPAIGN_STATE_PROC_APPROVE, &campaign);
            }   
            
            snprintf(mq_client_buf, sizeof(mq_client_buf), 
                "%s", "proc:approve");
                    
            MQ_SEND("proc:approve", mq_proc_host);
        }
        else if ((pstr = strstr(mq_host_buf, "procApprove:done")) != NULL) {
            printf_it(LOG_TRACE, "process approval is done");
            
            if (campaign == CAMPAIGN_STATE_PROC_APPROVE) {
                fota_update_vehicle_campaign(CAMPAIGN_STATE_PROC, &campaign);
            }

            MQ_SEND("proc:process", mq_proc_host);
        }
        else if ((pstr = strstr(mq_host_buf, "proc:error")) != NULL) {
#if (LCD_ENABLE == ON)
            updateAllScreenString(NULL, "Proc error");
#endif

            sleep(FOTA_UPDATING_CYCLE_S);

            proc_retry--;
            if (proc_retry < 0) {

#if (INTEGRATION_TEST == ON)
                exit(EXIT_FAILURE);
#endif

                fota_update_vehicle_campaign(CAMPAIGN_STATE_CHECK, &campaign);
                MQ_SEND("check:check", mq_chek_host);
                continue;
            }

            MQ_SEND("proc:process", mq_proc_host);
        }
        else if ((pstr = strstr(mq_host_buf, "proc:done")) != NULL) {
            printf_it(LOG_TRACE, "process is done");
            
            if (campaign == CAMPAIGN_STATE_PROC) {
                fota_update_vehicle_campaign(CAMPAIGN_STATE_ACTIVE_APPROVE, &campaign);
            }

            MQ_SEND("acti:approve", mq_acti_host);
        }
        else if ((pstr = strstr(mq_host_buf, "actiApprove:done")) != NULL) {
            printf_it(LOG_TRACE, "activation approval is done");
            
            if (campaign == CAMPAIGN_STATE_ACTIVE_APPROVE) {
                fota_update_vehicle_campaign(CAMPAIGN_STATE_ACTIVE, &campaign);
            }

            MQ_SEND("acti:activate", mq_acti_host);
        }
        else if ((pstr = strstr(mq_host_buf, "acti:error")) != NULL) {
            
#if (LCD_ENABLE == ON)
            updateAllScreenString(NULL, "Activation error");
#endif

            sleep(FOTA_UPDATING_CYCLE_S);

            acti_retry--;
            if (acti_retry < 0) {

#if (INTEGRATION_TEST == ON)
                exit(EXIT_FAILURE);
#endif

                fota_update_vehicle_campaign(CAMPAIGN_STATE_CHECK, &campaign);
                
                MQ_SEND("check:check", mq_chek_host);
                
                continue;
            }

            MQ_SEND("acti:activate", mq_acti_host);
        }
        else if ((pstr = strstr(mq_host_buf, "acti:done")) != NULL) {
            printf_it(LOG_TRACE, "activation is done");
            
            if (campaign == CAMPAIGN_STATE_ACTIVE) {
                fota_update_vehicle_campaign(CAMPAIGN_STATE_CHECK, &campaign);
            }

            MQ_SEND("check:check", mq_chek_host);
        }
        else if ((pstr = strstr(mq_host_buf, "check:done")) != NULL) {
            printf_it(LOG_TRACE, "checking is done");
            
            if (campaign == CAMPAIGN_STATE_CHECK) {
                fota_update_vehicle_campaign(CAMPAIGN_STATE_IDLE, &campaign);
            }
        }
        else if ((pstr = strstr(mq_host_buf, "check:error")) != NULL) {

            sleep(FOTA_UPDATING_CYCLE_S);

#if (LCD_ENABLE == ON)
            updateAllScreenString(NULL, "Check error");
#endif
            check_retry--;
            if (check_retry < 0) {

#if (INTEGRATION_TEST == ON)
                exit(EXIT_FAILURE);
#endif

                printf_dbg(LOG_ERROR, "Did not finish installation, abort...");
                fota_update_vehicle_campaign(CAMPAIGN_STATE_IDLE, &campaign);
                continue;
            }
            
            MQ_SEND("check:check", mq_chek_host);
        }
    }

err_return:
    
        return;
}

void fota_main(void)
{
	int32_t ret;

	struct mq_attr attr;

	/* initialize the queue attributes */
	attr.mq_flags = 0;
	attr.mq_maxmsg = 2;
	attr.mq_msgsize = 64;
	attr.mq_curmsgs = 0;

#if (LCD_ENABLE == ON)
    initialUart();
	initmap();
	clearAllScreenLog();
#endif
    
	/* create the message queue */
	if ((mq_open("/mq_fota_campaign_host", O_CREAT, 0644, &attr)) < 0) {
		printf_it(LOG_ERROR, "mq_fota_campaign_host fail errno=%d\n", errno);
	}

	if ((mq_open("/mq_fota_campaign_client", O_CREAT, 0644, &attr)) < 0) {
		printf_it(LOG_ERROR, "mq_fota_campaign_client fail\n");
	}
    if ((mq_open("/mq_fota_factory_host", O_CREAT, 0644, &attr)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_fota_factory_host fail");

    if ((mq_open("/mq_fota_sync_host", O_CREAT, 0644, &attr)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_fota_sync_host fail");

    if ((mq_open("/mq_fota_trans_host", O_CREAT, 0644, &attr)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_fota_trans_host fail");

    if ((mq_open("/mq_fota_proc_host", O_CREAT, 0644, &attr)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_fota_proc_host fail");

    if ((mq_open("/mq_fota_acti_host", O_CREAT, 0644, &attr)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_fota_acti_host fail");

    if ((mq_open("/mq_fota_chek_host", O_CREAT, 0644, &attr)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_fota_chek_host fail");

    if ((mq_open("/mq_fota_rollback_host", O_CREAT, 0644, &attr)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_fota_rollback_host fail");


	/* create threads */
    ret = pthread_create(&fota_sync_tid, NULL, fota_sync_thread, NULL);
	if (ret != 0)
		printf_it(LOG_ERROR, "creating fota_sync_thread failed");

    ret = pthread_create(&fota_factory_tid, NULL, fota_register_thread, NULL);
	if (ret != 0)
		printf_it(LOG_ERROR, "creating fota_factory_thread failed");

    ret = pthread_create(&fota_trans_tid, NULL, fota_transfer_thread, NULL);
	if (ret != 0)
		printf_it(LOG_ERROR, "creating fota_trans_thread failed");

    ret = pthread_create(&fota_proc_tid, NULL, fota_process_thread, NULL);
	if (ret != 0)
		printf_it(LOG_ERROR, "creating fota_proc_thread failed");

    ret = pthread_create(&fota_acti_tid, NULL, fota_activate_thread, NULL);
	if (ret != 0)
		printf_it(LOG_ERROR, "creating fota_acti_thread failed");

    ret = pthread_create(&fota_check_tid, NULL, fota_check_thread, NULL);
	if (ret != 0)
		printf_it(LOG_ERROR, "creating fota_eheck_thread failed");

    ret = pthread_create(&fota_rollback_tid, NULL, fota_rollback_thread, NULL);
	if (ret != 0)
		printf_it(LOG_ERROR, "creating fota_rollback_thread failed");

    ret = pthread_create(&fota_campaign_tid, NULL, fota_campaign_main, NULL);
	if (ret != 0)
		printf_it(LOG_ERROR, "creating fota_it_main failed");
}