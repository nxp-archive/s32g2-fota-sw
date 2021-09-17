/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#include "pl_types.h"
#include "pl_tcpip.h"
#include "pl_stdlib.h"
#include "pl_string.h"
#include "pl_stdio.h"
#include "pl_stdarg.h"
#include "../fotav.h"
#include "../json/cJSON.h"
#include "../json/json_file.h"
#include "../repo/repo.h"
#include "../init/fota_config.h"
#include <string.h>
#include <errno.h>
#include <sys/inotify.h>
#include "../uds/fota_uds.h"
#include "fota_conn.h"
#if (LCD_ENABLE == ON)
#include "../touch_panel/cmd.h"
#endif

fota_conn_if_t conn_if;

void fota_conn_register(fota_conn_if_t *conn)
{
    conn_if.get_repo_dl_url = conn->get_repo_dl_url;
    conn_if.get_repo_ul_url = conn->get_repo_ul_url;
    conn_if.repo_download = conn->repo_download;
    conn_if.repo_upload = conn->repo_upload;
}

error_t fota_conn_upload(uint8_t *vin, uint8_t *primary_ecu_esrial, uint8_t* manifest_filename)
{
    uint8_t dlul_url[128];

#if (STUB_TEST == ON)
    return ERR_OK;
#endif

   	/* #1 upload file to DRS */
	printf_it(LOG_TRACE, "=> upload file [%s] to DRS", manifest_filename);
	if (conn_if.repo_upload != NULL)
		if ((conn_if.repo_upload)(vin, primary_ecu_esrial, manifest_filename) < 0) {
			printf_dbg(LOG_ERROR, "upload file to server failed");
			return ERR_NOK;
		}
     return ERR_OK;
}

error_t fota_conn_get_dl_url(char *url)
{
    uint8_t dlul_url[128];

#if 0

    dlul_url[0] = '\0';
        
    /* check url for downloading metadata files */
	if (conn_if.get_repo_dl_url != NULL)
		if ((conn_if.get_repo_dl_url)(get_factory_vin(), dlul_url) < 0) {
            if (url != NULL) *url = '\0';
			return ERR_NOK;
		}

    if (url != NULL)
        strncpy(url, dlul_url, 128);
#endif

    return ERR_OK;
}

void download_callback(uint8_t progress)
{
    static int32_t log_counter = 0;

    log_counter++;
    if ((log_counter & 7) == 0) {
#if (LCD_ENABLE == ON)
    setDownloading1file_EScreen(progress, NULL);
#else
    printf_it(LOG_INFO, "finished downloading %d ...", progress);
#endif

        log_counter = 0;
    }
}
/*
 repo_server: 0-IRS, 1-DRS
 store_path: the path (fold) to store the downloaded file
 fname: name of the file to be downloaded
   the same name is used to store the downloaded file
*/
error_t fota_conn_dl(int8_t repo_server, const char *store_path, const char *fname)
{
    uint8_t dlul_url[128];
    int32_t mirror = 0;
	char *p;
    char repo[128];
    char lcd_display[64];
    

    if ((store_path == NULL) || (fname == NULL))
        return ERR_NOK;
    
    /* check url for downloading metadata files */
	/*
	if (conn_if.get_repo_dl_url != NULL)
		if ((conn_if.get_repo_dl_url)(get_factory_vin(), dlul_url) < 0) {
            printf_dbg(LOG_ERROR, "can not get download URL");
			return ERR_NOK;
		}
	*/	

#if (LCD_ENABLE == ON)
    snprintf(lcd_display, 64, "Downloading File: %s", fname);
    setLogText(downloading1file_screen_id, lcd_display);
    setDownloading1file_EScreen(0, lcd_display);
#endif

	while (mirror < 4) {
		if (repo_server == REPO_DIRECT) {
			if((p = strstr(fname,".der")) != NULL)
			{
				snprintf(repo,128,"%s:30401/%s%s",get_factory_dr_url(mirror), get_factory_vin(), "/metadata/");
			}else
			{
			
				snprintf(repo,128,"%s:30401/%s%s",get_factory_dr_url(mirror), get_factory_vin(), "/targets/");
			}
			
		} else {
	
			if((p = strstr(fname,".der")) != NULL)
			{
				snprintf(repo,128,"%s:30301/metadata/", get_factory_ir_url(mirror));
			}else
			{
				snprintf(repo,128,"%s:30301/targets/", get_factory_ir_url(mirror));
			}

		}
		snprintf(dlul_url, 128, "%s%s", repo, fname);
        
		printf_it(LOG_DEBUG, "repo:%s",dlul_url);	

#if (STUB_TEST == ON)
            return ERR_OK;
#endif

        if (conn_if.repo_download != NULL)  {
    		if ((conn_if.repo_download)(dlul_url, store_path, download_callback) == 0) {
    			break;
    		} else {
    			mirror++;
    			printf_dbg(LOG_INFO, "download failed, have a try with mirror server");
    		}	
        }
	}

	if (mirror >= 4) {
		printf_dbg(LOG_ERROR, "can not download the metadata %s", dlul_url);
		return ERR_NOK;
	}

#if (LCD_ENABLE == ON)
        setDownloading1file_EScreen(100, NULL);
#endif

	return ERR_OK;
}

