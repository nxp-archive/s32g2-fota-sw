/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#ifndef _CONFIG_H
#define _CONFIG_H

#ifndef ON
#define ON 	(1)
#endif
#ifndef OFF
#define OFF 	(0)
#endif

#define UNIT_TEST		OFF
#define INTEGRATION_TEST	OFF      /* automatic aprove installing updates, don't need user's input */
#define DEBUG_			ON 

#if (INTEGRATION_TEST == ON)
#define UPDATE_TEST     OFF          /* repeat updating cycle, without publish repeatly */
#else
#define UPDATE_TEST     OFF         /* finish one updating cycle and exit */
#endif

#define STUB_TEST   OFF             /* don't download metadata from fota server; use the local ones */
#define STUB_MANIFEST   ON          /* stubs for ECUs that don't send ECU manifest */
#define LCD_ENABLE  OFF             /* integrate LCD display */
#define SCP_CLIENT_TEST    OFF
#define PRIMARY_ECU_RESET      OFF  /* self-reboot after installing updates */

#define PRIMARY_ECU		ON

#define REPO_URL_MIRROR_NUM_MAX	(4)
#define LENGTH_ECU_SERIAL	(64)
#define LENGTH_HW_ID        (16)
#define LENGTH_REPO_URL		(64)
#define LENGTH_DIR_PATH		(128)
#define LENGTH_FNAME		(64)

#define FACTORY_CFG_UDS		(OFF)

#define FOTA_UPDATING_CYCLE_S   (5)

#endif