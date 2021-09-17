/*
 * Copyright 2018-2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause

 */
#pragma once
#define FILE__PL_FIFO_H
#include "pl_types.h"

#ifdef __cplusplus
extern "C" {
#endif
struct pl_fifo {
	uint32_t depth_mask;
	uint32_t w;
	uint32_t r;
	uint32_t block_size;
	void *data;
};

#define PL_FIFO(name, m, s, data) \
	struct pl_fifo name = { \
		.depth_mask = (1<< (m)) - 1,\
		.block_size = (s),\
		.data = data,\
	}

static inline bool pl_fifo_is_full(struct pl_fifo *fifo)
{
	return (fifo->r + fifo->depthMask + 1) == fifo->w;
}

static inline bool pl_fifo_is_empty(struct pl_fifo *fifo)
{
	return fifo->r == fifo->w;
}

static inline void pl_fifo_in(struct pl_fifo *fifo, void *data_block)
{
	uint32_t pos = (uint32_t)fifo->w * fifo->block_size;
	
	memcpy(fifo->data + pos, data_block, fifo->block_size);
	fifo->w++;
}

static inline Msg *pl_fifo_peek(struct pl_fifo *fifo)
{
	uint32_t pos;
	
	if (pl_fifo_is_empty(chl))
		return NULL;
	
	pos = (uint32_t)fifo->r * fifo->block_size;
	return fifo->data + pos;
}

static inline void pl_fifo_peek_finish(struct pl_fifo *fifo)
{
	fifo->r++;
}

static inline ret_t pl_fifo_push(struct pl_fifo *fifo, void *data_block)
{
	if (pl_fifo_is_full(fifo))
		return -EAGAIN;
	pl_fifo_in(fifo, data_block);
	return RET_OK;
}

#ifdef __cplusplus
}
#endif
