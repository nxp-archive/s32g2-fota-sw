/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#include "pl_types.h"
#include "pl_stdlib.h"
#include "pl_string.h"
#include "pl_stdio.h"
#include "pl_tcpip.h"

#include "../fotav.h"
#include "../json/cJSON.h"
#include "../json/json_file.h"
#include "../repo/repo.h"
#include "campaign.h"
#include "../init/fota_config.h"

cJSON *init_ecu_campaign_json(const char *ecu_serial, ecu_campaign_state state, uint32_t transID)
{
    cJSON *ecu_item;

    if ((ecu_item = cJSON_CreateObject()) != NULL) {
        cJSON_AddNumberToObject(ecu_item, "campaign state", state);
        cJSON_AddNumberToObject(ecu_item, "transfer ID", transID);
    }

    return ecu_item;
}

error_t init_campaign_json()
{
    cJSON *campaign, *ecu_array;
    fota_ecu_t *ecu;
    campaign = cJSON_CreateObject();

    cJSON_AddNumberToObject(campaign, "vehicle campaign", CAMPAIGN_STATE_IDLE);

    ecu_array = cJSON_CreateObject();

    if (ecu_array != NULL) {
        cJSON_AddItemToObject(campaign, "ecu campaign", ecu_array);
        
        for (ecu = get_ecu_list(); ecu != NULL; ecu = ecu->next) {
            cJSON_AddItemToObject(ecu_array, ecu->hw_id,
                init_ecu_campaign_json(ecu->hw_id, ECU_CAMPAIGN_IDLE, 0));
        }
    }

    return json_file_write(campaign, REPO_CAMPAIGN);
}

cJSON * parse_campaign_json()
{
    cJSON *campaign;

    if ((campaign = json_file_parse(REPO_CAMPAIGN)) == NULL) {
		printf_dbg(LOG_ERROR, "parse campaign file error");
		return NULL;
	}

    return campaign;
}


/* 
  campaign file: campaign.json  (REPO_CAMPAIGN)
  json format:
  {
    "vehicle campaign":1,
    "ecu campaign": 
    [
        "ecu 1 serial": {
            "campaign state":1,
            "transfer ID":1086,
        },

        "ecu 2 serial": {
            "campaign state":1,
            "transfer ID": 10000
        }
    ]
  }

  if the campaign file does not exist, return error;
  parse the json file to get the value of "vehicle campaign" and feebback;
*/

/*
    if campaign state == SYNC, return IDLE
    if campaign state > SYNC, restore updates list
*/
error_t fota_get_vehicle_campaign_state(int32_t *campaign)
{
    int32_t exist;
    cJSON *json, *item;

    if (repo_is_campaign_exist(&exist) != ERR_OK) {
        return ERR_NOK;
    }

    if (exist == 0) {
        init_campaign_json();
        *campaign = CAMPAIGN_STATE_IDLE;
    } else {
        json = parse_campaign_json();
        if (json != NULL) {
            if ((item = cJSON_GetObjectItem(json, "vehicle campaign")) != NULL)
                *campaign = (int32_t)item->valuedouble;

            json_file_parse_end(json);
        }
    }

	return ERR_OK;
}

error_t fota_get_ecu_campaign_state()
{
    fota_image_t *fw;
    cJSON *campaign, *item, *array, *sub;
    char *ecu_id;
    fota_ecu_t *ecu_list, *ecu;

    if (is_updates_available()) {
        printf_dbg(LOG_INFO, "updates fw list is not NULL");
        return ERR_NOK;
    }

    ecu_list = get_ecu_list();

    campaign = parse_campaign_json();
    if (campaign != NULL) {
        array = cJSON_GetObjectItem(campaign, "ecu campaign");
        if (array != NULL) {
            cJSON_ArrayForEach(item, array) {

                ecu_id = item->string;
                for (ecu = ecu_list; ecu != NULL; ecu = ecu->next) {
                    if (strcmp(ecu_id, ecu->hw_id) == 0)
                        break;
                }
                if (ecu == NULL)
                    break;
                
                sub = cJSON_GetObjectItem(item, "campaign state");
                if (sub != NULL)
                    ecu->campaign = (ecu_campaign_state)sub->valuedouble;

                if (ecu->campaign != ECU_CAMPAIGN_IDLE) {
                    /* find target in verified metadata */
                    fw = find_target_from_verified_metadata(ecu);
                    add_to_update_ecu_list(fw);
                }
            }
        }

        json_file_parse_end(campaign);
        
    } else {
        return ERR_NOK;
    } 

    return ERR_OK;
}


