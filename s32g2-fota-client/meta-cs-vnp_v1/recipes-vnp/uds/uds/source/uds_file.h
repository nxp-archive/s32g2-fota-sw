/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	UDS_FILE_FLAG_READ,
	UDS_FILE_FLAG_WRITE,
	UDS_FILE_FLAG_DIR
} UDS_FILE_FLAG;

struct uds_file {
	const char *name;
	uint32_t uncompressed_size;
	uint32_t compressed_size;
	int (*read)(struct uds_file *file, void *buf, int len);
	int (*write)(struct uds_file *file, void *buf, int len);
	uint32_t crc;
};

#ifdef __cplusplus
}
#endif
