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


int32_t repo_test(void)
{
	error_t ret;
	ret = repo_mv_verified_2_current();
	if (ret != ERR_OK)
		printf_ut(LOG_ERROR, "repo_mv_verified_2_current() failed");
	else
		printf_ut(LOG_INFO, "repo_mv_verified_2_current() success");
	return ret;
}