/*
  key_name: key word in json
  str_value: string value. if it is NULL, use the value
  value: number type value of the key_name
*/
static error_t update_json_file(json_item_t *json_items, int32_t json_number)
{
    cJSON *campaign, *item, *sub;
    cJSON *array;
    cJSON *backup;
    int32_t found = 0;
    json_item_t *json;
    int32_t i;

    campaign = parse_campaign_json();
    if (campaign != NULL) {
        for (i = 0; i < json_number; i++) {
            json = &json_items[i];
            
            if ((item = cJSON_GetObjectItem(campaign, json->key)) != NULL) {
                    item->valuedouble = json->value;
                    
                found += 1;
                continue;
            }

            array = cJSON_GetObjectItem(campaign, "ecu campaign");
            if ((array != NULL) && (json->ecu_id != NULL)) {
                cJSON_ArrayForEach(item, array) {
                    if (strcmp(item->string, json->ecu_id) == 0) {
                        break;
                    }
                }

                sub = cJSON_GetObjectItem(item, json->key);
                if (sub != NULL) {
                    found += 1;

                    sub->valuedouble = json->value;
                }            
            
            }
        }   
    } else {
        return ERR_NOK;
    }
    
f_done:
    if (found > 0) {
        json_file_write(campaign, REPO_CAMPAIGN);
    }

    if (campaign != NULL)
        json_file_parse_end(campaign);

    return ERR_OK;
}

/*
 * new: the new vehicle campaign state
   addr: the address to store the new campaign state
*/
error_t fota_update_vehicle_campaign(vehicle_campaign_state new, int32_t *addr)
{
    /*
        if the file (campaign.json) does not exist, create it and set campaign state to IDLE;
    */
    int32_t exist;
    json_item_t json;

    char *campaign_str[] = {
        "null",
        "CAMPAIGN_STATE_REG",
        "CAMPAIGN_STATE_IDLE",
        "CAMPAIGN_STATE_SYNC",
        "CAMPAIGN_STATE_TRANS_APPROVE",
        "CAMPAIGN_STATE_TRANS",
        "CAMPAIGN_STATE_PROC_APPROVE",
        "CAMPAIGN_STATE_PROC",
        "CAMPAIGN_STATE_ACTIVE_APPROVE",
        "CAMPAIGN_STATE_ACTIVE",
        "CAMPAIGN_STATE_CHECK",
        "CAMPAIGN_STATE_FINISH",
        "CAMPAIGN_STATE_ROLLBACK"
    };

    if (repo_is_campaign_exist(&exist) != ERR_OK) {
        return ERR_NOK;
    }

    if (exist == 0) {
        init_campaign_json();
    } else {
        json.ecu_id = NULL;
        json.key = "vehicle campaign";
        json.value = (int32_t)new;
        update_json_file(&json, 1);
    }

    if (addr != NULL)
        *addr = (int32_t)new;

    printf_it(LOG_INFO, "update vehicle campaign state to [%s]", campaign_str[(int32_t)new]);
    
    return ERR_OK;
}

error_t fota_update_ecu_campaign(fota_ecu_t *ecu, ecu_campaign_state new, uint32_t transferID)
{
    char *ecu_id;
    /*
        if the file (campaign.json) does not exist, create it and set campaign state to IDLE;
    */
    json_item_t json[3];

    ecu_id = ecu->hw_id;

    json[0].ecu_id = ecu_id;
    json[0].key = "campaign state";
    json[0].value = (int32_t)new;

    json[1].ecu_id = ecu_id;
    json[1].key = "transfer ID";
    json[1].value = transferID;

    update_json_file(json, 2);

    ecu->campaign = new;
  
    return ERR_OK;
}

