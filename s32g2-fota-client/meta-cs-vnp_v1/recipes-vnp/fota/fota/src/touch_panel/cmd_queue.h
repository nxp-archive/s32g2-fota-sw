/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#ifndef _CMD_QUEUE_H
#define _CMD_QUEUE_H
#include<stdio.h>
#include<stdint.h>
#include<stdbool.h>
#define QUEUE_MAX_SIZE 3



typedef struct textqueue
{
    int tail;
    char* data[QUEUE_MAX_SIZE];
};
extern struct textqueue fotademo_e_queue;
extern struct textqueue request_e_queue;
extern struct textqueue downloading1file_e_queue;
extern struct textqueue verifying1file_e_queue;
extern struct textqueue verifydone_e_queue;
extern struct textqueue installing1file_e_queue;
extern struct textqueue acitvate_e_queue;
extern struct textqueue successful_e_queue;
extern struct textqueue failed_e_queue;

bool cmd_queue_push(struct textqueue* text_queue,char* data);
#endif
