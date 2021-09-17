/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/

#include "../../src/json/cJSON.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "pl_types.h"
#include "pl_stdlib.h"
#include "pl_string.h"
#include "pl_stdio.h"

#include "../../src/fotav.h"
#include "../json/json_file.h"
#include "../init/fota_config.h"
static error_t parse_config(const int8_t *cfg_file_path, factory_cfg_t *pcfg)
{
    cJSON *root = NULL;
    cJSON *factory = NULL;
    cJSON *item = NULL;
    cJSON *sub_item = NULL, *subsub=NULL;
    int32_t cnt = 0;
    fota_ecu_t *ecu_list;
    fota_ecu_t *tmp_item;
	char* actual = NULL;
    if ((root = json_file_parse(cfg_file_path)) == NULL) {
        printf_dbg(LOG_ERROR, "parse config file error");
        return ERR_NOK;
    }

     actual = cJSON_Print(root);
     printf_ut(LOG_INFO, "parsed json: \n %s", actual);

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

int32_t json_test(void)
{
	const int8_t* json_file_path = "/etc/fota_config/case1/config.json";
	factory_cfg_t* pcfg = (factory_cfg_t*)pl_malloc_zero(sizeof(factory_cfg_t));
    if(parse_config(json_file_path,pcfg) == ERR_OK)
	printf_ut(LOG_INFO,"parse config done");
	return 0;
}
// int32_t json_test(void)
// {
// 	char *json_string;
// 	cJSON *parsed = NULL;
// 	char *actual = NULL;

// 	cJSON *primary = NULL;
// 	cJSON *secondary;
// 	cJSON *interval;
// 	cJSON *tmp;
// 	int32_t size=0;
	
// 	json_string = read_file("/etc/fota_config/case1/config.json",&size);
// 	if (json_string == NULL)
// 		return 0;

// 	parsed = cJSON_Parse(json_string);

// 	free(json_string);

// 	if (parsed != NULL) {
// 		actual = cJSON_Print(parsed);
// 		printf_ut(LOG_INFO, "parsed json: \n %s", actual);
// 		if (actual != NULL)
// 		{
// 			free(actual);
// 		}

// 		if (cJSON_HasObjectItem(parsed, "ECU")) {
// 			tmp = cJSON_GetObjectItem(parsed, "ECU");
// 			printf_ut(LOG_INFO, "%s, %s, %f", tmp->string, tmp->valuedouble, tmp->valuestring);
// 		}

// 		if (cJSON_HasObjectItem(tmp, "secondary_ecu")) {
// 			secondary = cJSON_GetObjectItem(tmp, "Scenary_ECU");
// 			printf_ut(LOG_INFO, "%s, %s, %f", secondary->string, secondary->valuedouble, secondary->valuestring);
// 		}
		

// 		if (cJSON_HasObjectItem(tmp, "Primary_ECU")) {
// 			primary = cJSON_GetObjectItem(tmp, "Primary_ECU");
// 			printf_ut(LOG_INFO, "%s, %s, %f", primary->string, primary->valuedouble, primary->valuestring);
// 		}

// 		interval = cJSON_GetObjectItemCaseSensitive(primary, "Checking_Interval");
// 		if (interval) {

// 			printf_ut(LOG_INFO, "%s, %s, %f", interval->string, interval->valuedouble, interval->valuestring);

// 			if (cJSON_IsNumber(interval))
// 				printf_ut(LOG_INFO, "checking interval=%f", interval->valuedouble);
// 			else if (cJSON_IsString(interval))
// 				printf_ut(LOG_INFO, "checking interval=%s", interval->valuestring);
// 		}

// 		cJSON_Delete(parsed);
// 	}

// 	return 1;
// }