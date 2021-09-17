/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
extern int8_t *read_file(const int8_t *filename, int32_t *size);
extern cJSON* json_file_parse(const int8_t *path);
extern void json_file_parse_end(cJSON *parsed);
extern error_t json_file_write(cJSON *json, const char *path);
extern error_t json_file_copy(const int8_t *old, const int8_t *new);
extern error_t file_copy(const char *src_file, const char *dest_file);

