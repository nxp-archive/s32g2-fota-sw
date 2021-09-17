/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#include "pl_types.h"
#include "pl_stdlib.h"
#include "pl_string.h"
#include "pl_stdio.h"

#include "../fotav.h"
#include "../json/cJSON.h"
#include "../json/json_file.h"
#include "fota_config.h"

#if (PRIMARY_ECU > 0)
factory_cfg_t g_factory_cfg;
#endif

#if (FACTORY_CFG_UDS == OFF)
char g_factory_config_path_setting[LENGTH_DIR_PATH] = {0};

void factory_set_config_path(const int8_t *path)
{
	snprintf(g_factory_config_path_setting, LENGTH_DIR_PATH, "%s", path);
}

char * factory_get_config_path(void) 
{
	return &g_factory_config_path_setting[0];
}
#endif



/* for test purpose */
static error_t parse_config(const int8_t *cfg_file_path, factory_cfg_t *pcfg)
{
    cJSON *root = NULL;
    cJSON *factory = NULL;
    cJSON *item = NULL;
    cJSON *sub_item = NULL, *subsub=NULL;
    int32_t cnt = 0;
    fota_ecu_t *ecu_list;
    fota_ecu_t *tmp_item;

    if ((root = json_file_parse(cfg_file_path)) == NULL) {
        printf_dbg(LOG_ERROR, "parse config file error");
        return ERR_NOK;
    }

    if ((factory = cJSON_GetObjectItem(root, "factory")) != NULL) {
        if ((item = cJSON_GetObjectItem(factory, "vin")) != NULL) {
            if (cJSON_IsString(item))
                strncpy(pcfg->vin, item->valuestring, LENGTH_ECU_SERIAL);
        } else {
            printf_dbg(LOG_ERROR, "can not find 'vin' in configuration");
            goto ERROR_RETURN;
        }

        /* IRS url */
        if ((item = cJSON_GetObjectItem(factory, "irs_url_list")) != NULL) {
            cnt = 0;
            cJSON_ArrayForEach(sub_item, item) {
                strncpy(pcfg->repo_addr.irs_url[cnt], sub_item->valuestring, LENGTH_ECU_SERIAL);                
                cnt++;
            }
        } else {
            printf_dbg(LOG_ERROR, "can not find 'irs_url_list' in configuration");
            goto ERROR_RETURN;
        }
        /* DRS url */
        if ((item = cJSON_GetObjectItem(factory, "drs_url_list")) != NULL) {
            cnt = 0;
            cJSON_ArrayForEach(sub_item, item) {
                strncpy(pcfg->repo_addr.drs_url[cnt], sub_item->valuestring, LENGTH_ECU_SERIAL);
                cnt++;
            }
        } else {
            printf_dbg(LOG_ERROR, "can not find 'drs_url_list' in configuration");
            goto ERROR_RETURN;
        }

        /* primary */
        if ((item = cJSON_GetObjectItem(factory, "primary_ecu")) != NULL) {
            if ((tmp_item = (fota_ecu_t *)pl_malloc_zero(sizeof(fota_ecu_t))) != NULL) {
                pcfg->primary = tmp_item;
                ecu_list = pcfg->primary;
            } else {
                printf_dbg(LOG_ERROR, "memory alloc fail");
                goto ERROR_RETURN;
            }
            
            cJSON_ArrayForEach(sub_item, item) {
                if (strcmp(sub_item->string, "ecu_hw_id") == 0) {
                    strncpy(tmp_item->hw_id, sub_item->valuestring, LENGTH_HW_ID);
                }              
                else if (strcmp(sub_item->string, "initial_fw_version") == 0) {
                    tmp_item->factory.version_major = (int8_t)sub_item->valuedouble;
                } 
                else if (strcmp(sub_item->string, "initial_fw_release_counter") == 0) {
                    tmp_item->fw_install_cnt = (int32_t)sub_item->valuedouble;
                } 
                else if (strcmp(sub_item->string, "net_addr") == 0) {
                    tmp_item->addr = (uint32_t)sub_item->valuedouble;
                } 
                else if (strcmp(sub_item->string, "net_addr_ext") == 0) {
                    tmp_item->addr_ext = (uint32_t)sub_item->valuedouble;
                } 
                else if (strcmp(sub_item->string, "initial_fw_image_file_name") == 0) {
                    strcpy(tmp_item->factory.fname, sub_item->valuestring);
                } 
                else if (strcmp(sub_item->string, "net_protocal") == 0) {
                    if (strcmp(sub_item->valuestring, "uds") == 0)
                        tmp_item->protocal = PROTOCAL_UDS;
                    else if (strcmp(sub_item->valuestring, "scp") == 0)
                        tmp_item->protocal = PROTOCAL_SCP;
                }

                tmp_item->factory.ecu_target = (void *)tmp_item;
                tmp_item->is_primary = 1;
            }
        } 
        else {
            printf_dbg(LOG_ERROR, "can not find 'primary_ecu' in configuration");
            goto ERROR_RETURN;
        }

        /* secondary list */
        if ((item = cJSON_GetObjectItem(factory, "secondary_ecu")) != NULL) {
            pcfg->secondary = NULL;
            cJSON_ArrayForEach(sub_item, item) {

                if ((tmp_item = (fota_ecu_t *)pl_malloc_zero(sizeof(fota_ecu_t))) != NULL) {

                    if (pcfg->secondary == NULL) {
                        pcfg->secondary = tmp_item;
                    }
                    ecu_list->next = tmp_item;
                    ecu_list = tmp_item;

                    tmp_item->is_primary = 0;
                } else {
                    printf_dbg(LOG_ERROR, "memory alloc fail");
                    goto ERROR_RETURN;
                }

                tmp_item->factory.ecu_target = (void *)tmp_item;

                cJSON_ArrayForEach(subsub, sub_item) {
                    if (strcmp(subsub->string, "ecu_hw_id") == 0) {
                        strncpy(tmp_item->hw_id, subsub->valuestring, LENGTH_HW_ID);
                    } 
                    else if (strcmp(subsub->string, "initial_fw_version") == 0) {
                        tmp_item->factory.version_major = (int32_t)subsub->valuedouble;
                    } 
                    else if (strcmp(subsub->string, "initial_fw_release_counter") == 0) {
                        tmp_item->fw_install_cnt = (int32_t)subsub->valuedouble;
                    } 
                    else if (strcmp(subsub->string, "net_addr") == 0) {
                        tmp_item->addr = (uint32_t)subsub->valuedouble;
                    } 
                    else if (strcmp(subsub->string, "net_addr_ext") == 0) {
                        tmp_item->addr_ext = (uint32_t)subsub->valuedouble;
                    } 
                    else if (strcmp(subsub->string, "initial_fw_image_file_name") == 0) {
                        strcpy(tmp_item->factory.fname, subsub->valuestring);
                    } 
                    else if (strcmp(subsub->string, "net_protocal") == 0) {
                        if (strcmp(subsub->valuestring, "uds") == 0)
                            tmp_item->protocal = PROTOCAL_UDS;
                        else if (strcmp(subsub->valuestring, "scp") == 0)
                            tmp_item->protocal = PROTOCAL_SCP;
                    }

                }
            }
        } 
        else {
            printf_dbg(LOG_ERROR, "can not find 'secondary_ecu' in configuration");
            goto ERROR_RETURN;
        }

    } else {
        printf_dbg(LOG_ERROR, "can not find 'factory' in configuration");
        goto ERROR_RETURN;
    }

    /* genaral cfg parameters */
    if ((item = cJSON_GetObjectItem(root, "general")) != NULL) {
        cJSON_ArrayForEach(sub_item, item) {
            if (strcmp(sub_item->string, "checking_interval") == 0) {
                pcfg->checking_interval = (int32_t)sub_item->valuedouble;
            }
        }
    } else {
        printf_dbg(LOG_ERROR, "can not find 'general' in configuration");
        goto ERROR_RETURN;
    }

    json_file_parse_end(root);

    return ERR_OK;

ERROR_RETURN:
    json_file_parse_end(root);
    return ERR_NOK;
}

