/*
 * Copyright 2018-2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause

 */
#include <pthread.h>
#include "pl_event.h"

struct posix_event {
	PL_EVENT_BASE
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	uint32_t event_val;
};

ret_t pl_init_event(struct pl_event *event)
{
	uint8_t check_size[sizeof(struct pl_event) - sizeof(struct posix_event)] __attribute__((unused));
	struct posix_event *pevent = (void*)event;

	if (!event)
		return -1;

	pevent->event_val = 0;
	pthread_mutex_init(&pevent->mutex, NULL);
	pthread_cond_init(&pevent->cond, NULL);
	return RET_OK;
}

ret_t pl_set_event(struct pl_event *event, uint32_t val)
{
	struct posix_event *pevent = (void*)event;
	
	pthread_mutex_lock(&pevent->mutex);
	
	pevent->event_val |= val;
	pthread_cond_signal(&pevent->cond);
	
	pthread_mutex_unlock(&pevent->mutex);
	return RET_OK;
}

#define EVENT_MATCH(x, mask, val, exact) ((exact) ? ((x) & (mask)) == (val) : (x) & (mask))

ret_t pl_wait_event(struct pl_event *event, uint32_t mask, uint32_t *val, bool exact)
{
	struct posix_event *pevent = (void*)event;
	uint32_t expect = *val;
	
	pthread_mutex_lock(&pevent->mutex);
	
	while (!EVENT_MATCH(pevent->event_val, mask, expect, exact))
		pthread_cond_wait(&pevent->cond, &pevent->mutex);
	*val = pevent->event_val & mask;
	
	if (pevent->auto_clear)
		pevent->event_val &= ~(*val);
	
	pthread_mutex_unlock(&pevent->mutex);
	return RET_OK;
}

void pl_destroy_event(struct pl_event *event)
{
	struct posix_event *pevent = (void*)event;
	
	pthread_mutex_destroy(&pevent->mutex);
	pthread_cond_destroy(&pevent->cond);
}

