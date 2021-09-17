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
#include "../init/fota_config.h"
#include "repo.h"

#define REPO_VEHICLE_MANIFEST	REPO_MANIFEST "manifest.der"

#define CREAT_UPTANE_DIR(path)  do { \
					if (!((pl_stat(path, &is_dir, &size) == 0) && is_dir)) { \
						if ((ret = pl_mkdir(path, 0)) != 0) { \
							printf_dbg(LOG_ERROR, "can not mkdir %s", path); \
							return ERR_NOK; \
						} \
					} \
				} while (0)

static error_t repo_is_file_exist(const int8_t *path, int32_t *exist);

error_t repo_init_clear(void)
{
	/* todo */
}

error_t repo_init(void)
{
	int32_t ret;
	int32_t is_dir, size;

    /* dir init */
	CREAT_UPTANE_DIR(REPO_ROOT);
	CREAT_UPTANE_DIR(REPO_METADATA);
	CREAT_UPTANE_DIR(REPO_METADATA_IR);
	CREAT_UPTANE_DIR(REPO_METADATA_DR);
	CREAT_UPTANE_DIR(REPO_TARGETS);
	CREAT_UPTANE_DIR(REPO_TARGETS_IR);
    CREAT_UPTANE_DIR(REPO_TARGETS_DR);
	CREAT_UPTANE_DIR(REPO_MANIFEST);
	
	CREAT_UPTANE_DIR(REPO_METADATA_IR REPO_CURR_RELATIVE);
	CREAT_UPTANE_DIR(REPO_METADATA_IR REPO_PREV_RELATIVE);
	CREAT_UPTANE_DIR(REPO_METADATA_IR REPO_VERI_RELATIVE);
	CREAT_UPTANE_DIR(REPO_METADATA_IR REPO_DL_RELATIVE);

    CREAT_UPTANE_DIR(REPO_TARGETS_IR REPO_CURR_RELATIVE);
	CREAT_UPTANE_DIR(REPO_TARGETS_IR REPO_PREV_RELATIVE);
	CREAT_UPTANE_DIR(REPO_TARGETS_IR REPO_VERI_RELATIVE);
	CREAT_UPTANE_DIR(REPO_TARGETS_IR REPO_DL_RELATIVE);

	CREAT_UPTANE_DIR(REPO_METADATA_DR REPO_CURR_RELATIVE);
	CREAT_UPTANE_DIR(REPO_METADATA_DR REPO_PREV_RELATIVE);
	CREAT_UPTANE_DIR(REPO_METADATA_DR REPO_VERI_RELATIVE);
	CREAT_UPTANE_DIR(REPO_METADATA_DR REPO_DL_RELATIVE);

    CREAT_UPTANE_DIR(REPO_TARGETS_DR REPO_CURR_RELATIVE);
	CREAT_UPTANE_DIR(REPO_TARGETS_DR REPO_PREV_RELATIVE);
	CREAT_UPTANE_DIR(REPO_TARGETS_DR REPO_VERI_RELATIVE);
	CREAT_UPTANE_DIR(REPO_TARGETS_DR REPO_DL_RELATIVE);

	return ERR_OK;
}

static error_t repo_mv_metadata(const int8_t *src, const int8_t *dest)
{
	int32_t ret;
	int32_t is_dir, size;
	
	if (pl_deldir(dest) < 0) {
		printf_dbg(LOG_ERROR, "clean dir:%s failed", dest);
		return ERR_NOK;
	}

	if (pl_rename(src, dest) < 0) {
		printf_dbg(LOG_ERROR, "rename file %s->%s error", src, dest);
		return ERR_NOK;
	}

	CREAT_UPTANE_DIR(src);

	return ERR_OK;
}