error_t load_factory_config(void)
{
	fota_ecu_t *ecu;
	int8_t path[128];

	memset((void *)&g_factory_cfg, 0, sizeof(factory_cfg_t));

	snprintf(path, 128, "%sconfig.json", factory_get_config_path());
	parse_config(path, &g_factory_cfg);

    printf_it(LOG_INFO, "vin: %s", g_factory_cfg.vin);
	printf_it(LOG_INFO, "factory ecu list:");
	for (ecu = g_factory_cfg.primary; ecu != NULL; ecu = ecu->next) {
		printf_it(LOG_INFO, "  -> %s", ecu->hw_id);
	}

	return ERR_OK;
}

factory_cfg_t *get_factory_cfg(void)
{
	return &g_factory_cfg;
}

int8_t *get_factory_primary_serial(void)
{
	return g_factory_cfg.primary->hw_id;
}

int8_t *get_factory_vin(void)
{
	return g_factory_cfg.vin;
}

fota_ecu_t *get_ecu_list(void)
{
	return g_factory_cfg.primary;
}

int8_t *get_factory_dr_url(int32_t index)
{
	return g_factory_cfg.repo_addr.drs_url[index];
}

int8_t *get_factory_ir_url(int32_t index)
{
	return g_factory_cfg.repo_addr.irs_url[index];
}
