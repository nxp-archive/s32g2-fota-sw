/*
 * Copyright 2018-2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause

 */
#pragma once
#define FILE__PL_STDIO_H
#include "pl_types.h"
#include <stdio.h>

int pl_stat(const char *pathname, int *is_dir, int *size);

/**
 * @brief brief
 *
 * @param pathname name
 * @param mode 0=default
 * @return comment
 *   @retval value
 * @see link
 * @attention attention
 * @warning warning
 */
int pl_mkdir(const char *pathname, uint32_t mode);

int pl_deldir(const char *path);

int pl_rename(const char *oldpath, const char *newpath);
