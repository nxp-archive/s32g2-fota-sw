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

enum uds_tp_type {
	UDS_TP_IP,
	UDS_TP_CAN,
	UDS_TP_LIN
};

struct tp_buff {
	uint8_t *head;
	uint8_t *end;
	uint8_t *data;
	uint16_t len;
	uint16_t sa;
	uint16_t ta;
	struct uds_tp *tp;
};

static inline void *tpb_push(struct tp_buff *tpb, uint16_t len)
{
	tpb->data -= len;
	tpb->len  += len;
	return tpb->data;
}

static inline void *tpb_pull(struct tp_buff *tpb, uint16_t len)
{
	tpb->data += len;
	tpb->len  -= len;
	return tpb->data;
}

static inline void tpb_reserve(struct tp_buff *tpb, uint16_t len)
{
	tpb->data += len;
}
static inline void *tpb_put(struct tp_buff *tpb, uint16_t len)
{
	void *tmp = tpb->data + tpb->len;

	tpb->len += len;
	return tmp;
}

typedef enum {
	TP_TX,
	TP_RX,
} TP_DIR;

struct uds_tp_ops {
	struct tp_buff *(*alloc)(struct uds_tp *tp, TP_DIR dir);
	void (*free)(struct uds_tp *tp, struct tp_buff *tpb);
	int (*activate)(struct uds_tp *tp);
	int (*send)(struct uds_tp *tp, struct tp_buff *tpb);
	int (*recv)(struct uds_tp *tp, struct tp_buff *tpb);
};

struct uds_tp {
	int mtu;
	enum uds_tp_type type;
	struct uds_tp_ops *ops;
	void (*rx_callback)(struct uds_tp *tp, struct tp_buff *tpb);
};

#ifdef __cplusplus
}
#endif