#define SCP_REMOTE_PATH     "/home/will/fota_remote/"
#define SCP_LOCAL_PATH      "/home/root/fota_local/"
#define SCP_WATCH_PATH     "/home/root/fota_remote/"

#define SCP_CMD_IDLE        "cmd_idle"
#define SCP_CMD_PROCESS     "cmd_process"
#define SCP_CMD_ACTIVE      "cmd_activate"
#define SCP_CMD_FINISH      "cmd_finish"
#define SCP_CMD_GET_STATUS  "cmd_status"


error_t scp_file_transfer(uint32_t addr, char *path)
{
    uint8_t cmd_buf[1024];

#if (SCP_CLIENT_TEST == ON)
    snprintf(cmd_buf, sizeof(cmd_buf), "cp %s /home/root/test_scp_remote/", path);
#else
    snprintf(cmd_buf, sizeof(cmd_buf), "scp %s will@%s:" SCP_REMOTE_PATH, path, pl_inet_ntoa(addr));
#endif

    if (pl_system(cmd_buf) < 0)
        return ERR_NOK;

    return ERR_OK;
}


error_t fota_conn_transfer_files(fota_ecu_t *target_ecu, uint32_t file_num, char file_path[][128], udsc_cb callback)
{
    int32_t i;
    int32_t try = 4;
    
    if (target_ecu->protocal == PROTOCAL_UDS) {

        while (try > 0) {
            if (uds_transfer_files(target_ecu->addr, target_ecu->addr_ext, file_num, file_path, callback) == ERR_OK)
                break;

            try--;
        }

        if (try <= 0)
            return ERR_NOK;
    
        return ERR_OK;
    } 
    else if (target_ecu->protocal == PROTOCAL_SCP) {
        for (i = 0; i < file_num; i++) {
        	if (scp_file_transfer(target_ecu->addr, file_path[i]) != ERR_OK)
                break;
        }

        if (i == file_num) {
            return ERR_OK;
        } else {
            return ERR_NOK;
        }
    }
}

error_t fota_conn_cmd_idle(fota_ecu_t *target_ecu)
{
    if (target_ecu->protocal == PROTOCAL_UDS) {
        return uds_cmd_idle(target_ecu->addr, target_ecu->addr_ext);
    }
    else if (target_ecu->protocal == PROTOCAL_SCP) {
        pl_system("mkdir " SCP_LOCAL_PATH);
        pl_system("touch " SCP_LOCAL_PATH SCP_CMD_IDLE);

        return scp_file_transfer(target_ecu->addr, SCP_LOCAL_PATH SCP_CMD_IDLE);
    }
}

error_t fota_conn_cmd_process(fota_ecu_t *target_ecu)
{
    if (target_ecu->protocal == PROTOCAL_UDS) {
        return uds_cmd_process(target_ecu->addr, target_ecu->addr_ext);
    }
    else if (target_ecu->protocal == PROTOCAL_SCP) {
        pl_system("mkdir " SCP_LOCAL_PATH);
        pl_system("touch " SCP_LOCAL_PATH SCP_CMD_PROCESS);

        return scp_file_transfer(target_ecu->addr, SCP_LOCAL_PATH SCP_CMD_PROCESS);
    }

}

error_t fota_conn_cmd_activate(fota_ecu_t *target_ecu)
{
    if (target_ecu->protocal == PROTOCAL_UDS) {
        return uds_cmd_activate(target_ecu->addr, target_ecu->addr_ext);
    }
    else if (target_ecu->protocal == PROTOCAL_SCP) {
        pl_system("mkdir " SCP_LOCAL_PATH);
        pl_system("touch " SCP_LOCAL_PATH SCP_CMD_ACTIVE);

        return scp_file_transfer(target_ecu->addr, SCP_LOCAL_PATH SCP_CMD_ACTIVE);
    }
}

error_t fota_conn_cmd_finish(fota_ecu_t *target_ecu)
{
    if (target_ecu->protocal == PROTOCAL_UDS) {
        return uds_cmd_finish(target_ecu->addr, target_ecu->addr_ext);
    }
    else if (target_ecu->protocal == PROTOCAL_SCP) {
        pl_system("mkdir " SCP_LOCAL_PATH);
        pl_system("touch " SCP_LOCAL_PATH SCP_CMD_FINISH);

        return scp_file_transfer(target_ecu->addr, SCP_LOCAL_PATH SCP_CMD_FINISH);
    }
}

static error_t inotify_watch(char * path, int32_t *pfd, int32_t *pwd)
{
    int32_t length, i = 0;
    int32_t fd;
    int32_t wd;

    fd = inotify_init();

    if ( fd < 0 ) {
        printf_it(LOG_ERROR, "inotify_init: %s", strerror(errno));
        return ERR_NOK;
    }

    wd = inotify_add_watch( fd, path, IN_CLOSE);
    if (wd < 0) {
        printf_it(LOG_ERROR, "Cannot watch:%s", strerror(errno) );
        ( void ) close( fd );
        return ERR_NOK;
    }

    if (pfd != NULL)
        *pfd = fd;
    if (pwd != NULL)
        *pwd = wd;

    printf_it(LOG_TRACE, "start to monitor dir %s", path);

    return ERR_OK;
}

