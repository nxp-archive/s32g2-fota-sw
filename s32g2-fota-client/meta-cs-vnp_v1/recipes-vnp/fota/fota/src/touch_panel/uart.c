/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<termios.h>
#include<errno.h>
#include<string.h>
#include<stdbool.h>
#include<getopt.h>
#include<time.h>
#include<pthread.h>

#include "uart.h"

int fd=-1;
int initialUart(void)
{
	char *device_name = "/dev/ttyLF2";
	/* open uart device, set baud rate */
	fd = Uart_open(device_name, 9600);
	printf("the device fd is %d,%s %d\n",fd, __func__,__LINE__);
}
/**
 * @brief Open the specified device serial port and return the serial port file operation descriptor fd.
 * @return success: fd. fail:0
 *
 */
int Uart_open(char *device_name, int speed)
{

	/* Open the Uart device */
	//int fd = open(device_name, O_RDWR |O_NOCTTY|O_NDELAY);
	int fd = open(device_name, O_RDWR |O_NOCTTY);
	if (fd < 0) {
		perror("Open SerialPort faild:");
		return -1;
	}

	/* set fd as blocking mode */
	if (fcntl(fd, F_SETFL, 0) < 0) {
		perror("fcntl F_SETFL:");
		return -1;
	}

	/* fd device type check */
	if (isatty(fd) == 0) {
		printf("this is not termial device");
		return -1;
	}

	/* Serial port option settings */
	struct termios options;
	tcgetattr(fd, &options);

	options.c_cflag |= (CLOCAL | CREAD);
	options.c_cflag &= ~CSIZE;
	options.c_cflag &= ~CRTSCTS;
	options.c_cflag |= CS8;
	options.c_cflag &= ~CSTOPB;
	options.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	options.c_oflag = 0;
	options.c_lflag = 0;

	// set baud rate
	switch(speed)
	{
		case 2400:
			cfsetispeed(&options, B2400);
			cfsetospeed(&options, B2400);
			break;
		case 4800:
			cfsetispeed(&options, B4800);
			cfsetospeed(&options, B4800);
			break;
		case 9600:
			cfsetispeed(&options, B9600);
			cfsetospeed(&options, B9600);
			break;
		case 115200:
			cfsetispeed(&options, B115200);
			cfsetospeed(&options, B115200);
			break;
		default:
			cfsetispeed(&options, B9600);
			cfsetospeed(&options, B9600);
			break;
	}


	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &options);

	return fd;
}

/**
 * @brief Serial port operation to realize data sending
 * @param fd 
 * @param data 
 * @param datalen 
 * @return success: data length. fail: -1
 *
 */
int Uart_send(int fd, char *data, int datalen) {
	int len = 0;
	len = write(fd, data, datalen);

	if (len == datalen) {
		printf("fd : %d ; the len is:%d\n", fd, len);
		return len;
	} else {
		tcflush(fd, TCIFLUSH); //TCFLUSH flush but do not send data
		return -1;
	}

	return -1;
}

/**
 * @brief Serial port operation to realize data receive
 * @param fd 
 * @param data 
 * @return success: 0. fail: -1
 *
 */
int Uart_receive(int fd, char *data) 
{
	
}





