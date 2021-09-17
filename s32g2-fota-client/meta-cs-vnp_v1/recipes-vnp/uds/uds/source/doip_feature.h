/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#define DOIP_VERSION 0x2
#define DOIP_UDP_DISCOVERY_PORT 13400
#define DOIP_TCP_DATA_PORT 13400

enum {
	DOIP_PT_generic_neg_ack = 0x0000,
	DOIP_PT_vehicle_id_req  = 0x0001,
	DOIP_PT_vehicle_id_req_EID  = 0x0002,
	DOIP_PT_vehicle_id_req_VIN  = 0x0003,
	DOIP_PT_vehicle_id_rsp  	= 0x0004,
	DOIP_PT_routine_act_req  	= 0x0005,
	DOIP_PT_routine_act_rsp  	= 0x0006,
	DOIP_PT_alive_check_req  	= 0x0007,
	DOIP_PT_alive_check_rsp  	= 0x0008,

	DOIP_PT_entity_status_req  	= 0x4001,
	DOIP_PT_entity_status_rsp  	= 0x4002,
	DOIP_PT_diag_power_info_req  	= 0x4003,
	DOIP_PT_diag_power_info_rsp 	= 0x4004,

	DOIP_PT_diag_msg_req  	= 0x8001,
	DOIP_PT_diag_msg_pos_ack  	= 0x8002,
	DOIP_PT_diag_msg_neg_ack  	= 0x8003
};

struct doip_hdr {
	uint8_t ver;
	uint8_t inv_ver;
	uint16_t payload_type;
	uint32_t payload_length;
};

struct doip_pt_vehicle_id_rsp {
	uint8_t vin[17];
	uint8_t la[2];
	uint8_t eid[6];
	uint8_t gid[6];
	uint8_t further_action;
};

enum doip_routing_act_type {
	DOIP_ROUTINE_ACT_DEFAULT = 0x00,
	DOIP_ROUTINE_ACT_WWH_OBD = 0x01
};

struct doip_pt_routing_act_req {
	uint8_t sa[2];
	uint8_t act_type[2];
	uint8_t reserved[3];
};

struct doip_pt_routing_act_rsp {
	uint8_t req_la[2];
	uint8_t la[2];
	uint8_t rsp;
	uint8_t reserved[5];
};

#define DOIP_HDR_LEN 8
#define DOIP_SA_TA_LEN 4
#define DOIP_MTU 1400

#ifdef __cplusplus
}
#endif