static error_t inotify_get_event(int32_t fd, int32_t wd, char *event_str)
{
    int32_t length, i = 0;
    char buffer[4096];
    error_t err = ERR_NOK;
    
    length = read( fd, buffer, sizeof(buffer) );

    printf_it(LOG_TRACE, "new events! len=%d", length);

    if ( length < 0 ) {
        printf_it(LOG_ERROR, "read() error:%s", strerror(errno) );
        goto _return;
    }

    i = 0; 
    while ( i < length ) {
        struct inotify_event *event = ( struct inotify_event * ) &buffer[i];
        if ( event->len ) {
            if ( event->mask & IN_CLOSE ) {
                if ( event->mask & IN_ISDIR ) {
                    printf_it(LOG_INFO, "The directory %s was modified.\n", event->name );
                }
                else {
                    if (event_str != NULL)
                        strcpy(event_str, event->name);

                    err = ERR_OK;
                    printf_it(LOG_TRACE, "The file %s was received\n", event->name ); 
                }

            }
        }

        i += ( sizeof(struct inotify_event) ) + event->len;
    }
    
_return:
    ( void ) inotify_rm_watch( fd, wd );
    ( void ) close( fd );
    
    return err;

}


static error_t str2hex(char *ascii, uint8_t *hex)
{
    uint8_t number = 0;

    if (ascii == NULL) {
        *hex = 0;
        return ERR_NOK;
    }
    
    if ((ascii[0] >= 'a') && (ascii[0] <= 'f'))
        number = ascii[0] - 'a' + 10;
    else if ((ascii[0] >= 'A') && (ascii[0] <= 'F'))
        number = ascii[0] - 'A' + 10;
    else if ((ascii[0] >= '0') && (ascii[0] <= '9'))
        number = ascii[0] - '0';

    number = number << 4;

    if ((ascii[1] >= 'a') && (ascii[1] <= 'f'))
        number += ascii[1] - 'a' + 10;
    else if ((ascii[1] >= 'A') && (ascii[1] <= 'F'))
        number += ascii[1] - 'A' + 10;
    else if ((ascii[1] >= '0') && (ascii[1] <= '9'))
        number += ascii[1] - '0';

    if (hex != NULL)
        *hex = number;

    return ERR_OK;
}

/*
    RSP: rsp_err-??_status-??_progress-??
    ??: hex number, lowercase 
    err: 0=ok, 1=nok
    status: see fota_secu_state 
    progress: HEX 00 - 64
*/
static error_t parse_scp_event (char *rsp, uint8_t *error, uint8_t *status, uint8_t *progress)
{   
    char *str, *perror, *pstatus, *pprogress;

    if (rsp == NULL) 
        return ERR_NOK;

    perror = NULL;
    pstatus = NULL;
    pprogress = NULL;

    if ((str = strstr(rsp, "rsp_")) != NULL) {
        if ((str = strstr(rsp, "err-")) != NULL)
            perror = str + strlen("err-");
        
        if ((str = strstr(rsp, "status-")) != NULL)
            pstatus = str + strlen("status-");

        if ((str = strstr(rsp, "progress-")) != NULL)
            pprogress = str + strlen("progress-");

        str2hex(perror, error);
        str2hex(pstatus, status);
        str2hex(pprogress, progress);

        printf_it(LOG_INFO, "error=%d, status=%d, progress=%d", *error, *status, *progress);

        return ERR_OK;
    }
    else {
        return ERR_NOK;
    }
}


error_t fota_conn_cmd_get_status(fota_ecu_t *target_ecu, uint8_t *status, uint8_t *progress)
{
    uint8_t error = 0;

    printf_it(LOG_INFO, "get ecu %s state...", target_ecu->hw_id);

    if (target_ecu->protocal == PROTOCAL_UDS) {
        uds_cmd_get_ecu_status(target_ecu->addr, target_ecu->addr_ext, status, progress);
    }
    else if (target_ecu->protocal == PROTOCAL_SCP) {
        uint8_t rsponse[128];
        int32_t fd, wd;
        
        pl_system("mkdir " SCP_LOCAL_PATH);
        pl_system("mkdir " SCP_WATCH_PATH);
        pl_system("touch " SCP_LOCAL_PATH SCP_CMD_GET_STATUS);

        /* setup watch points */
        if (inotify_watch(SCP_WATCH_PATH, &fd, &wd) == ERR_OK) {
            /* request status */
            if (scp_file_transfer(target_ecu->addr, SCP_LOCAL_PATH SCP_CMD_GET_STATUS) == ERR_OK)
            {
                if (inotify_get_event(fd, wd, rsponse) == ERR_OK) {
                    return parse_scp_event(rsponse, &error, status, progress);
                }
            }
        }

        return ERR_NOK;
    }
}




