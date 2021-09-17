/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/

/*
 * Error code definitions
 */
#ifndef _FOTAV_H
#define _FOTAV_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "config.h"

#define VERSION_NUM_MAIN	0
#define VERSION_NUM_MINOR	2
#define CUR_VERSION		(((VERSION_NUM_MAIN * 256) + VERSION_NUM_MINOR))

#define IS_DEBUG 1


#define LOG_DEBUG "DEBUG"  
#define LOG_TRACE "TRACE"  
#define LOG_ERROR "ERROR"  
#define LOG_INFO  "INFOR"  
#define LOG_CRIT  "CRTCL"  
 
#define MY_PRINTF(level, format, ...) \
    do { \
        fprintf(stderr, "[%s|%s()@%s,%d] " format "\n", \
            level, __func__, __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#if UNIT_TEST > 0
	#define printf_ut(level, format, ...)	MY_PRINTF(level, format, ##__VA_ARGS__)
#else
	#define printf_ut(level, format, ...) 	
#endif

#if DEBUG_ > 0
	#define printf_dbg(level, format, ...)	MY_PRINTF(level, format, ##__VA_ARGS__)
#else
	#define printf_dbg(level, format, ...)
#endif

	#define printf_it(level, format, ...)	MY_PRINTF(level, format, ##__VA_ARGS__)

typedef int16_t error_t;


#define ERR_OK		((int16_t)(0x0001))
#define ERR_NOK		((int16_t)(0xFFFF))
#define ERR_NEG		((uint16_t)(0x8000))
#define ERR_MODULE_HTTP	(0x0100)
#define ERR_CODE(mod, code) ((int16_t)(ERR_NEG | mod | code))

enum {
	E_ERR_CODE_NULL_PTR = 1,
	E_ERR_CODE_SOCK_NULL,
	E_ERR_CODE_HOST_ADDR,
	E_ERR_CODE_CONNECT,
	E_ERR_CODE_HTTP_RESP_LEN,
	E_ERR_CODE_HTTP_READ_MORE,
};

/* HTTP client */
typedef void (*store_cb)(const int8_t *ptr, int32_t size, int32_t acc_size, int32_t finish, int8_t *param);
extern error_t http_rpc_method(const int8_t *server_url, store_cb cust_store, int8_t *cust_param, const int8_t *rpc_method, ...);
extern error_t http_download(const int8_t *server_url, store_cb cust_store, int8_t *param);
extern error_t http_upload(const int8_t *url, const int8_t *filename, int8_t *data, int32_t size);

/* communication */

/* system initialization */
extern error_t fota_system_init(int32_t *registered);

/* factory */
#if (FACTORY_CFG_UDS == OFF)
void factory_set_config_path(const int8_t *path);
char * factory_get_config_path(void);
#endif

/* repo */
extern int8_t* repo_get_manifest_path(void);
extern int8_t *repo_get_irs_url(int32_t mirror);
extern int8_t *repo_get_drs_url(int32_t mirror);
extern int8_t *repo_get_vin(void);

/* ota update cycle */
typedef int32_t (* get_rm_upload_url)(const int8_t *vin, int8_t *url);
typedef int32_t (* get_rm_download_url)(const int8_t *vin, int8_t *url);
typedef int32_t (* upload_file_to)(const int8_t *url, const int8_t *rname, const int8_t *path);
typedef void (*downloaf_file_cb)(uint8_t progress);
typedef int32_t (* download_file_from)(const int8_t *url, const int8_t *store_path, downloaf_file_cb dl_cb);

typedef struct {
	get_rm_upload_url get_repo_ul_url;
	get_rm_download_url get_repo_dl_url;
	upload_file_to	repo_upload;
	download_file_from repo_download;

} fota_conn_if_t;

typedef struct ecu_status {
	struct ecu_status *next;
	int8_t *serial;
	int32_t status;
} ecu_status_t;

typedef struct {
	error_t code;
	ecu_status_t *ecu_status;
} update_indication_t;

/* package module */
/* for test framework */
error_t pkg_restore_factory_fw(void);
error_t pkg_restore_factory_fw_confirm(void);



#endif

