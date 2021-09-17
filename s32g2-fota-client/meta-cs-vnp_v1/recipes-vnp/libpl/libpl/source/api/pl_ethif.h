/*
 * Copyright 2018-2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause

 */
#pragma once
#define FILE__PL_ETHIF_H
#include "pl_types.h"

#ifdef __cplusplus
extern "C" {
#endif
struct pl_ethif {
	uint32_t magic;
	struct dlist list;
	const char *name;
	int (*ops_tx)(struct pl_ethif *pif, uint8_t *packet, uint16_t lengt);
};

/* Call by driver */
int pl_reg_ethif(struct pl_ethif *pif);
int pl_unreg_ethif(struct pl_ethif *pif);
void pl_eth_receive_packet(struct pl_ethif *pif, uint8_t *packet, uint16_t length);

/* Call by application or stack */
int pl_ethif_send_packet(struct pl_ethif *pif, uint8_t *packet, uint16_t length);
int pl_ethif_set_mac(struct pl_ethif *pif, uint8_t mac[6]);
int pl_ethif_get_mac(struct pl_ethif *pif, uint8_t mac[6]);

#ifdef __cplusplus
}
#endif