void print_ecu_campaign()
{
    fota_image_t *fw_item;
    fota_ecu_t *ecu;

    char *ecu_campaign_str[] = {
        "ECU_CAMPAIGN_IDLE",
        "ECU_CAMPAIGN_SYNC",
        "ECU_CAMPAIGN_TRANS_APPROVE",
        "ECU_CAMPAIGN_TRANS_DL",
        "ECU_CAMPAIGN_TRANS_VERIFY",
        "ECU_CAMPAIGN_TRANS",
        "ECU_CAMPAIGN_PROC_APPROVE",
        "ECU_CAMPAIGN_PROC_CMD",
        "ECU_CAMPAIGN_PROC_POLL",
        "ECU_CAMPAIGN_ACTIVE_APPROVE",
        "ECU_CAMPAIGN_ACTIVE_CMD",
        "ECU_CAMPAIGN_ACTIVE_POLL",
        "ECU_CAMPAIGN_CHECK"
    };


    char *campaign_err_str[] = {
        "CAMPAIGN_ERR_NONE",
        "CAMPAIGN_ERR_IDLE",
        "CAMPAIGN_ERR_SYNC",
        "CAMPAIGN_ERR_TRANS_APPROVE",
        "CAMPAIGN_ERR_TRANS",
        "CAMPAIGN_ERR_PROC_APPROVE",
        "CAMPAIGN_ERR_PROC",
        "CAMPAIGN_ERR_ACTIVE_APPROVE",
        "CAMPAIGN_ERR_ACTIVE",
        "CAMPAIGN_ERR_CHECK",
        "CAMPAIGN_ERR_FINISH",
        "CAMPAIGN_ERR_ROLLBACK"
    };

    char *secu_state_str[] = {
        "SECU_STATUS_IDLE",
        "SECU_STATUS_TRANSFERING",
        "SECU_STATUS_TRANSFERED",
        "SECU_STATUS_PROCESS",
        "SECU_STATUS_PROCESSING",
        "SECU_STATUS_PROCESSED",
        "SECU_STATUS_ACTIVE",
        "SECU_STATUS_ACTIVATING",
        "SECU_STATUS_ACTIVATED",
        "SECU_STATUS_FINISH"
    };

    if (is_updates_available() == 0)
        printf_it(LOG_INFO, "updates fw list is empty");

    for (ecu = get_ecu_list(); ecu != NULL; ecu = ecu->next) {
        fw_item = ecu->updates;

        while(fw_item != NULL) {
            
            printf_it(LOG_INFO, "Campaign state of updates list:\n\tECU %s \t"  
                                "\n\t Campaign: %s"
                                "\n\t Campaign error: %s"
                                "\n\t SECU state: %s",
                            ecu->hw_id, ecu_campaign_str[ecu->campaign],
                            campaign_err_str[ecu->err_state],
                            secu_state_str[ecu->fota_state]);

            fw_item = fw_item->next;
        }
    }
}

void printf_update_list(int8_t *string_buf, int32_t size_buf)
{
    fota_image_t *item;
    fota_ecu_t *ecu;
    char *ecu_hw_id, *ecu_serial;

    int32_t string_len = 0;

    string_len += snprintf(&string_buf[string_len], size_buf - string_len, 
            "Available Updates list: \n\r");
        
    for (ecu = get_ecu_list(); ecu != NULL; ecu = ecu->next) {
        item = ecu->updates;

        ecu_hw_id = "NULL";
        ecu_serial = "NULL";
    
        while (item != NULL) {

            ecu_hw_id = ecu->hw_id;
            ecu_serial = ecu->serial;

            string_len += snprintf(&string_buf[string_len], size_buf - string_len, 
                "ecu: %s \t file name: %s size %d\n", 
                ecu_hw_id, item->fname, item->fsize);

            item = item->next;
        }
    }

    printf_it(LOG_INFO, "%s", string_buf);
}


void update_ecu_campaign_list(ecu_campaign_state state)
{
    fota_image_t *fw_item;
    fota_ecu_t *ecu;
    
    for (ecu = get_ecu_list(); ecu != NULL; ecu = ecu->next) {
        fw_item = ecu->updates;
        if (fw_item != NULL) {
            if (fota_update_ecu_campaign(ecu->hw_id, state, 0) != ERR_OK) {
                printf_it(LOG_ERROR, "update ecu campaign error");
                break;
            }

            ecu->campaign = state;
        }
    }
}

error_t update_campaign_error(fota_ecu_t *ecu, campaign_err err_in)
{
    if (ecu != NULL)
        ecu->err_state = err_in;

    return ERR_OK;
}

