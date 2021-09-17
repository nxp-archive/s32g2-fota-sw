/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/

#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ftw.h>
#include <mqueue.h>
#include <fcntl.h>


#include "../fotav.h"
#include "it_fota.h"

/* global */
int32_t caseno;
int8_t path[128];
int32_t path_idx;
mqd_t mq_s, mq_c, mq_dl_c, mq_dl_s;

static int32_t rm_files(const char *pathname, const struct stat *sbuf, int type, struct FTW *ftwb)
{
    if(remove(pathname) < 0)
    {
        printf_it(LOG_ERROR, "remove %s", pathname);
        return -1;
    }
    return 0;
}

static error_t xml_parsing_url(const int8_t *file_path, int8_t *url_o)
{
	FILE *fp;
	int32_t fsize;
	int8_t *fdata;
	int8_t *url;
	int8_t *pstr;

	printf_it(LOG_INFO, "opening xml rsp file %s", file_path);

	if ((fp = fopen(file_path, "r")) == NULL) {
		printf_it(LOG_ERROR, "can not open file %s", file_path);
		return ERR_NOK;
	}

	fseek(fp, 0L, SEEK_END);	/* get file size */
	fsize = ftell(fp);

	if ((fdata = malloc(fsize)) == NULL) {
		fclose(fp);
		printf_it(LOG_ERROR, "can not malloc %d", fsize);
		return ERR_NOK;
	}

	fseek(fp, 0L, SEEK_SET);
	if (fsize != fread(fdata, 1, fsize, fp)) {
		fclose(fp);
		free(fdata);
		printf_it(LOG_ERROR, "read xml file error");
		return ERR_NOK;
	}

	if ((pstr = strstr(fdata, "<fault>")) != NULL) {
		printf_it(LOG_ERROR, "RPC return fault!");
		fclose(fp);
		free(fdata);
		return ERR_NOK;
	}

	if ((pstr = strstr(fdata, "<value><string>")) != NULL) {
		url = pstr + strlen("<value><string>");

		if ((pstr = strstr(pstr, "</string></value>")) != NULL) {
			*pstr = 0;
		} else {
			fclose(fp);
			free(fdata);
			return ERR_NOK;
		}
	} else {
		fclose(fp);
		free(fdata);
		return ERR_NOK;
	}

	printf_it(LOG_INFO, "parse url: %s", url);

	strcpy(url_o, url);

	fclose(fp);
	free(fdata);

	return ERR_OK;
}

int32_t it_get_repo_ul_url(const int8_t *vin, int8_t *url)
{
	int8_t mq_buf[128];
	int8_t mq_cmd[128];
	int8_t *pstr, *fname;
	int32_t mq_len;
	

	/* step #3: RPC, get upload URL of version manifest file */
	snprintf(mq_cmd, 128, 
		"xmlrpc=check_update, "
		"vin=%s, "
		"case%d",
		vin, caseno);

	printf_it(LOG_INFO, "# RPC to get upload URL: %s", mq_cmd);

	/* send xmlrpc:check for update command */
	if (mq_send(mq_dl_c, mq_cmd, sizeof(mq_cmd), 0) < 0) {
		printf_it(LOG_ERROR, "mq_send error");
		return -1;
	}

	/* wait for xmlrpc response */
	if ((mq_len = mq_receive(mq_dl_s, mq_buf, sizeof(mq_buf), NULL)) < 0) {
		printf_it(LOG_ERROR, "mq_receive error");
		return -1;
	}

	if ((pstr = strstr(mq_buf, "xmlrpc=ok")) != NULL) {
		printf_it(LOG_INFO, "xmlrpc ok");
	} else {
		printf_it(LOG_INFO, "xmlrpc failed, %s", mq_buf);
		return -1;
	}

	/* parsing rpc response */
	if ((pstr = strstr(pstr, "xml=")) != NULL) {
		fname = pstr + strlen("xml=");
		printf_it(LOG_INFO, "fname=%s, msg=%s", fname, mq_buf);
	} else {
		printf_it(LOG_ERROR, "xml not find, %s", mq_buf);
		return -1;
	}

	snprintf(&path[path_idx], 128, 
		"/%s", fname);
	
	if ((xml_parsing_url(path, url)) == ERR_NOK) {
		printf_it(LOG_ERROR, "url not find, %s", mq_buf);
		return -1;
	}

	printf_it(LOG_INFO, "... Got upload URL: %s", url);

	return 0;
}

