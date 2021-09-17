/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#include "pl_types.h"
#include "pl_tcpip.h"
#include "pl_stdlib.h"
#include "pl_string.h"
#include "pl_stdio.h"
#include "pl_stdarg.h"

#include "../fotav.h"
#include "../repo/repo.h"
#include "../json/cJSON.h"
#include "../campaign/campaign.h"
#include "fota_uds.h"

#define TRANS_CHUNK_SIZE (1280)

/*
    path: local storage path (with file name)
    addr: remote address
    addr_ext: remote address extended
*/
error_t uds_read_ecu_manifest(const char *path, uint32_t addr, uint32_t addr_ext)
{
    return ERR_OK;
}

error_t uds_transfer_files(uint32_t addr, uint32_t addr_ext, uint32_t file_num, char file_path[][LENGTH_DIR_PATH], udsc_cb callback)
{
#if 0
	uint8_t cmd_buf[1024];

	snprintf(cmd_buf, sizeof(cmd_buf), "uds -c %s -r %s", pl_inet_ntoa(addr), path);

	if (pl_system(cmd_buf) < 0)
        return ERR_NOK;
#else
    char ip_string[32];
    int i;
    
    snprintf(ip_string, 32, "%s", pl_inet_ntoa(addr));

    for (i = 0; i < file_num; i++) {
        printf_it(LOG_DEBUG, "%s", file_path[i]);
    }
    
    if (uds_client_file_op(ip_string, 'r', file_num, file_path, callback))
        return ERR_NOK;
#endif
    return ERR_OK;
}

error_t uds_cmd_idle(uint32_t addr, uint32_t addr_ext)
{
#if 0
    uint8_t cmd_buf[128];

    snprintf(cmd_buf, sizeof(cmd_buf), "uds -c %s -i", pl_inet_ntoa(addr));
    
	if (pl_system(cmd_buf) < 0)
        return ERR_NOK;
#else
    char ip_string[32];
    snprintf(ip_string, 32, "%s", pl_inet_ntoa(addr));

    if (uds_client_cmd_set(ip_string, 'c'))
        return ERR_NOK;

#endif
    return ERR_OK;
}


error_t uds_cmd_process(uint32_t addr, uint32_t addr_ext)
{
#if 0
    uint8_t cmd_buf[128];

    snprintf(cmd_buf, sizeof(cmd_buf), "uds -c %s -i", pl_inet_ntoa(addr));
    
	if (pl_system(cmd_buf) < 0)
        return ERR_NOK;
#else
    char ip_string[32];
    snprintf(ip_string, 32, "%s", pl_inet_ntoa(addr));

    if (uds_client_cmd_set(ip_string, 'i'))
        return ERR_NOK;

#endif
    return ERR_OK;
}

error_t uds_cmd_get_ecu_status(uint32_t addr, uint32_t addr_ext, uint8_t *status, uint8_t *progress)
{
#if 0
    int32_t err_code = 0;
    uint8_t cmd_buf[128];

	if (progress != NULL ) *progress = 0;
    if (status != NULL) *status = SECU_STATUS_IDLE;

    snprintf(cmd_buf, sizeof(cmd_buf), "uds -c %s -s >/tmp/uds.out", pl_inet_ntoa(addr));

    if (pl_system(cmd_buf) < 0)
        return ERR_NOK;
    
	FILE *fp = fopen("/tmp/uds.out", "r");
	
	if (fp) {
		char buf[128];

		fgets(buf, sizeof(buf), fp);
		fclose(fp);
		sscanf(buf, "%u %u %u", &err_code, status, progress);

        printf_dbg(LOG_INFO, "err_code=%d, status=%d, progress=%d", err_code, *status, *progress);
	}
#else
    uint8_t err;
    char ip_string[32];
    snprintf(ip_string, 32, "%s", pl_inet_ntoa(addr));

	if (progress != NULL ) *progress = 0;
    if (status != NULL) *status = SECU_STATUS_IDLE;

    if (uds_client_get_state(ip_string, &err, status, progress))
        return ERR_NOK;

    printf_dbg(LOG_INFO, "err_code=%d, status=%d, progress=%d", err, *status, *progress);

#endif
    return ERR_OK;
}

error_t uds_cmd_activate(uint32_t addr, uint32_t addr_ext)
{
#if 0
    uint8_t cmd_buf[128];

    snprintf(cmd_buf, sizeof(cmd_buf), "uds -c %s -e", pl_inet_ntoa(addr));
    
    if (pl_system(cmd_buf) < 0)
        return ERR_NOK;
#else
    char ip_string[32];
    snprintf(ip_string, 32, "%s", pl_inet_ntoa(addr));

    if (uds_client_cmd_set(ip_string, 'e'))
        return ERR_NOK;
    
#endif

	return ERR_OK;
}

error_t uds_cmd_finish(uint32_t addr, uint32_t addr_ext)
{
#if 0
    uint8_t cmd_buf[128];

    snprintf(cmd_buf, sizeof(cmd_buf), "uds -c %s -f", pl_inet_ntoa(addr));
    
    if (pl_system(cmd_buf) < 0)
        return ERR_NOK;
#else
    char ip_string[32];
    snprintf(ip_string, 32, "%s", pl_inet_ntoa(addr));

    if (uds_client_cmd_set(ip_string, 'f'))
        return ERR_NOK;
    
#endif

    return ERR_OK;
}

error_t uds_cmd_cancel(uint32_t addr, uint32_t addr_ext)
{
    return ERR_OK;
}

error_t uds_cmd_rollback(uint32_t addr, uint32_t addr_ext)
{
    return ERR_OK;
}

