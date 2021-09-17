/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#ifndef SECURITY_COMMON_H
#define SECURITY_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extern  unsigned char prvkey_manifest[32];
extern  unsigned char pubkey_manifest[32];

#endif