/*
 * Copyright 2018-2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause

 */
#pragma once
#define FILE__PL_STRING_H
#include "pl_types.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
char *pl_strcpy_s(char *dest, size_t dest_size, const char *src, bool *p_trunc);

char *pl_strncpy_s(char *dest, size_t dest_size, const char *src, size_t n, bool *p_trunc);

#ifdef __cplusplus
}
#endif
