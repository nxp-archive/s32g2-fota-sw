/*
 * Copyright 2018-2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause

 */
#pragma once
#define FILE__PL_RCU_H
#include <stdint.h>
#ifdef USING_OS_FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
#ifdef USING_OS_FREERTOS
#define pl_rcu_read_lock() vTaskSuspendAll()
#define pl_rcu_read_unlock() xTaskResumeAll()
#define pl_call_rcu(head, func) func(head)
#else
#define pl_rcu_read_lock()
#define pl_rcu_read_unlock()
#define pl_call_rcu(head, func)
#warning "rcu is not implemented!"
#endif
#ifdef __cplusplus
}
#endif
