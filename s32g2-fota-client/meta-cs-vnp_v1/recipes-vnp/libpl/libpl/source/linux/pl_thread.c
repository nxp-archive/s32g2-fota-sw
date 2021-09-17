/*
 * Copyright 2018-2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause

 */
#include "pl_thread.h"
#include "pl_stdlib.h"
#include <pthread.h>

struct pl_thread_info {
	pthread_t thread_id;
	void (*entry)(void *arg);
	void *arg;
};

static void *pthread_entry(void *arg)
{
	struct pl_thread_info *thread = arg;
	
	thread->entry(thread->arg);
	pl_free(arg);
	return NULL;
}

ret_t pl_start_thread(
		uint8_t priority,
		uint32_t stackSize,
		void (*entry)(void *arg),
		void *arg
		)
{
	int ret;
	pthread_attr_t attr;
	struct pl_thread_info *thread;

	if (!entry)
		return -EINVAL;
	
	ret= pthread_attr_init(&attr);
	if (ret != 0)
		return ret;

	if (stackSize > 0) {
		ret = pthread_attr_setstacksize(&attr, stackSize);
		if (ret)
			return ret;
	}

	thread = pl_malloc_zero(sizeof(*thread));
	if (!thread) {
		ret = -ENOMEM;
		goto EXIT1;
	}
	
	thread->entry = entry;
	thread->arg = arg;
	ret = pthread_create(&thread->thread_id, &attr,
		&pthread_entry, thread);
	if (ret)
		pl_free(thread);
	
EXIT1:	
	pthread_attr_destroy(&attr);
	return ret;
}

