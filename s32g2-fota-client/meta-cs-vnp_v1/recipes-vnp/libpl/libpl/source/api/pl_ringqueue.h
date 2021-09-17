/*
 * Copyright 2018-2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause

 */
#pragma once
#define FILE__PL_RINGQUEUE_H
#include "pl_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct pl_ringqueue {
	uint16_t mask;
	uint16_t ring_w;
	uint16_t ring_r;
	uint16_t queue_w;
	uint16_t queue_r;
	uint16_t index[0];
};

#define RINGQUEUE_SIZE(n) ((long)&((struct pl_ringqueue*)0)->index[n])

static inline 
int pl_ringqueue_init(struct pl_ringqueue *rq)
{
	if (rq->mask & (rq->mask+1))
		return -1;
	rq->ring_w = rq->queue_r = 0;
	rq->queue_w = rq->queue_r = 0;
	return 0;
}

static inline
int pl_ringqueue_in(struct pl_ringqueue *rq, uint16_t index)
{
	if ((rq->queue_r + rq->mask +1 )== rq->ring_w)
		return -1;
	rq->index[rq->ring_w++ & rq->mask] = index;
	return index;
}

static inline
int pl_ringqueue_out(struct pl_ringqueue *rq)
{	
	if (rq->ring_r == rq->ring_w)
		return -1;

	return rq->index[rq->ring_r++ & rq->mask];
}

static inline
int pl_ringqueue_undo_out(struct pl_ringqueue *rq)
{	
	if (rq->ring_r == rq->queue_w)
		return -1;

	return rq->index[--rq->ring_r & rq->mask];
}


static inline
int pl_ringqueue_enqueue(struct pl_ringqueue *rq, uint16_t index)
{
	rq->index[rq->queue_w++ & rq->mask] = index;
	return index;
}

static inline
int pl_ringqueue_dequeue(struct pl_ringqueue *rq)
{
	if (rq->queue_r == rq->queue_w)
		return -1;

	return rq->index[rq->queue_r++ & rq->mask];
}

#ifdef __cplusplus
}
#endif
