/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <mqueue.h>
#include <errno.h>

pthread_t dl_tid;
pthread_t fota_it_tid;
pthread_t scp_test_tid;
/*
pthread_t fota_sync_tid;
pthread_t fota_campaign_tid;
*/
#include "it_deamon.h"
#include "it_dl.h"
#include "it_fota.h"
#include "inte_test.h"
#include "../fotav.h"
//#include "../campaign/campaign.h"

void fota_inte_test(void) 
{
	int32_t ret;

	struct mq_attr attr;

	/* initialize the queue attributes */
	attr.mq_flags = 0;
	attr.mq_maxmsg = 2;
	attr.mq_msgsize = 128;
	attr.mq_curmsgs = 0;
    
	/* create the message queue */
	if ((mq_open("/mq_it_host", O_CREAT, 0644, &attr)) < 0) {
		printf_it(LOG_ERROR, "mq_it_host fail %d\n", errno);
	}

	if ((mq_open("/mq_it_client", O_CREAT, 0644, &attr)) < 0) {
		printf_it(LOG_ERROR, "mq_it_client fail\n");
	}

	if ((mq_open("/mq_it_dl_cmd", O_CREAT, 0644, &attr)) < 0) {
		printf_it(LOG_ERROR, "mq_it_dl_cmd fail\n");
	}

	if ((mq_open("/mq_it_dl_rsp", O_CREAT, 0644, &attr)) < 0) {
		printf_it(LOG_ERROR, "mq_it_dl_rsp fail\n");
	}


	/* create threads */

	ret = pthread_create(&fota_it_tid, NULL, fota_it_main, NULL);
	if (ret != 0)
		printf_it(LOG_ERROR, "cannot create fota_it_main\n");


	ret = pthread_create(&dl_tid, NULL, dl_thread, NULL);
	if (ret != 0)
		printf_it(LOG_ERROR, "cannot create dl_thread\n");

#if (SCP_CLIENT_TEST == ON)
    extern void* scp_test_thread(void *arg);
	ret = pthread_create(&scp_test_tid, NULL, scp_test_thread, NULL);
	if (ret != 0)
		printf_it(LOG_ERROR, "cannot create scp_test_thread\n");
#endif
	
}