error_t repo_mv_dl_2_verified(void)
{
	int8_t ir_oldpath[] = REPO_METADATA_IR REPO_DL_RELATIVE;
	int8_t ir_newpath[] = REPO_METADATA_IR REPO_VERI_RELATIVE;
	int8_t dr_oldpath[] = REPO_METADATA_DR REPO_DL_RELATIVE;
	int8_t dr_newpath[] = REPO_METADATA_DR REPO_VERI_RELATIVE;
	if (repo_mv_metadata(ir_oldpath, ir_newpath) != ERR_OK) 
		return ERR_NOK;
	if (repo_mv_metadata(dr_oldpath, dr_newpath) != ERR_OK)
		return ERR_NOK;

	return ERR_OK;
}

error_t repo_mv_verified_2_current(void)
{
	int8_t ir_oldpath[] = REPO_METADATA_IR REPO_VERI_RELATIVE;
	int8_t ir_newpath[] = REPO_METADATA_IR REPO_CURR_RELATIVE;
	int8_t dr_oldpath[] = REPO_METADATA_DR REPO_VERI_RELATIVE;
	int8_t dr_newpath[] = REPO_METADATA_DR REPO_CURR_RELATIVE;
	if (repo_mv_metadata(ir_oldpath, ir_newpath) != ERR_OK) 
		return ERR_NOK;
	if (repo_mv_metadata(dr_oldpath, dr_newpath) != ERR_OK)
		return ERR_NOK;

	return ERR_OK;
}

error_t repo_mv_current_2_previous(void)
{
	int8_t ir_oldpath[] = REPO_METADATA_IR REPO_CURR_RELATIVE;
	int8_t ir_newpath[] = REPO_METADATA_IR REPO_PREV_RELATIVE;
	int8_t dr_oldpath[] = REPO_METADATA_DR REPO_CURR_RELATIVE;
	int8_t dr_newpath[] = REPO_METADATA_DR REPO_PREV_RELATIVE;
	if (repo_mv_metadata(ir_oldpath, ir_newpath) != ERR_OK)
		return ERR_NOK;
	if (repo_mv_metadata(dr_oldpath, dr_newpath) != ERR_OK)
		return ERR_NOK;

	return ERR_OK;
}

error_t repo_targets_dl_2_verified(void)
{
	int8_t ir_oldpath[] = REPO_TARGETS_IR REPO_DL_RELATIVE;
	int8_t ir_newpath[] = REPO_TARGETS_IR REPO_VERI_RELATIVE;
	int8_t dr_oldpath[] = REPO_TARGETS_DR REPO_DL_RELATIVE;
	int8_t dr_newpath[] = REPO_TARGETS_DR REPO_VERI_RELATIVE;
	if (repo_mv_metadata(ir_oldpath, ir_newpath) != ERR_OK) 
		return ERR_NOK;
	if (repo_mv_metadata(dr_oldpath, dr_newpath) != ERR_OK)
		return ERR_NOK;

	return ERR_OK;
}

error_t repo_targets_verified_2_current(void)
{
	int8_t ir_oldpath[] = REPO_TARGETS_IR REPO_VERI_RELATIVE;
	int8_t ir_newpath[] = REPO_TARGETS_IR REPO_CURR_RELATIVE;
	int8_t dr_oldpath[] = REPO_TARGETS_DR REPO_VERI_RELATIVE;
	int8_t dr_newpath[] = REPO_TARGETS_DR REPO_CURR_RELATIVE;
	if (repo_mv_metadata(ir_oldpath, ir_newpath) != ERR_OK) 
		return ERR_NOK;
	if (repo_mv_metadata(dr_oldpath, dr_newpath) != ERR_OK)
		return ERR_NOK;

	return ERR_OK;
}

error_t repo_targets_current_2_previous(void)
{
	int8_t ir_oldpath[] = REPO_TARGETS_IR REPO_CURR_RELATIVE;
	int8_t ir_newpath[] = REPO_TARGETS_IR REPO_PREV_RELATIVE;
	int8_t dr_oldpath[] = REPO_TARGETS_DR REPO_CURR_RELATIVE;
	int8_t dr_newpath[] = REPO_TARGETS_DR REPO_PREV_RELATIVE;
	if (repo_mv_metadata(ir_oldpath, ir_newpath) != ERR_OK)
		return ERR_NOK;
	if (repo_mv_metadata(dr_oldpath, dr_newpath) != ERR_OK)
		return ERR_NOK;

	return ERR_OK;
}


