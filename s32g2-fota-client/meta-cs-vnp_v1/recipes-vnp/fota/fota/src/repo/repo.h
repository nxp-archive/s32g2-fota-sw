/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#ifndef _REPO_H
#define _REPO_H

#define REPO_ROOT		    "/repo_uptane/"
#define REPO_METADATA		"/repo_uptane/metadata/"
#define REPO_METADATA_IR	"/repo_uptane/metadata/ir/"
#define REPO_METADATA_DR	"/repo_uptane/metadata/dr/"
#define REPO_TARGETS		"/repo_uptane/targets/"
#define REPO_TARGETS_IR		"/repo_uptane/targets/ir/"
#define REPO_TARGETS_DR		"/repo_uptane/targets/dr/"
#define REPO_MANIFEST		"/repo_uptane/metadata/manifest/"
#define REPO_CAMPAIGN       REPO_ROOT "campaign.json"

#define REPO_DL_RELATIVE	"download/"
#define REPO_VERI_RELATIVE	"verified/"
#define REPO_CURR_RELATIVE	"current/"
#define REPO_PREV_RELATIVE	"previous/"

#define REPO_DL	"download"
#define REPO_VERI	"verified"
#define REPO_CURR	"current"
#define REPO_PREV	"previous"

#define REPO_DR_METADATA_VERIFIED	REPO_METADATA_DR REPO_VERI_RELATIVE
#define REPO_DR_METADATA_CURRENT	REPO_METADATA_DR REPO_CURR_RELATIVE
#define REPO_DR_METADATA_PREVIOUS	REPO_METADATA_DR REPO_PREV_RELATIVE
#define REPO_DR_METADATA_DOWNLOAD	REPO_METADATA_DR REPO_DL_RELATIVE

#define REPO_IR_METADATA_VERIFIED	REPO_METADATA_IR REPO_VERI_RELATIVE
#define REPO_IR_METADATA_CURRENT	REPO_METADATA_IR REPO_CURR_RELATIVE
#define REPO_IR_METADATA_PREVIOUS	REPO_METADATA_IR REPO_PREV_RELATIVE
#define REPO_IR_METADATA_DOWNLOAD	REPO_METADATA_IR REPO_DL_RELATIVE

extern error_t repo_init(void);
error_t repo_clear_all_metadata(void);
error_t repo_gen_init_vehicle_manifest(void);

error_t repo_mv_dl_2_verified(void);
error_t repo_mv_verified_2_current(void);
error_t repo_mv_current_2_previous(void);
error_t repo_targets_dl_2_verified(void);
error_t repo_targets_verified_2_current(void);
error_t repo_targets_current_2_previous(void);

int8_t* repo_get_manifest_path(void);
int8_t *repo_get_dr_verified_md_path(void);
int8_t *repo_get_ir_verified_md_path(void);
int8_t *repo_get_dr_curr_md_path(void);
int8_t *repo_get_dr_prev_md_path(void);
int8_t *repo_get_dl_image_path(void);
int8_t *repo_get_dr_dl_md_path(void);
int8_t *repo_get_ir_dl_md_path(void);
int8_t *repo_get_ir_curr_md_path(void);

int8_t *repo_get_dr_dl_image_path(void);

error_t repo_is_manifest_exist(int32_t *exist);
error_t repo_is_campaign_exist(int32_t *exist);
error_t repo_is_verified_target_exist(int32_t *exist);
error_t repo_is_curr_target_exist(int32_t *exist);
error_t repo_is_prev_target_exist(int32_t *exist);

#endif