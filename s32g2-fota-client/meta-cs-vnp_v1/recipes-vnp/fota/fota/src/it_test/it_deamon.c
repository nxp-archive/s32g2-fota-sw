/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>

#include "../fotav.h"
#include "it_deamon.h"

void* test_thread(void *arg)
{
	int32_t sockfd, newsockfd, portno;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	int32_t ret;
	int8_t rx_buf[128];
	int8_t tx_buf[128];

	mqd_t mq_s, mq_c;
	int8_t mq_buf[128];
	int32_t mq_len;

	uint8_t *pstr;
	int32_t caseno;

	bool stopped = false;

	/* open the queue */
	if ((mq_s = mq_open("/mq_it_host", O_WRONLY)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_it_host fail\n");

	if ((mq_c = mq_open("/mq_it_client", O_RDONLY)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_it_client fail\n");

	/* socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		printf_it(LOG_ERROR, "opening socket error\n");

	clilen = sizeof(cli_addr);

	memset((char *)&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(8888);

	if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
		printf_it(LOG_ERROR, "binding\n");

	listen(sockfd, 5);
	printf_it(LOG_INFO, "test_thread starts, listening...");
	printf_it(LOG_INFO, "waiting for client conn...");
	if ((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) < 0) {
		printf_it(LOG_ERROR, "accept error\n");
	}
	printf_it(LOG_INFO, "client accepted!\n");
	
	while(true) {
		ret = read(newsockfd, &rx_buf[0], sizeof(rx_buf));
		if (ret < 0)
			printf_it(LOG_ERROR, "read error\n");

		rx_buf[ret] = 0;
		printf_it(LOG_INFO, "rx_buf=%s\n", rx_buf);

		if ((pstr = strstr(rx_buf, "START:case")) != NULL) {
			caseno = strtol(pstr + strlen("start:case"), NULL, 10);
			printf_it(LOG_INFO, "received command %s. caseno=%d\n", rx_buf, caseno);

			snprintf(mq_buf, 128, 
				"start=case%04d", caseno);

			/* send message */
			printf_it(LOG_INFO, "sending msg:%s\n", mq_buf);
			if (mq_send(mq_s, mq_buf, sizeof(mq_buf), 0) < 0) {
				printf_it(LOG_ERROR, "mq_send error\n");
			}

			/* waiting for message response */
			printf_it(LOG_INFO, "waiting for resp msg\n");
			if ((mq_len = mq_receive(mq_c, mq_buf, sizeof(mq_buf), NULL)) < 0) {
				printf_it(LOG_ERROR, "mq_receive error\n");
			}

			if ((pstr = strstr(mq_buf, "--pass--")) == NULL) {
				
				pstr = strstr(mq_buf, ", ");
				snprintf(tx_buf, 128, 
					"END:case%d, fail%s", caseno, pstr);

			} else {
				snprintf(tx_buf, 128, 
					"END:case%d, pass", caseno);
			}

		} else if ((pstr = strstr(rx_buf, "STOP:")) != NULL) {
			snprintf(tx_buf, 128, 
				"stopped");
			stopped = true;
		} else {
			strcpy(tx_buf, "error: command not supported");
			printf_it(LOG_INFO, "command %s not supported\n", rx_buf);
		}

		/* socket response */
		printf_it(LOG_INFO, "sending resp %s", tx_buf);
		ret = write(newsockfd, &tx_buf[0], strlen(tx_buf));
		if (ret != strlen(tx_buf))
			printf_it(LOG_ERROR, "write error: len=%d\n", strlen(tx_buf));

		if (stopped) exit(0);
	}
}
