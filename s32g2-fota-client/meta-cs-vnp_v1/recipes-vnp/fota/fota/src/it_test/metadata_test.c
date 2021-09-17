/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "../../src/fotav.h"
#include "../../src/repo/repo.h"
#include "../init/fota_config.h"
#include "../asn1_coder/Metadata.h"
#include "metadata_test.h"
#include "../metadata/metadata.h"
extern metadata_t metadata_curr[2];
extern metadata_t metadata_dl[2];
int32_t metadata_test(void)
{
    if (parse_metadata_files(repo_get_dr_curr_md_path(), &metadata_curr[REPO_DIRECT]) != ERR_OK)
    {
        return 0;
    }
	printf("*************************\n");
    if (parse_metadata_files(repo_get_ir_curr_md_path(), &metadata_curr[REPO_IMAGE]) != ERR_OK)
    {
        return 0;
    }
	printf("Metadata Parsing function works fine\n");
    return 0;
}
	