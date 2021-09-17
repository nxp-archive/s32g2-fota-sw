/*
 * Copyright 2018-2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause

 */
#include "pl_stdlib.h"
#include "pl_ringpool.h"

struct pl_ringpool *pl_ringpool_init(struct pl_ringpool *rp_ptr, 
					void *mem, uint16_t buffer_num, uint16_t buffer_size)
{
	struct pl_ringpool *rp = rp_ptr;
	uint16_t i;
	
	if (!buffer_num || !buffer_size)
		return NULL;

	if (buffer_num & (buffer_num-1))
		return NULL;
	
	if (!rp)
		rp = pl_malloc(RINGPOOL_SIZE(buffer_num));
	if (!mem)
		mem = pl_malloc((int)buffer_num * buffer_size);

	if (!rp || !mem)
		return NULL;

	rp->mem = mem;
	rp->buffer_num = buffer_num;
	rp->buffer_size = buffer_size;
	rp->rq.mask = buffer_num - 1;
	pl_ringqueue_init(&rp->rq);
	
	for (i = 0; i < buffer_num; i++)
		pl_ringqueue_in(&rp->rq, i);

	return rp;
}

void *pl_ringpool_alloc(struct pl_ringpool *rp)
{
	int index;

	if (!rp)
		return NULL;

	index = pl_ringqueue_out(&rp->rq);
	if (index < 0)
		return NULL;
	return rp->mem + index * rp->buffer_size;
}

int pl_ringpool_undo_alloc(struct pl_ringpool *rp)
{
	return pl_ringqueue_undo_out(&rp->rq);
}

int pl_ringpool_free(struct pl_ringpool *rp, void *ptr)
{
	unsigned long index;
	
	if (!rp)
		return -1;

	index = (ptr - rp->mem) / rp->buffer_size;
	if (index >= rp->buffer_num)
		return -2;
	
	return pl_ringqueue_in(&rp->rq, (uint16_t)index);
}

int pl_ringpool_enqueue(struct pl_ringpool *rp, void *ptr)
{
	unsigned long index;
	
	if (!rp)
		return -1;

	index = (ptr - rp->mem) / rp->buffer_size;
	if (index >= rp->buffer_num)
		return -2;
	return pl_ringqueue_enqueue(&rp->rq, (uint16_t)index);
}

void *pl_ringpool_dequeue(struct pl_ringpool *rp)
{
	int index;

	if (!rp)
		return NULL;

	index = pl_ringqueue_dequeue(&rp->rq);
	if (index < 0)
		return NULL;
	return rp->mem + index * rp->buffer_size;
}

