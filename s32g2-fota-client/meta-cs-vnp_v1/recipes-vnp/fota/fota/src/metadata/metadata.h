/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#ifndef _METADATA_H_
#define _METADATA_H_

#define REPO_IMAGE  (0)
#define REPO_DIRECT (1)
#include "security_common.h"
#include "fotav.h"
typedef enum {
    FW_STATUS_UNREGISTERED = 1,
    FW_STATUS_PRESENT,
    FW_STATUS_ADDED,
    FW_STATUS_UPDATED,
    FW_STATUS_REMOVED
} fw_status;

typedef enum {
    ECU_CAMPAIGN_IDLE = 0,
    ECU_CAMPAIGN_SYNC,
    ECU_CAMPAIGN_TRANS_APPROVE,
    ECU_CAMPAIGN_TRANS_DL,
    ECU_CAMPAIGN_TRANS_VERIFY,
    ECU_CAMPAIGN_TRANS,
    ECU_CAMPAIGN_PROC_APPROVE,
    ECU_CAMPAIGN_PROC_CMD,
    ECU_CAMPAIGN_PROC_POLL,
    ECU_CAMPAIGN_ACTIVE_APPROVE,
    ECU_CAMPAIGN_ACTIVE_CMD,
    ECU_CAMPAIGN_ACTIVE_POLL,
    ECU_CAMPAIGN_CHECK
} ecu_campaign_state;

typedef enum {
    CAMPAIGN_ERR_NONE = 0,
    CAMPAIGN_ERR_IDLE,
    CAMPAIGN_ERR_SYNC,
    CAMPAIGN_ERR_TRANS_APPROVE,
    CAMPAIGN_ERR_TRANS,
    CAMPAIGN_ERR_PROC_APPROVE,
    CAMPAIGN_ERR_PROC,
    CAMPAIGN_ERR_ACTIVE_APPROVE,
    CAMPAIGN_ERR_ACTIVE,
    CAMPAIGN_ERR_CHECK,
    CAMPAIGN_ERR_FINISH,
    CAMPAIGN_ERR_ROLLBACK
} campaign_err;

typedef enum {
    SECU_STATUS_IDLE = 0,
    SECU_STATUS_TRANSFERING,
    SECU_STATUS_TRANSFERED,
    SECU_STATUS_PROCESS,
    SECU_STATUS_PROCESSING,
    SECU_STATUS_PROCESSED,
    SECU_STATUS_ACTIVE,
    SECU_STATUS_ACTIVATING,
    SECU_STATUS_ACTIVATED,
    SECU_STATUS_FINISH
} fota_secu_state;

typedef enum {
    PROTOCAL_UDS = 0,
    PROTOCAL_SCP
}fota_net_protocal;

typedef struct fota_image_t {
	struct fota_image_t *next;
	struct fota_ecu_t *ecu_target;	/* ecu target, fota_ecu_t* */
	int32_t sub_target;	/* sub ID, for ecu with more than one image */
	char fname[LENGTH_FNAME];
	int32_t fsize;
	struct hash_list *image_hash;
    int8_t  version_major;
    int8_t  version_minor;
    int8_t  isCompressed;
    int8_t  isDelta;
    fw_status  status;
    int8_t  dependency;
//    campaign_err err_state;
    int8_t  transferID[16];
	uint8_t sha256[(256 >> 2) + 8];	/* +4 for string end '\0' */

} fota_image_t;

typedef struct fota_ecu_t {
	struct fota_ecu_t *next;
	char serial[LENGTH_ECU_SERIAL];     /* unique, used only by director */
    char hw_id[LENGTH_HW_ID];           /* hardware id: ECU type */
	int32_t is_primary;
	uint32_t addr;
	uint32_t addr_ext;
    uint32_t fw_install_cnt;
	int8_t hw_version_major;
    int8_t hw_version_minor;
	int8_t full_verify;
    int8_t installMethod;
    int8_t imageFormat;
    int8_t protocal;
    campaign_err err_state;
    ecu_campaign_state campaign;
    fota_secu_state fota_state;
	fota_image_t fw;
    fota_image_t factory;
    fota_image_t *updates;
} fota_ecu_t;

#define KEY_TYPE_LENGTH		(16)
#define KEY_ID_LENGTH		(72)
#define KEY_VALUE_LENGTH	(384)
#define HASH_VALUE_LENGTH     (64)
#define HASH_FUNCTION_LENGTH     (64)

typedef struct hash_list {
    struct hash_list *next;
	int8_t hash_function[HASH_FUNCTION_LENGTH];
	int8_t hash_value[HASH_VALUE_LENGTH];
    int8_t hash_value_size;

} hash_list_t;


typedef struct key_list {
	struct key_list *next;
	int8_t keyid[KEY_ID_LENGTH];
	int8_t key_type[KEY_TYPE_LENGTH];
	int8_t key_value[KEY_VALUE_LENGTH];
    int8_t key_size;
} key_list_t;

typedef struct sig_list {
	struct sig_list *next;
	int8_t keyid[KEY_ID_LENGTH];
	int8_t scheme[KEY_TYPE_LENGTH];
	int8_t sig_value[KEY_VALUE_LENGTH];
  int8_t sig_size;
} sig_list_t;

typedef struct key_ {
	int32_t threshold;
	key_list_t *keys;
} metadata_key_t;

typedef struct {
	int32_t version;
	int32_t expires;
	sig_list_t *signs;
	metadata_key_t root;
	metadata_key_t target;
	metadata_key_t snapshot;
	metadata_key_t timestamp;
} metadata_root_t;

typedef struct {
	int32_t version;
	int32_t expires;
	sig_list_t *signs;
	fota_image_t *images;    /* list */
} metadata_target_t;

typedef struct {
	int32_t version;
	int32_t length;
  int8_t hash_function[HASH_FUNCTION_LENGTH];
	int8_t hash[KEY_ID_LENGTH];
  int8_t hash_value_size;
} metadata_file_info_t;

typedef struct {
	int32_t version;
	int32_t expires;
	sig_list_t *signs;
	metadata_file_info_t snapshot;
} metadata_timestamp_t;

typedef struct {
	int32_t version;
	int32_t expires;
	sig_list_t *signs;
	metadata_file_info_t root;
	metadata_file_info_t target;
} metadata_snapshot_t;

typedef struct {
    metadata_root_t root;
    metadata_timestamp_t ts;
    metadata_snapshot_t snapshot;
    metadata_target_t target;
} metadata_t;


error_t download_verify_metadata(void);
//error_t verify_targets_full(void);

error_t get_verified_metadata_fname(int8_t repo, const char *role, char *fname);
error_t parse_current_metadata();
error_t parse_verified_metadata();
int32_t is_updates_available(void);
error_t empty_update_ecu_list(void);
error_t parse_metadata_init(void);
error_t check_factory_metadata_consistent(void);
error_t check_manifest_against_metadata(fota_ecu_t *ecu_check, uint32_t *consistent);
fota_image_t * find_target_from_verified_metadata(fota_ecu_t *ecu);
error_t verify_image(const char *path,  fota_image_t *image);

//error_t parse_root(const char *path, const char *fname, metadata_root_t *root);


extern metadata_t metadata_curr[2];
extern metadata_t metadata_dl[2];
extern int32_t remote_repo_id;  /* 1=DRS, 0=IRS */
#endif


