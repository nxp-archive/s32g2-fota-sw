/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#pragma once
#include "uds_tp.h"
#include "doip_feature.h"
#ifdef __cplusplus
extern "C" {
#endif

int doipc_init(void);

void doipc_uninit(void);

int doipc_get_ta_by_ip(uint32_t ip, uint16_t *ta);

struct uds_tp *doipc_open(uint16_t sa, uint32_t ip);

void doipc_close(struct uds_tp * tp);

int doipc_for_each_entity(uint32_t bc_ip, int (*callback)(uint32_t ip, struct doip_pt_vehicle_id_rsp *));

#ifdef __cplusplus
}
#endif
