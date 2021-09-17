/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#include"../metadata/metadata.h"
#include "..//fotav.h"
#include "../asn1_coder/Metadata.h"
#include "../repo/repo.h"
#include "../init/fota_config.h"
#include "../campaign/campaign.h"




extern error_t verify_root_metadata(const char *path, const char *fname, metadata_root_t *root, int8_t repo);
extern error_t parse_root(const char *path, const char *fname, metadata_root_t *root);
int32_t metadata_verify(void)
{
    int8_t * path = "/repo_uptane/metadata/dr/current/";
    int32_t remote_repo = 1;  /* 1=DRS, 0=IRS */
    if(parse_root(path, "root.der", &metadata_dl[remote_repo].root) != ERR_OK)
    {
        printf_dbg(LOG_ERROR,"root.der verification false");
        return 0;
    }  
    
    if(verify_root_metadata(path, "root.der", &metadata_dl[remote_repo].root,remote_repo)!= ERR_OK)
    {
        printf_dbg(LOG_ERROR,"root.der verification false");
        return 0;
    };
    printf_dbg(LOG_ERROR,"root.der verification OK");
  
    return 0;
}