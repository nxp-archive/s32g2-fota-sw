/*
 * Copyright 2018-2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause

 */

#include <stdio.h>
#include <stdarg.h>

#include "pl_log.h"

struct pl_logm {
	int level;
};

static struct pl_logm g_global_logm = {
	.level = PL_LOG_INFO,
};

int pl_log(
		struct pl_logm *logm,
		int level,
		const char *fmt, ...)
{
	FILE *fp = level <= PL_LOG_WARNING ? stderr : stdout;
	int ret;
	va_list ap;

	logm = logm ? logm : &g_global_logm;
	if (level > logm->level)
		return 0;
	
	if (level)
	va_start(ap, fmt);
	ret = vfprintf(fp, fmt, ap);
	va_end(ap);
	return ret;
}

void pl_set_loglevel(struct pl_logm *logm, int level)
{
	logm = logm ? logm : &g_global_logm;
	logm->level = level;
}

