/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/

/* commands defines */
#ifndef _FOTA_UDS_H_
#define _FOTA_UDS_H_

#include "uds/uds_client.h"

extern int uds_client_file_op(const char *server_ip, char file_op, int file_num, char file_path[][128], udsc_cb callback);
extern int uds_client_cmd_set(const char *server_ip, char op);
extern int uds_client_get_state(const char *server_ip, uint8_t *err, uint8_t *state, uint8_t *progress);

error_t uds_read_ecu_manifest(const char *path, uint32_t addr, uint32_t addr_ext);
error_t uds_transfer_files(uint32_t addr, uint32_t addr_ext, uint32_t file_num, char file_path[][128], udsc_cb callback);
error_t uds_cmd_process(uint32_t addr, uint32_t addr_ext);
error_t uds_cmd_activate(uint32_t addr, uint32_t addr_ext);
error_t uds_cmd_get_ecu_status(uint32_t addr, uint32_t addr_ext, uint8_t *status, uint8_t *progress);
error_t uds_cmd_finish(uint32_t addr, uint32_t addr_ext);
error_t uds_cmd_cancel(uint32_t addr, uint32_t addr_ext);
error_t uds_cmd_rollback(uint32_t addr, uint32_t addr_ext);
error_t uds_cmd_idle(uint32_t addr, uint32_t addr_ext);

#endif

