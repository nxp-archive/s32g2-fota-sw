/*
 * Copyright 2018-2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause

 */
#define _XOPEN_SOURCE 500

#include "../api/pl_stdio.h"
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <ftw.h>

int pl_stat(const char *pathname, int *is_dir, int *size)
{
	int ret;
	struct stat stat_buf;

	*is_dir = 0;
	*size = 0;

	if ((ret = stat(pathname, &stat_buf)) == 0) {
		if (S_ISDIR(stat_buf.st_mode)) {
			*is_dir = 1;
			return 0;
		}

		if (S_ISREG(stat_buf.st_mode)) {
			*is_dir = 0;
			*size = stat_buf.st_size;
			return 0;
		}
	} else {
		return -errno;
	}
	return 0;
}

int pl_mkdir(const char *pathname, uint32_t mode)
{
	mode_t m;

	if (0 == mode)
		m = S_IRWXU;
	return mkdir(pathname, m);
}

static int32_t rm_files(const char *pathname, const struct stat *sbuf, int type, struct FTW *ftwb)
{
    if(remove(pathname) < 0)
    {
        return -errno;
    }
    return 0;
}

int pl_deldir(const char *path)
{
	struct stat stat_buf;
	int ret;

	if ((stat(path, &stat_buf) == 0) && S_ISDIR(stat_buf.st_mode)) {
		/* dir exist */
		if ((ret = nftw(path, rm_files, 2, FTW_DEPTH|FTW_MOUNT|FTW_PHYS)) < 0) {
			return -errno;
		}
	}

	return 0;
}

int pl_rename(const char *oldpath, const char *newpath)
{
	return rename(oldpath, newpath);
}