int32_t it_get_repo_dl_url(const int8_t *vin, int8_t *url)
{
	int8_t mq_buf[128];
	int8_t mq_cmd[128];
	int8_t *pstr, *fname;
	int32_t mq_len;

	/* step #5: xml RPC, get URL of metadata */
	snprintf(mq_cmd, 128, 
		"xmlrpc=get_metadata, "
		"vin=%s, "
		"case%d", 
		vin, caseno);

	printf_it(LOG_INFO, "# RPC to get metadata URL: %s", mq_cmd);

	/* send xmlrpc:check for update command */
	if (mq_send(mq_dl_c, mq_cmd, sizeof(mq_cmd), 0) < 0) {
		printf_it(LOG_ERROR, "mq_send error");
		return -1;
	}

	/* wait for xmlrpc response */
	if ((mq_len = mq_receive(mq_dl_s, mq_buf, sizeof(mq_buf), NULL)) < 0) {
		printf_it(LOG_ERROR, "mq_receive error");
		return -1;
	}

	if ((pstr = strstr(mq_buf, "xmlrpc=ok")) != NULL) {
		printf_it(LOG_INFO, "xmlrpc ok");
	} else {
		printf_it(LOG_INFO, "xmlrpc failed, %s", mq_buf);
		return -1;
	}
	/* parsing RPC response */
	if ((pstr = strstr(mq_buf, "xml=")) != NULL) {
		fname = pstr + strlen("xml=");
		printf_it(LOG_INFO, "xml=%s", fname);
	} else {
		printf_it(LOG_ERROR, "xml not find, %s", mq_buf);
		return -1;
	}

	snprintf(&path[path_idx], 128, 
		"/%s", fname);
	
	if ((xml_parsing_url(path, url)) == ERR_NOK) {
		printf_it(LOG_ERROR, "url not find, %s", mq_buf);
		return -1;
	}

	printf_it(LOG_INFO, "... Got metadata URL: %s", url);

	return 0;
}

int32_t it_repo_upload(const int8_t *vin, const int8_t *primary_ecu_esrial, const int8_t *vehicle_manifest)
{
	int8_t mq_buf[128];
	int8_t mq_cmd[128];
	int8_t *pstr, *fname;
	int32_t mq_len;

	//TODO: get the manifest from sync_vehicle_manifest() 
	/*
	vin = get_factory_vin();
	primary_ecu_esrial = get_factory_primary_serial();
	vehicle_manifest = repo_get_manifest_path();
*/
	int temcase = 1;
	snprintf(mq_cmd, 128, 
		"method=submit_vehicle_manifest, "
		"vin=%s, "
		"esrial=%s, "
		"case%d",
		vin, primary_ecu_esrial,temcase);

	//send xmlrpc:check for update command
	if (mq_send(mq_dl_c, mq_cmd, sizeof(mq_cmd), 0) < 0) {
		printf_it(LOG_ERROR, "mq_send error");
		return -1;
	}
	
	//wait for xmlrpc response 
	if ((mq_len = mq_receive(mq_dl_s, mq_buf, sizeof(mq_buf), NULL)) < 0) {
		printf_it(LOG_ERROR, "mq_receive error");
		return -1;
	}

	if ((pstr = strstr(mq_buf, "xmlrpc=ok")) != NULL) {
		printf_it(LOG_INFO, "uploading finished ok");
	} else {
		printf_it(LOG_INFO, "uploading failed, %s", mq_buf);
		return -1;
	}

	return 0;
}

int32_t it_repo_download(const int8_t *url, const int8_t *store_path, downloaf_file_cb dl_cb)
{
	int8_t mq_buf[128];
	int8_t mq_cmd[128];
	int8_t *pstr, *fname;
	int32_t mq_len;
    uint8_t prog;

	/* step #6: download metadata files */
	snprintf(mq_cmd, 128, 
		"download=%s, "
		"store=%s, "
		"case%d", 
		url, store_path, caseno);

	printf_it(LOG_INFO, "# Download file: %s", mq_cmd);

	/* send download command */
	if (mq_send(mq_dl_c, mq_cmd, sizeof(mq_cmd), 0) < 0) {
		printf_it(LOG_ERROR, "mq_send error");
		return -1;
	}

    while (1) {

    	/* wait for download response */
    	if ((mq_len = mq_receive(mq_dl_s, mq_buf, sizeof(mq_buf), NULL)) < 0) {
    		printf_it(LOG_ERROR, "mq_receive error");
    		return -1;
    	}


    	if ((pstr = strstr(mq_buf, "download=ok")) != NULL) {

    		if ((pstr = strstr(pstr, "name=")) != NULL) {
    			fname = pstr + strlen("name=");
    		}
            
            break;
    	} else if ((pstr = strstr(mq_buf, "download=ing")) != NULL) {
            prog = mq_buf[strlen(mq_buf) + 2];
            if (dl_cb != NULL) 
                dl_cb(prog);
        } else {
    		printf_it(LOG_ERROR, "download failed, %s", mq_buf);
    		return -1;
    	}
    }

	return 0;
}

