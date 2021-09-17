/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <fcntl.h>

#include "../fotav.h"
#include "it_dl.h"
#include "../http_client/http_client.h"
#include "../init/fota_config.h"

//#include "base64.h"
mqd_t mq_dl_rsp;
uint8_t mq_it_dl_rsp_open_counts = 0;
unsigned char b64_chr[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
unsigned int b64_encode(const unsigned char* in, unsigned int in_len, unsigned char* out) {

	unsigned int i=0, j=0, k=0, s[3];
	
	for (i=0;i<in_len;i++) {
		s[j++]=*(in+i);
		if (j==3) {
			out[k+0] = b64_chr[ (s[0]&255)>>2 ];
			out[k+1] = b64_chr[ ((s[0]&0x03)<<4)+((s[1]&0xF0)>>4) ];
			out[k+2] = b64_chr[ ((s[1]&0x0F)<<2)+((s[2]&0xC0)>>6) ];
			out[k+3] = b64_chr[ s[2]&0x3F ];
			j=0; k+=4;
		}
	}
	
	if (j) {
		if (j==1)
			s[1] = 0;
		out[k+0] = b64_chr[ (s[0]&255)>>2 ];
		out[k+1] = b64_chr[ ((s[0]&0x03)<<4)+((s[1]&0xF0)>>4) ];
		if (j==2)
			out[k+2] = b64_chr[ ((s[1]&0x0F)<<2) ];
		else
			out[k+2] = '=';
		out[k+3] = '=';
		k+=4;
	}

	out[k] = '\0';
	
	return k;
}


unsigned char *bin_to_strhex(const unsigned char *bin, unsigned int binsz,
                                  unsigned char **result)
{
  unsigned char hex_str[]= "0123456789abcdef";
  unsigned int  i;

  if (!(*result = (unsigned char *)malloc(binsz * 2 + 1)))
    return (NULL);

  (*result)[binsz * 2] = 0;

  if (!binsz)
    return (NULL);

  for (i = 0; i < binsz; i++)
    {
      (*result)[i * 2 + 0] = hex_str[(bin[i] >> 4) & 0x0F];
      (*result)[i * 2 + 1] = hex_str[(bin[i]     ) & 0x0F];
    }
  return (*result);
}



void file_dl(const int8_t *ptr, int32_t size, int32_t acc_size, int32_t fsize, int8_t *filename)
{
	//mqd_t mq_dl_rsp;
	FILE *pfd;
	ssize_t written;
    uint8_t progress;
   
    uint8_t mq_buf[128];
    int32_t prog_index;

	printf_ut(LOG_INFO, "opening file %s\n", filename);
	
	if (acc_size == 0) {
		pfd = fopen(filename, "w+");
	} else {
		pfd = fopen(filename, "a");
	}

	if (pfd == NULL) {
		printf_it(LOG_ERROR, "file_dl open fail, %d\n", pfd);
		return;
	}

	written = fwrite(ptr, 1, size, pfd);
	if ((written < 0) || (written != size))
		printf_it(LOG_ERROR, "file_dl write fail, %d\n", written);

	fclose(pfd);

    if (fsize >= 0) {
		if(mq_it_dl_rsp_open_counts == 0)
       {
		  if ((mq_dl_rsp = mq_open("/mq_it_dl_rsp", O_WRONLY)) < 0)
		    printf_it(LOG_ERROR, "mq_open mq_it_dl_rsp fail");
			mq_it_dl_rsp_open_counts +=1;
	   } 
	  
        progress = (uint8_t)((acc_size + size) * 100 / (acc_size + fsize));

        snprintf(mq_buf, 128, "download=ing, prog=%d", progress);
        prog_index = strlen(mq_buf);
        mq_buf[prog_index + 2] = progress;

        printf_ut(LOG_INFO, "%s: %d:%d %d", filename, acc_size + size, acc_size + fsize, progress);
        
        if (mq_send(mq_dl_rsp, mq_buf, sizeof(mq_buf), 0) < 0) {
			printf_it(LOG_ERROR, "mq_send error\n");
		}

		// if (mq_close(mq_dl_rsp) < 0) {
		// 	printf_it(LOG_ERROR, "mq_close error\n");
		// }

    }
	
}
void file_pass(const int8_t *ptr, int32_t size, int32_t acc_size, int32_t finish, int8_t *filename)
{
	
	
}

void* dl_thread(void *arg)
{
	mqd_t mq_dl_cmd, mq_dl_rsp;
	uint8_t mq_buf[128];
	uint8_t mq_cmd[128];
	int32_t mq_len;
	int8_t *pstr, *param1, *param2, *param3,*param4,*param5;
	int8_t *fname;

	FILE *fp;
	int32_t fsize;
	int8_t path[128];
	int8_t *fdata;
    int8_t url[128];


	/* open the queue */
	if ((mq_dl_cmd = mq_open("/mq_it_dl_cmd", O_RDONLY)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_it_dl_cmd fail\n");

	if ((mq_dl_rsp = mq_open("/mq_it_dl_rsp", O_WRONLY)) < 0)
		printf_it(LOG_ERROR, "mq_open mq_it_dl_rsp fail\n");


	printf_it(LOG_INFO, "DL thread starts...");
	while(true) {
		/* waiting for msg */
		memset(&mq_buf, 0, sizeof(mq_buf));
		if ((mq_len = mq_receive(mq_dl_cmd, mq_buf, sizeof(mq_buf), NULL)) < 0) {
			printf_it(LOG_ERROR, "mq_receive error\n");
		}

		/* file upload */
		/* param1 = filename, param2 = url, param3 = case no */
		if ((pstr = strstr(mq_buf, "ul=")) != NULL) {
			/* upload */
			param1 = pstr + strlen("ul=");
			if ((pstr = strstr(pstr, ", ")) != NULL) {
				*pstr = 0;
				pstr += 2; /* jump over ", " */
			} else {
				printf_it(LOG_ERROR, "can not find ', ' in %s\n", mq_buf);
				goto _DL_ERROR;
			}
			/* param2 */
			if ((pstr = strstr(pstr, "url=")) != NULL) {
				param2 = pstr + strlen("url=");
			} else {
				printf_it(LOG_ERROR, "can not find 'case' in %s\n", mq_buf);
				goto _DL_ERROR;
			}

			if ((pstr = strstr(pstr, ", ")) != NULL) {
				*pstr = 0;
				pstr += 2; /* jump over ", " */
			} else {
				printf_it(LOG_ERROR, "can not find 'case' in %s\n", mq_buf);
				goto _DL_ERROR;
			}
			/* param3 */
			if ((pstr = strstr(pstr, "rn=")) != NULL) {
				param3 = pstr + strlen("rn=");
			} else {
				printf_it(LOG_ERROR, "can not find 'rname' in %s\n", mq_buf);
				goto _DL_ERROR;
			}

			/* open file */
			strcpy(path, param1);
			printf_it(LOG_INFO, "upload file %s\n", path);
			if ((fp = fopen(path, "r")) == NULL) {
				printf_it(LOG_ERROR, "can not open file %s\n", path);
				goto _DL_ERROR;
			}

			/* get file size */
			fseek(fp, 0L, SEEK_END);
			fsize = ftell(fp);

			if ((fdata = malloc(fsize)) == NULL) {
				printf_it(LOG_ERROR, "can not alloc memory size %d\n", fsize);
				fclose(fp);
				goto _DL_ERROR;
			}

			printf_it(LOG_INFO, "reading file %s, size %d\n", path, fsize);
			fseek(fp, 0L, SEEK_SET);
			if (fsize != fread(fdata, 1, fsize, fp)) {
				printf_it(LOG_ERROR, "fread error %d\n", fsize);
				fclose(fp);
				free(fdata);
				goto _DL_ERROR;
			}

			printf_it(LOG_INFO, "uploading file %s\n", path);
			if (ERR_NOK == http_upload(param2, param3, fdata, fsize)) {
				printf_it(LOG_ERROR, "upload file error\n");
				fclose(fp);
				free(fdata);
				goto _DL_ERROR;
			}

			printf_it(LOG_INFO, "upload file %s ok\n", path);

			/* response msg */
			snprintf(mq_cmd, 128, "upload=ok");
			fclose(fp);
			free(fdata);
			goto _DL_DONE;
		}

		/* submitmanifest rpc */
		/* param1 = rpc command, param2 = vin,
		 * param3 = primary_ecu_esrial, para4 = case no */
		else if ((pstr = strstr(mq_buf, "method=")) != NULL) {
			
			param1 = pstr + strlen("method=");
			if ((pstr = strstr(pstr, ", ")) != NULL) {
				*pstr = 0;
				pstr += 2; /* jump over ", " */
			} else {
				printf_it(LOG_ERROR, "can not find ', ' in %s\n", mq_buf);
				goto _DL_ERROR;
			}
			/* param2 */
			if ((pstr = strstr(pstr,"vin=")) != NULL) {
				param2 = pstr + strlen("vin=");
			} else {
				printf_it(LOG_ERROR, "can not find 'vin' in %s\n", mq_buf);
				goto _DL_ERROR;
			}
			if ((pstr = strstr(pstr, ", ")) != NULL) {
				*pstr = 0;
				pstr += 2; /* jump over ", " */
			} else {
				printf_it(LOG_ERROR, "can not find 'case' in %s\n", mq_buf);
				goto _DL_ERROR;
			}
			/* param3 */
			if ((pstr = strstr(pstr, "esrial=")) != NULL) {
				param3 = pstr + strlen("esrial=");
				*pstr = 0;
				pstr += 2; /* jump over ", " */
			
			} else {
				printf_it(LOG_ERROR, "can not find 'esrial' in %s\n", mq_buf);
				goto _DL_ERROR;
			}

			if ((pstr = strstr(pstr, ", ")) != NULL) {
				*pstr = 0;
				pstr += 2; /* jump over ", " */
			} else {
				printf_it(LOG_ERROR, "can not find 'case' in %s\n", mq_buf);
				goto _DL_ERROR;
			}
			/* param4*/
			if ((pstr = strstr(pstr, "case")) != NULL) {
				param4 = pstr;
				
			} else {
				printf_it(LOG_ERROR, "can not find 'case' in %s\n", mq_buf);
				goto _DL_ERROR;
			}

			/* param1 = rpc command, param2 = vin,
		    * param3 = primary_ecu_esrial, para4 = case no */
			uint8_t *manifest_path = repo_get_manifest_path();
			
			/* param1 = rpc command, param2 = vin, param3 = case no */
            snprintf(url, sizeof(url), "%s:30501", get_factory_dr_url(0));
			if (ERR_NOK == http_rpc_method(url, file_pass,\
											manifest_path, param1, \
										    "string", param2, \
											"string", param3, \
											"base64", manifest_path,NULL \
											)) {
				printf_it(LOG_ERROR, "rpc response error: url=%s\n", url);
				goto _DL_ERROR;
			}
		
			/* response msg */
			snprintf(mq_cmd, 128, "xmlrpc=ok, xml=rpc_rsp_%s.xml", param1);
			goto _DL_DONE;
		}

#if 0
		/* xml rpc */
		/* param1 = rpc command, param2 = vin, param3 = case no */
		else if ((pstr = strstr(mq_buf, "xmlrpc=")) != NULL) {
			param1 = pstr + strlen("smlrpc=");
			if ((pstr = strstr(pstr, ", ")) != NULL) {
				*pstr = 0;
				pstr += 2; /* jump over ", " */
			} else {
				printf_it(LOG_ERROR, "can not find ', ' in %s\n", mq_buf);
				goto _DL_ERROR;
			}
			/* param2 */
			if ((pstr = strstr(pstr, "vin=")) != NULL) {
				param2 = pstr + strlen("vin=");
			} else {
				printf_it(LOG_ERROR, "can not find 'vin' in %s\n", mq_buf);
				goto _DL_ERROR;
			}

			/* param3 */
			if ((pstr = strstr(pstr, ", ")) != NULL) {
				*pstr = 0;
				pstr += 2; /* jump over ", " */
			} else {
				printf_it(LOG_ERROR, "can not find 'case' in %s\n", mq_buf);
				goto _DL_ERROR;
			}

			if ((pstr = strstr(pstr, "case")) != NULL) {
				param3 = pstr;
			} else {
				printf_it(LOG_ERROR, "can not find 'case' in %s\n", mq_buf);
				goto _DL_ERROR;
			}

			snprintf(path, 128, 
				TEST_DIR "/%s" "/dl/rpc_rsp_%s.xml",
				param3, param1);
			/* param1 = rpc command, param2 = vin, param3 = case no */
            snprintf(url, sizeof(url), "%s:30501", get_factory_dr_url(0));
			if (ERR_NOK == http_rpc_method(url, file_dl, path, param1, "string", param2, NULL)) {
				printf_it(LOG_ERROR, "rpc response error, url=%s\n", url);
				goto _DL_ERROR;
			}

			/* response msg */
			snprintf(mq_cmd, 128, "xmlrpc=ok, xml=rpc_rsp_%s.xml", param1);
			goto _DL_DONE;
		}
#endif
		/* file download */
		/* param1 = URL, param2 = store path, param3 = case no */
		else if ((pstr = strstr(mq_buf, "download=")) != NULL) {
			param1 = pstr + strlen("download=");
			if ((pstr = strstr(param1, ", ")) != NULL) {
				*pstr = 0;
				pstr += 2; /* jump over ", " */
			} else {
				printf_it(LOG_ERROR, "can not find ', ' in %s\n", mq_buf);
				goto _DL_ERROR;
			}

			/* param2 */
			if ((pstr = strstr(pstr, "store=")) != NULL) {
				param2 = pstr + strlen("store=");
			} else {
				printf_it(LOG_ERROR, "can not find 'store=' in %s\n", mq_buf);
				goto _DL_ERROR;
			}

			if (((pstr = strstr(param2, ", ")) != NULL) || 
                ((pstr = strstr(param2, ",")) != NULL)) {
				*pstr = 0;
				pstr += 2; /* jump over ", " */
			} else {
				printf_it(LOG_ERROR, "can not find ',' in %s\n", mq_buf);
				goto _DL_ERROR;
			}

			if ((pstr = strrchr(param1, '/')) != NULL) {
				pstr += 1; /* jump over '/' */
			}

			if (strstr(param2, "metadata") != NULL) {
				/* will download metadata
				 * file name pattern: n.root.json 
				 * n is option. is n is present, remove it */
				fname = strchr(pstr, '.');
				if (((void *)fname == (void *)strrchr(pstr, '.')) 
					&& (fname != NULL)) {
					fname = pstr;
				} else if (fname != NULL) {
					fname += 1; /* jump over '.' */
				}
			} else {
				fname = pstr;
			}

			snprintf(path, 128, 
				"%s%s",
				param2, fname);
			if (ERR_NOK == http_download(param1, file_dl, path)) {
				printf_it(LOG_ERROR, "download response error\n");
				goto _DL_ERROR;
			}

			/* response msg */
			snprintf(mq_cmd, 128, "download=ok, name=%s", pstr);
			goto _DL_DONE;
		}

		/* processing */
		// http_rpc_download("10.193.248.99:30309", "BCU1.2.txt", file_dl, "/opt/xuewei/fota/dl/BCU1.2.txt");

_DL_ERROR:
		snprintf(mq_cmd, 128, "error encountered");

_DL_DONE:
		/* send response msg */
		if (mq_send(mq_dl_rsp, mq_cmd, sizeof(mq_cmd), 0) < 0) {
			printf_it(LOG_ERROR, "mq_send error\n");
		}
	}
}
