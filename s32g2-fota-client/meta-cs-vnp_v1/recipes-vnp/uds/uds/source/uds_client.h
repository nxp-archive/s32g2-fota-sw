/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#pragma once
#include "uds_tp.h"

#include "uds_feature.h"
#include "uds_file.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*udsc_cb)(const char *fname, int progress);

int udsc_init(void);

void udsc_uninit(void);

struct udsc *udsc_open(struct uds_tp *tp);

void udsc_close(struct udsc *uds);

int udsc_do_start_routine(struct udsc *uds, enum UDS_RoutineID rid,
						const uint8_t *opt, uint16_t opt_len,
						uint8_t *output, uint16_t *output_len);

int udsc_add_file(struct udsc *uds, struct uds_file *file, udsc_cb callback);

int udsc_replace_file(struct udsc *uds, struct uds_file *file, udsc_cb callback);

int udsc_delete_file(struct udsc *uds, const char *filename);

int udsc_reset_ecu(struct udsc *uds, enum ECUReset_resetType type);

#ifdef __cplusplus
}
#endif
