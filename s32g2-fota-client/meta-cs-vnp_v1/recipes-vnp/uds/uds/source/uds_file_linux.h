/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#pragma once

#include "uds_file.h"
#ifdef __cplusplus
extern "C" {
#endif

struct uds_file *linux_open_uds_file(const char *path, UDS_FILE_FLAG flag);

void linux_close_uds_file(struct uds_file *file);

#ifdef __cplusplus
}
#endif

