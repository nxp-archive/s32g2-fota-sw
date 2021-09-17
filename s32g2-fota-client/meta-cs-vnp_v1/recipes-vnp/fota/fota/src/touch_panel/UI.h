/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#ifndef	_UI_H_
#define	_UI_H_

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wait.h>
#include <errno.h>
#include <netdb.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <stdint.h>
#include <fcntl.h>

/*------------------------------------------------------------------------------
                             Local Macros & Constants
------------------------------------------------------------------------------*/




/*-----------------------------------------------------------------------------
                          API Function Declarations
-----------------------------------------------------------------------------*/
void *UI_Thread(void *arg);

#endif
