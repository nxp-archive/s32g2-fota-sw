/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
 #ifndef _CAMPAIGN_H_
 #define _CAMPAIGN_H_
 
#include "../fotav.h"
#include "../metadata/metadata.h"

typedef enum {
    CAMPAIGN_STATE_REG = 1,
    CAMPAIGN_STATE_IDLE,
    CAMPAIGN_STATE_SYNC,
    CAMPAIGN_STATE_TRANS_APPROVE,
    CAMPAIGN_STATE_TRANS,
    CAMPAIGN_STATE_PROC_APPROVE,
    CAMPAIGN_STATE_PROC,
    CAMPAIGN_STATE_ACTIVE_APPROVE,
    CAMPAIGN_STATE_ACTIVE,
    CAMPAIGN_STATE_CHECK,
    CAMPAIGN_STATE_FINISH,
    CAMPAIGN_STATE_ROLLBACK
} vehicle_campaign_state;

typedef struct 
{
    char *ecu_id;
    char *key;
    int32_t value;
} json_item_t;

extern error_t upload_vehicle_manifest(uint8_t *vin, uint8_t *primary_ecu_esrial, uint8_t* manifest_filename);
extern error_t sync_vehicle_manifest(void);

extern void* fota_sync_thread(void *arg);
extern void* fota_register_thread(void *arg);
extern void* fota_transfer_thread(void *arg);
extern void* fota_process_thread(void *arg);
extern void* fota_activate_thread(void *arg);
extern void* fota_check_thread(void *arg);
extern void* fota_rollback_thread(void *arg);


extern error_t fota_get_vehicle_campaign_state(int32_t *campaign);
extern error_t fota_update_vehicle_campaign(vehicle_campaign_state new, int32_t *addr);
error_t fota_update_ecu_campaign(fota_ecu_t *ecu, ecu_campaign_state new, uint32_t transferID);
error_t fota_get_ecu_campaign_state();
void update_ecu_campaign_list(ecu_campaign_state state);
error_t update_campaign_error(fota_ecu_t *ecu, campaign_err err_in);
void print_ecu_campaign();
void printf_update_list(int8_t *string_buf, int32_t size_buf);


#endif

