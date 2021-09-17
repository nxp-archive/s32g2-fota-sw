/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#ifndef _SCREEN_DRIVER_CONFIG_H
#define _SCREEN_DRIVER_CONFIG_H
#include<stdio.h>
#include<stdint.h>
#include<stdbool.h>

extern const bool DCU_enable;
extern const bool ECU1_enable;
extern const bool ECU2_enable;
extern const bool ECU3_enable;

/*****************
* Receive Commands map
* ***************/
extern const char msg_rec_S32G_unupdates[];
extern const char msg_rec_S32G_updates[];
extern const char msg_rec_S32G_later_updates[];
extern const char msg_rec_S32G_uninstall[];
extern const char msg_rec_S32G_install[];
extern const char msg_rec_S32G_later_install[];
extern const char msg_rec_S32G_inactive[];
extern const char msg_rec_S32G_active[];
extern const char msg_rec_S32G_later_active[];
extern const char msg_rec_S32G_English[];




#endif