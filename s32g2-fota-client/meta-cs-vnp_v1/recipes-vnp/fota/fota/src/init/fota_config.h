/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#ifndef _FACTORY_H_
#define _FACTORY_H_

#include "../campaign/campaign.h"

typedef struct {
	char irs_url[4][LENGTH_REPO_URL];
	char drs_url[4][LENGTH_REPO_URL];
} remote_repo_cfg_t;

typedef struct {
	char vin[LENGTH_ECU_SERIAL];
	remote_repo_cfg_t repo_addr;
	fota_ecu_t *primary;	/* only one primary */
	fota_ecu_t *secondary;
    int32_t checking_interval;
} factory_cfg_t;

extern error_t load_factory_config(void);
extern factory_cfg_t *get_factory_cfg(void);
extern int8_t *get_factory_vin(void);
extern int8_t *get_factory_primary_serial(void);
extern fota_ecu_t *get_ecu_list(void);
extern int8_t *get_factory_dr_url(int32_t index);
extern int8_t *get_factory_ir_url(int32_t index);
char * factory_get_config_path(void);
 
 #endif