/*
 * Copyright 2018-2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause

 */
#pragma once
#define FILE__PL_STDLIB_H
#include "pl_types.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif
void *pl_malloc_zero(uint32_t size);
void *pl_malloc(uint32_t size);
void pl_free(void *ptr);
int pl_system(const char *cmd);

#ifdef __cplusplus
}
#endif
