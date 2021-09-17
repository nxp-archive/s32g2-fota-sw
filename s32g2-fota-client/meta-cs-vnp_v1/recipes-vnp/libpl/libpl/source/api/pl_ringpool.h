/*
 * Copyright 2018-2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause

 */
#pragma once
#define FILE__PL_RINGPOOL_H
#include "pl_ringqueue.h"

#ifdef __cplusplus
extern "C" {
#endif

struct pl_ringpool {
	void *mem;
	uint16_t buffer_num;
	uint16_t buffer_size;
	struct pl_ringqueue rq;
};

#define RINGPOOL_SIZE(n) ((long)&((struct pl_ringpool*)0)->rq.index[n])

struct pl_ringpool *pl_ringpool_init(struct pl_ringpool *rp, 
					void *mem, uint16_t buffer_num, uint16_t buffer_size);

void *pl_ringpool_alloc(struct pl_ringpool *rp);

int pl_ringpool_undo_alloc(struct pl_ringpool *rp);

int pl_ringpool_free(struct pl_ringpool *rp, void *ptr);

int pl_ringpool_enqueue(struct pl_ringpool *rp, void *ptr);

void *pl_ringpool_dequeue(struct pl_ringpool *rp);

#ifdef __cplusplus
}
#endif
