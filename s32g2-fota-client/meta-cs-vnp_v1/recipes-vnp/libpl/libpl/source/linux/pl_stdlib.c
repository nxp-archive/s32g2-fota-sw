/*
 * Copyright 2018-2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause

 */
#include <stdio.h>
#include "pl_stdlib.h"

#if 1
void *pl_malloc_zero(uint32_t size)
{
	return calloc(1, size);
}

void *pl_malloc(uint32_t size)
{
	return malloc(size);
}

void pl_free(void *ptr)
{
	free(ptr);
}

#define MODULE_MESSAGE "[pl]: "

int pl_system(const char *cmd)
{
	int ret;

	printf(MODULE_MESSAGE "system(%s)\n", cmd);
	
	ret = system(cmd);
	if (ret < 0) {
		fprintf(stderr, MODULE_MESSAGE "system() fail\n");
		return -1;
	}
	
	if (WIFEXITED(ret)) {
		ret = WEXITSTATUS(ret);
		printf(MODULE_MESSAGE "exit code %d\n", ret);
	} else
		ret = -2; 
	return ret;
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

