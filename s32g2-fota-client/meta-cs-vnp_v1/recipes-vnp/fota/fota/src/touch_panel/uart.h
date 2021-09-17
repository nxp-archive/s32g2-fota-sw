/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#ifndef	_UART_H_
#define	_UART_H_

/*------------------------------------------------------------------------------
                             Local Macros & Constants
------------------------------------------------------------------------------*/




/*-----------------------------------------------------------------------------
                          API Function Declarations
-----------------------------------------------------------------------------*/
int Uart_open(char *device_name, int speed);
int Uart_send(int fd ,char *data,int datalen);
int Uart_receive(int fd, char *data);
int initialUart(void);
extern int fd;

#endif