int8_t* repo_get_manifest_path(void)
{
	int8_t *manifest = REPO_VEHICLE_MANIFEST;
	return manifest;
}

int8_t *repo_get_irs_url(int32_t mirror)
{
	factory_cfg_t *cfg;
	
	cfg = get_factory_cfg();
	return cfg->repo_addr.irs_url[mirror];
}

int8_t *repo_get_drs_url(int32_t mirror)
{
	factory_cfg_t *cfg;
	
	cfg = get_factory_cfg();
	return cfg->repo_addr.drs_url[mirror];
}

int8_t *repo_get_vin(void)
{
	factory_cfg_t *cfg;
	
	cfg = get_factory_cfg();
	return cfg->vin;
}

static error_t repo_is_file_exist(const int8_t *path, int32_t *exist)
{
	int32_t is_dir, size;
	int32_t ret;

	*exist = 0;

	if ((ret = pl_stat(path, &is_dir, &size)) == 0) {
		if ((is_dir == 0) && (size > 0))
			*exist = 1;
	} else if (ret == -2) {
		*exist = 0;
	} else {
		printf_dbg(LOG_ERROR, "pl_stat error %d", ret);
		return ERR_NOK;
	}

	return ERR_OK;
}

error_t repo_is_manifest_exist(int32_t *exist)
{
	int8_t *fpath = REPO_VEHICLE_MANIFEST;

	return repo_is_file_exist(fpath, exist);
}

error_t repo_is_campaign_exist(int32_t *exist)
{
	int8_t *fpath = REPO_CAMPAIGN;

	return repo_is_file_exist(fpath, exist);
}


int8_t *repo_get_dr_verified_md_path(void) 
{
	return REPO_DR_METADATA_VERIFIED;
}

int8_t *repo_get_ir_verified_md_path(void) 
{
	return REPO_IR_METADATA_VERIFIED;
}

int8_t *repo_get_dr_curr_md_path(void)
{
	return REPO_DR_METADATA_CURRENT;
}

int8_t *repo_get_ir_curr_md_path(void)
{
	return REPO_IR_METADATA_CURRENT;
}

int8_t *repo_get_dr_prev_md_path(void)
{
	return REPO_DR_METADATA_PREVIOUS;
}

int8_t *repo_get_ir_dl_image_path(void)
{
	return REPO_TARGETS_IR REPO_DL_RELATIVE;
}

int8_t *repo_get_dr_dl_image_path(void)
{
	return REPO_TARGETS_DR REPO_DL_RELATIVE;
}


int8_t *repo_get_dr_dl_md_path(void)
{
	return REPO_DR_METADATA_DOWNLOAD;
}

int8_t *repo_get_ir_dl_md_path(void)
{
	return REPO_IR_METADATA_DOWNLOAD;
}

error_t repo_is_verified_target_exist(int32_t *exist)
{
	int8_t target_path[128];

	snprintf(target_path, 128, REPO_DR_METADATA_VERIFIED "targets.json");

	return repo_is_file_exist(target_path, exist);
}

error_t repo_is_curr_target_exist(int32_t *exist)
{
	int8_t target_path[128];

	snprintf(target_path, 128, REPO_DR_METADATA_CURRENT "targets.json");

	return repo_is_file_exist(target_path, exist);
}

error_t repo_is_prev_target_exist(int32_t *exist)
{
	int8_t target_path[128];

	snprintf(target_path, 128, REPO_DR_METADATA_PREVIOUS "targets.json");

	return repo_is_file_exist(target_path, exist);
}

error_t repo_clear_all_metadata(void)
{
	int32_t ret;

	if ((ret = pl_deldir(REPO_METADATA)) < 0) {
		printf_dbg(LOG_ERROR, "clear %s failed", REPO_METADATA);
		return ERR_NOK;
	}

	return repo_init();
}




