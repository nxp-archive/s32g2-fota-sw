/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#include "pl_types.h"
#include "pl_stdlib.h"
#include "pl_string.h"
#include "pl_stdio.h"

#include "../fotav.h"
#include "cJSON.h"
#include "json_file.h"


int8_t *read_file(const int8_t *filename, int32_t *size)
{
    FILE *file = NULL;
    int32_t length = 0;
    int8_t *content = NULL;
    int32_t read_chars = 0;

    /* open in read binary mode */
    file = fopen(filename, "rb");

    if (file == NULL)
    {
        printf_dbg(LOG_ERROR, "can not open file %s", filename);
        goto cleanup;
    }

    /* get the length */
    if (fseek(file, 0, SEEK_END) != 0)
    {
        printf_dbg(LOG_ERROR, "can not get file size %s", filename);
        goto cleanup;
    }
    length = ftell(file);
    if (length < 0)
    {
        printf_dbg(LOG_ERROR, "get file size error %d", length);
        goto cleanup;
    }
    if (fseek(file, 0, SEEK_SET) != 0)
    {
        printf_dbg(LOG_ERROR, "can not seek file header %s", filename);
        goto cleanup;
    }

    /* allocate content buffer */
    content = (char *)malloc((size_t)length + sizeof(""));
    if (content == NULL)
    {
        printf_dbg(LOG_ERROR, "memory alloc fail file=%s size=%d", filename, length);
        goto cleanup;
    }

    /* read the file into memory */
    read_chars = fread(content, sizeof(char), (size_t)length, file);
    if ((long)read_chars != length)
    {
        free(content);
        content = NULL;
        printf_dbg(LOG_ERROR, "file read error %s", filename);
        goto cleanup;
    }
    content[read_chars] = '\0';
    if (size != NULL)
        *size = length;
    fclose(file);
    return content;
cleanup:
    if (file != NULL)
    {
        fclose(file);
    }
    return content;
};



cJSON* json_file_parse(const int8_t *path)
{
	int8_t *fdata;
	cJSON *items;

	if ((fdata = read_file(path, NULL)) == NULL) {
		return NULL;
	}

	items = cJSON_Parse(fdata);

	pl_free(fdata);

	return items;
}

void json_file_parse_end(cJSON *parsed)
{
	cJSON_Delete(parsed);
}

error_t json_file_write(cJSON *json, const char *path)
{
	FILE *pfd = NULL;
	ssize_t written;
	int8_t *string = NULL;
	error_t err = ERR_NOK;

	printf_ut(LOG_INFO, "writing json file %s", path);
	if ((pfd = fopen(path, "w+")) == NULL) {
		printf_dbg(LOG_ERROR, "file open fail, %d\n", pfd);
		goto JSON_F_W_RET;
	}

//	printf_dbg(LOG_INFO, "fopen %s", path);

	if ((string = cJSON_Print(json)) == NULL) {
		printf_dbg(LOG_ERROR, "json print error");
		goto JSON_F_W_RET;
	}

//	printf_dbg(LOG_INFO, "cJSON_Print ok");

	written = fwrite(string, 1, strlen(string), pfd);
	if (written < 0) {
		printf_dbg(LOG_ERROR, "file write fail, %d\n", written);
		goto JSON_F_W_RET;
	}

//	printf_dbg(LOG_INFO, "fwrite ok");
	
	err = ERR_OK;

JSON_F_W_RET:
	if (pfd != NULL) 
		fclose(pfd);
	if (string != NULL)
		pl_free(string);

	return err;
}

error_t json_file_copy(const int8_t *old, const int8_t *new)
{
	cJSON *parsed;
	error_t err = ERR_NOK;

	if ((parsed = json_file_parse(old)) == NULL) {
		printf_dbg(LOG_ERROR, "parse json file error: %s", old);
		goto json_file_copy_clean;
	}

	if (json_file_write(parsed, new) != ERR_OK) {
		printf_dbg(LOG_ERROR, "write json file error: %s", new);
		goto json_file_copy_clean;
	}

	err = ERR_OK;
	
json_file_copy_clean:
	if (parsed != NULL) 
		json_file_parse_end(parsed);
	return err;
}

error_t file_copy(const char *src_file, const char *dest_file)
{
    FILE *out;
	FILE *in;
    int8_t fdata[4069];
    int32_t fsize, count;

	if((in = fopen(src_file,"rb")) == NULL) {
        printf_it(LOG_ERROR, "open dest file error");
        return ERR_NOK;
    }
	fsize = fread(fdata, 1, 4069, in);    
    if((out = fopen(dest_file,"wb+")) == NULL) {
        printf_it(LOG_ERROR, "open dest file error");
        return ERR_NOK;
    }

    if ((count = fwrite(fdata, sizeof(int8_t), fsize, out)) == fsize) {
//		printf_it(LOG_DEBUG, "count:%d",count);
		fclose(out);
	    fclose(in);
        return ERR_OK;
    }
    else
    {
        fclose(out);
	    fclose(in);
    }

    return ERR_NOK;
}

