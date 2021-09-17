/*
 * Copyright 2018-2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause

 */
#pragma once
#define FILE__PL_EVENT_H
#include "pl_types.h"

#ifdef __cplusplus
extern "C" {
#endif
#define PL_EVENT_BASE \
	bool auto_clear;

struct pl_event {
	PL_EVENT_BASE
	uint8_t space[128];
};

ret_t pl_init_event(struct pl_event *event);
ret_t pl_set_event(struct pl_event *event, uint32_t val);
ret_t pl_wait_event(struct pl_event *event, uint32_t mask, uint32_t *val, bool exact);
void pl_destroy_event(struct pl_event *event);

#ifdef __cplusplus
}
#endif
