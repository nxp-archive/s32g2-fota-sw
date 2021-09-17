/*
 * Copyright 2018-2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause

 */
#pragma once
#define FILE__PL_TYPES_H
#include <stdint.h>
#include <stdbool.h>
#include "pl_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RET_OK 0
typedef int32_t ret_t;
typedef uint32_t atomic_t;

#ifdef __LP64__
#define PR64 "l"
#else
#define PR64 "ll"
#endif

#ifndef offsetof
#define offsetof(type, member)	__builtin_offsetof(type, member)
#endif
#define container_of(ptr, type, member) ((type*)((void*)ptr - offsetof(type, member)))

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

struct pl_ref {
	atomic_t ref_cnt;
	void (*release)(struct pl_ref *);
};

#if !defined(__BYTE_ORDER__)
#define __ORDER_LITTLE_ENDIAN__ 1234
#define __ORDER_BIG_ENDIAN__    4321
#define __BYTE_ORDER__ __ORDER_LITTLE_ENDIAN__
#endif

static inline void atomic_set(atomic_t *ptr, atomic_t val)
{
	*ptr = val;
}

static inline void atomic_inc(atomic_t *ptr)
{
#ifdef __GNUC__
	__sync_fetch_and_add(ptr, 1);
#else
	(*ptr)++;
#endif
}

static inline void atomic_dec(atomic_t *ptr)
{
#ifdef __GNUC__
		__sync_fetch_and_sub(ptr, 1);
#else
	(*ptr)--;
#endif
}

static inline atomic_t atomic_inc_return(atomic_t *ptr)
{
#ifdef __GNUC__
	return __sync_add_and_fetch(ptr, 1);
#else
	return ++(*ptr);
#endif
}

static inline atomic_t atomic_dec_return(atomic_t *ptr)
{
#ifdef __GNUC__
	return __sync_sub_and_fetch(ptr, 1);
#else
	return --(*ptr);
#endif
}


static inline void pl_ref_init(struct pl_ref *pthis)
{
	atomic_set(&pthis->ref_cnt, 1);
	pthis->release = 0;
}

static inline void pl_ref_get(struct pl_ref *pthis)
{
	atomic_inc(&pthis->ref_cnt);
}

static inline int pl_ref_put(struct pl_ref *pthis)
{
	if (!atomic_dec_return(&pthis->ref_cnt)){
		if (pthis->release)
			pthis->release(pthis);
		return 1;
	}
	return 0;
}

#define pl_assert(x)
#ifdef __cplusplus
}
#endif

