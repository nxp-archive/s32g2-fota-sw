/*
 * Copyright 2018-2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause

 */
#include "pl_stdlib.h"

#if 0
void *pl_malloc_zero(uint32_t size)
{
	return calloc(1, size);
}

void pl_free(void *ptr)
{
	free(ptr);
}
#else
#include <string.h>
#include "FreeRTOS.h"

void *pl_malloc_zero(uint32_t size)
{
	void *p = pvPortMalloc(size);

	if (p)
		memset(p, 0, size);
	return p;
}

void pl_free(void *ptr)
{
	vPortFree(ptr);
}
#endif

