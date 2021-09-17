/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/

/*
  system initialization
  #1 initialize local repo directory
  #2 read configuration file to set up vehicle ECU list
  #3 if there is no vehicle manifest file exists, registered = 0;
     else if there is vehicle manifest file available, parse the vehicle manifest file to
        get ECU firmware infomation, and registered = (register state of PECU)
        NOTE: register state of PECU is changed from unregistered to registered only if all 
          SECU were registered
  #5 return registered state
*/

#include "../config.h"
#include "../fotav.h"

#include "../json/cJSON.h"
#include "../repo/repo.h"
#include "fota_config.h"
#include "../metadata/manifest.h"
#include "../campaign/campaign.h"

#if (LCD_ENABLE == ON)
#include "../touch_panel/cmd.h"
#endif

void update_display_demo(void);

error_t fota_system_init(int32_t *registered)
{
	error_t ret;

    /* Init the dir repo, create the director folder */
	if ((ret = repo_init()) != ERR_OK) {
		printf_dbg(LOG_ERROR, "repo init error");
		return ret;
	}

 	/*Load the factory config from the config.json file into struct g_factory_cfg */
	if ((ret = load_factory_config()) != ERR_OK) {
		printf_dbg(LOG_ERROR, "factory init error");
		return ret;
	}

	/*parse vehicle manifest in the /current*/
	if ((ret = parse_vehicle_manifest(registered)) != ERR_OK) {
		printf_dbg(LOG_ERROR, "parse vehicle manifest file error");
	}
	/*parse vehicle metadata in the /current */
    if ((ret = parse_metadata_init()) != ERR_OK) {
        printf_dbg(LOG_ERROR, "parse metadata files error");
    }

    update_display_demo();

	return ret;
}


void update_display_demo(void) 
{
    fota_ecu_t *ecu;
    int8_t text_buf[5][64];
    int32_t index = 0;
    char *ecu_list[5] = {NULL, NULL, NULL, NULL, NULL};

    memset(text_buf, 0x0, 5 * 64);

    for (ecu = get_ecu_list(); ecu != NULL; ecu = ecu->next) {
        snprintf(text_buf[index], 64, "%s, FW version: %d.%d", ecu->hw_id, ecu->fw.version_major, ecu->fw.version_minor);
        ecu_list[index] = text_buf[index];

#if (LCD_ENABLE == OFF)
        printf_it(LOG_INFO, "ECU list: %s", ecu_list[index]);
#endif
        
        index++;
    }

#if (LCD_ENABLE == ON)
    updateAllScreenString(get_factory_vin(), "IDLE");

    setFOTADemo_EScreen(ecu_list[0],
						ecu_list[1],
						ecu_list[2],
						ecu_list[3],
						ecu_list[4]);
    
	switchScreen(fotademo_e_screen_id);
    
#endif
}

