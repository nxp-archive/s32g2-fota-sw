/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#ifndef _MANIFEST_H
#define _MANIFEST_H

extern error_t parse_vehicle_manifest(int32_t *registered);
extern error_t build_vehicle_manifest(fota_ecu_t *ecu_list);
extern error_t parse_ecu_manifest_file(fota_ecu_t *ecu, const char *path);

#endif

