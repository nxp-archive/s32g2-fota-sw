/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#include"cmd_queue.h"

struct textqueue fotademo_e_queue = {0,NULL};
struct textqueue request_e_queue = {0,NULL};
struct textqueue downloading1file_e_queue = {0,NULL};
struct textqueue verifying1file_e_queue = {0,NULL};
struct textqueue verifydone_e_queue = {0,NULL};
struct textqueue installing1file_e_queue = {0,NULL};
struct textqueue acitvate_e_queue = {0,NULL};
struct textqueue successful_e_queue = {0,NULL};
struct textqueue failed_e_queue = {0,NULL};


bool cmd_queue_push(struct textqueue* text_queue,char* data)
{
    if(text_queue->tail < QUEUE_MAX_SIZE) //not empty
    {
        text_queue->data[text_queue->tail]=data;
        (text_queue->tail)++;
        return true;
    }
    else
    {
        text_queue->data[0]= text_queue->data[1];
        text_queue->data[1]= text_queue->data[2];
        text_queue->data[2]= data;
    }
    return true;
};
//bool queue_pop(struct textqueue* text_queue,char* data);