#if 0
int32_t pkg_broadcast(int8_t *fpath)
{
	int8_t mq_buf[128];
	int8_t mq_cmd[128];
	int8_t *pstr, *fname;
	int32_t mq_len;

	/* step #8: file broadcast */
	// snprintf(&path[path_idx], 128, 
	// 	"/%s", fname);

	printf_it(LOG_INFO, "# Broadcast metadata and image files");

	if (pri_send_file(fpath, "10.193.248.73", 2000) == ERR_OK) {
		printf_it(LOG_INFO, "send file to %s:2000 ok", "10.193.248.73");
		if (get_secondary_manifest("10.193.248.73", 2000, &side, &version) == ERR_OK) {
			printf_it(LOG_INFO, "secondary app runing on %d side, version %d", side, version);
		} else {
			printf_it(LOG_ERROR, "can not get app running info");
			goto _IT_FOTA_ERROR;
		}
	} else {
		printf_it(LOG_ERROR, "broadcast file to secondary %s:2000 failed", "10.193.248.73");
		return -1;
	}

	return 0;
}
#endif

static void fota_set_config_path(int32_t testcase)
{
	int8_t cfg_path[128];

	snprintf(cfg_path, 128, "/etc/fota_config" "/case%d/", testcase);

	factory_set_config_path(cfg_path);

    printf_it(LOG_TRACE, "set config file path: %s", cfg_path);
}

void* fota_it_main(void *arg)
{
	int8_t mq_buf[128];
	int8_t mq_cmd[128];
	int32_t mq_len;
	struct stat stat_buf;
	int8_t *pstr;
	void *g_list = NULL;

    mqd_t mq_campaign_host;

	error_t ret;
	fota_conn_if_t ota_if;
	int32_t pass = 0;

	int32_t iteration;

	/* open the queue */
	if ((mq_s = mq_open("/mq_it_host", O_RDONLY)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_it_host fail");

	if ((mq_c = mq_open("/mq_it_client", O_WRONLY)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_it_client fail");

	if ((mq_dl_c = mq_open("/mq_it_dl_cmd", O_WRONLY)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_it_dl_cmd fail");

	if ((mq_dl_s = mq_open("/mq_it_dl_rsp", O_RDONLY)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_it_dl_cmd fail");

    if ((mq_campaign_host = mq_open("/mq_fota_campaign_host", O_WRONLY)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_fota_campaign_host fail");

	while(true) {

#if (1)
        caseno = 1;
#else
		/* waiting for start msg */
		if ((mq_len = mq_receive(mq_s, mq_buf, sizeof(mq_buf), NULL)) < 0) {
			printf_it(LOG_ERROR, "mq_receive error");
		}

		if ((pstr = strstr(mq_buf, "start=case")) != NULL) {
			caseno = strtol(pstr + strlen("start=case"), NULL, 10);
		} else {
			printf_it(LOG_ERROR, "not find start command in %s", mq_buf);

			snprintf(mq_cmd, sizeof(mq_cmd), 
				"end=?, caseno not known");

			goto _IT_FOTA_PASS;
		}

		/* open log file */
		snprintf(path, 128, 
			TEST_DIR "/case%d" "/client.log",
			caseno);
		if ((flog = fopen(path, "w+")) == NULL) {
			printf_it(LOG_ERROR, "log file can not be opened");
			
			snprintf(mq_cmd, sizeof(mq_cmd), 
				"end=case%d, log file can not open", caseno);
			goto _IT_FOTA_PASS;
		}

		/* clear and make dir */
		path_idx = snprintf(path, 128, 
			TEST_DIR "/case%d" "/dl",
			caseno);
		if ((stat(path, &stat_buf) == 0) && S_ISDIR(stat_buf.st_mode)) {
			/* /dl/ exist */
			if (nftw(path, rm_files, 2, FTW_DEPTH|FTW_MOUNT|FTW_PHYS) < 0) {
				printf_it(LOG_ERROR, "nftw error");

				snprintf(mq_cmd, sizeof(mq_cmd), 
					"end=case%d, /dl dir can not be cleared", caseno);
				goto _IT_FOTA_PASS;
			}
		} 

		mkdir(path, S_IRWXU);  /* /dl */
		printf_it(LOG_INFO, "path=%s, %d", path, path_idx);
#endif

		/* set config location */
		fota_set_config_path(caseno);

        ota_if.get_repo_ul_url = NULL;
		ota_if.get_repo_dl_url = NULL;
		ota_if.repo_upload = it_repo_upload;
		ota_if.repo_download = it_repo_download;
        fota_conn_register(&ota_if);

//		printf_it(LOG_TRACE, "start to init fota system");

        snprintf(mq_cmd, 64, 
					"%s", "start fota");
        if (mq_send(mq_campaign_host, mq_cmd, 64, 0) < 0) {
			printf_it(LOG_ERROR, "mq_send error");
		}

        /* todo: pend here */
        if ((mq_len = mq_receive(mq_s, mq_buf, sizeof(mq_buf), NULL)) < 0) {
			printf_it(LOG_ERROR, "mq_receive error");
		}


_IT_FOTA_ERROR:
		snprintf(mq_cmd, sizeof(mq_cmd), 
			"end=case%d, **fail**", caseno);
_IT_FOTA_PASS:
		/* response msg */
		if (mq_send(mq_c, mq_cmd, sizeof(mq_cmd), 0) < 0) {
			printf_it(LOG_ERROR, "mq_send error");
		}
	}
}
