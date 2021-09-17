/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#ifndef _FOTA_CONN_H_
#define _FOTA_CONN_H_

#include "../metadata/metadata.h"
#include "uds/uds_client.h"

extern void fota_conn_register(fota_conn_if_t *conn);
extern error_t fota_conn_upload(uint8_t *vin, uint8_t *primary_ecu_esrial, uint8_t* manifest_filename);
extern error_t fota_conn_dl(int8_t repo_server, const char *store_path, const char *fname);

extern error_t fota_conn_transfer_files(fota_ecu_t *target_ecu, uint32_t file_num, char file_path[][128], udsc_cb callback);
extern error_t fota_conn_cmd_idle(fota_ecu_t *target_ecu);
extern error_t fota_conn_cmd_process(fota_ecu_t *target_ecu);
extern error_t fota_conn_cmd_activate(fota_ecu_t *target_ecu);
extern error_t fota_conn_cmd_finish(fota_ecu_t *target_ecu);
extern error_t fota_conn_cmd_get_status(fota_ecu_t *target_ecu, uint8_t *status, uint8_t *progress);

#endif

