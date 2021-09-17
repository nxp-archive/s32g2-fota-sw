/*
 * Copyright 2018-2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause

 */
#pragma once
#define FILE__PL_TIMER_H
#include "pl_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct pl_timer;

typedef void (*PL_TIMER_CALLBACK_T)(void *arg);

struct pl_timer {
	bool repeat;
	uint32_t period;
	PL_TIMER_CALLBACK_T func;
	void *arg;
	union {
		uint8_t sys_space[64];
		void *ptr;
	};
};

ret_t pl_add_timer(struct pl_timer *p_timer);

/**
 * pl_start_timer - startup timer 
 * @new_period: 0 - use old period  other to speiciafy a new period
 *
 * Returns Success (0) or -EINVAL
 */
ret_t pl_start_timer(struct pl_timer *p_timer, uint32_t new_period);
void pl_del_timer(struct pl_timer *p_timer);

#ifdef __cplusplus
}
#endif
