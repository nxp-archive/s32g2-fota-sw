/*
 * Copyright 2018-2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause

 */
#include "pl_dlist.h"
#include "pl_timer.h"
#include "pl_errno.h"
#include "FreeRTOS.h"
#include <timers.h>

struct freertos_timer {
	TimerHandle_t handle;
};

static void pl_timer_callback(TimerHandle_t xtimer)
{
	struct pl_timer *p_timer = pvTimerGetTimerID(xtimer);
	
	if (!p_timer->func)
		return;
	p_timer->func(p_timer->arg);
}

ret_t pl_add_timer(struct pl_timer *p_timer)
{
	struct freertos_timer *p_frt = (struct freertos_timer *)p_timer->sys_space;
	
	if (!p_timer)
		return -EINVAL;

	if (!p_timer->func || !p_timer->period)
		return -EINVAL;

	p_frt->handle = xTimerCreate("frt",
				pdMS_TO_TICKS(p_timer->period),
				p_timer->repeat ? pdTRUE : pdFALSE,
				p_timer,
				pl_timer_callback);
	if (!p_frt->handle)
		return -ENOMEM;	
	return RET_OK;
}

ret_t pl_start_timer(struct pl_timer *p_timer, uint32_t new_period)
{
	struct freertos_timer *p_frt = (struct freertos_timer *)p_timer->sys_space;
	
	if (!p_timer)
		return -EINVAL;

	if (!p_timer->func || !p_timer->period)
		return -EINVAL;

	if (!p_frt->handle)
		return -EINVAL;
	
	if (new_period && new_period != p_timer->period ) {
		p_timer->period = new_period;
		xTimerChangePeriod(p_frt->handle, 
			pdMS_TO_TICKS(p_timer->period),
			portMAX_DELAY);
	}
	return xTimerStart(p_frt->handle, portMAX_DELAY) == pdPASS ? RET_OK : -1;
}


void pl_del_timer(struct pl_timer *p_timer)
{
	struct freertos_timer *p_frt = (struct freertos_timer *)p_timer->sys_space;
	
	if (!p_timer)
		return;

	if (!p_frt->handle)
		return;
	
	xTimerDelete(p_frt->handle, portMAX_DELAY);
	p_frt->handle = 0;
}

