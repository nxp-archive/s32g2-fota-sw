/*
 * Copyright 2018-2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause

 */
#pragma once
#define FILE__PL_THREAD_H
#include "pl_types.h"

#ifdef __cplusplus
extern "C" {
#endif
ret_t pl_start_thread(
		uint8_t priority,
		uint32_t stackSize,
		void (*entry)(void *arg),
		void *arg
		);
#ifdef __cplusplus
}
#endif
