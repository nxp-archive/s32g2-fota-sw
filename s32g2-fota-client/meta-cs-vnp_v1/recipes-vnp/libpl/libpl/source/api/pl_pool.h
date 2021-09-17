/*
 * Copyright 2018-2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause

 */
#pragma once
#define FILE__PL_POOL_H
#include "pl_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PL_BUFF_USE_FLAG 0xfe

struct pl_pool {
	uint16_t buffer_num;
	uint16_t buffer_size;
	uint8_t header_size;
	uint8_t tailer_size;
	void *data;
	uint16_t free_head;
	uint16_t *chain;
};

static inline int pl_pool_init(struct pl_pool *pool)
{
	uint32_t i;

	for (i = 0; i < pool->buffer_num; i++)
		pool->chain[i] = i+1;
	pool->chain[pool->buffer_num - 1] = 0;
	pool->free_head = 1;
	return 0;
}

static inline void *pl_pool_alloc(struct pl_pool *pool)
{	
	if (pool->free_head) {
		void *ret = pool->data + (pool->free_head -1)* pool->buffer_size;
		
		pool->free_head = pool->chain[pool->free_head - 1];
		if (pool->header_size)
			(uint8_t*)ret = PL_BUFF_USE_FLAG;
		if (pool->tailer_size)
			(uint8_t*)(ret + pool->buffer_size - pool->tailer_size) = PL_BUFF_USE_FLAG;
		return ret + pool->header_size;
	}
	return NULL;
}

static inline int pl_pool_free(struct pl_pool *pool, void *data)
{
	uint32_t pos;

	if (!data)
		return 0;

	pos = (data - pool->data) / pool->buffer_size;
	
	if (pool->header_size + pool->tailer_size) {
		void *align_data = pool->data + pos * pool->buffer_size;

		if (pool->header_size && (uint8_t*)align_data != PL_BUFF_USE_FLAG)
			return -1;
		if (pool->tailer_size 
			&& (uint8_t*)(align_data + pool->buffer_size - pool->tailer_size) != PL_BUFF_USE_FLAG)
			return -2;
	}
	pool->chain[pos] = pool->free_head;
	pool->free_head = pos + 1;
	return 0;
}

static inline uint16_t pl_pool_b2i(struct pl_pool *pool, void *buff)
{
	return (buff - pool->data) / pool->buffer_size;
}

static inline uint16_t pl_pool_i2b(struct pl_pool *pool, uint16_t index)
{
	return pool->data +  (uint32_t)index * pool->buffer_size;
}

#ifdef __cplusplus
}
#endif
