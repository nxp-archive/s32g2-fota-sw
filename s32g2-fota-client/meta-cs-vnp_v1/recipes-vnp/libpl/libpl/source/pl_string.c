/*
 * Copyright 2018-2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause

 */
#include "pl_string.h"

char *pl_strcpy_s(char *dest, size_t dest_size, const char *src, bool *p_trunc)
{
	char *ret = dest;
	size_t n = dest_size;

	while (n > 0 && (*dest++ = *src++) != '\0')
		n--;
	
	if (n == 0 && dest_size)
		dest[-1] = '\0';
	
	if (p_trunc)
		*p_trunc = (n == 0);
	return ret;
}

char *pl_strncpy_s(char *dest, size_t dest_size, const char *src, size_t n, bool *p_trunc)
{
	char *ret = dest;

	n = dest_size < n ? dest_size : n;
	while (n > 0 && (*dest++ = *src++) != '\0')
		n--;
	
	if (n == 0 && dest_size)
		dest[-1] = '\0';
	
	if (p_trunc)
		*p_trunc = (n == 0);
	
	return ret;
}



