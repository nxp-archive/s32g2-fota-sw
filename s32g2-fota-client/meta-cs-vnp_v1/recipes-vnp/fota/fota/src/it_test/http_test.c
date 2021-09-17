/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "../../src/fotav.h"
#include "../../src/http_client/http_client.h"
#include "http_test.h"
#include "../init/fota_config.h"
#include "it_dl.h"
uint8_t test_resp_data[1024] = 
"HTTP/1.0 200 OK\r\n"
"Server: BaseHTTP/0.3 Python/2.7.15rc1\r\n"
"Date: Tue, 21 May 2019 01:44:38 GMT\r\n"
"Content-type: text/xml\r\n"
"Content-length: 152\r\n"
"\r\n"
"<?xml version='1.0'?>\n"
"<methodResponse>\n"
"<params>\n"
"<param>\n"
"<value><base64>\n"
"QkNVIDEuMiAtLSB1cGRhdGUK\n"
"</base64></value>\n"
"</param>\n"
"</params>\n"
"</methodResponse>\n";

uint8_t test_resp_data_chunked[1024] = 
"HTTP/1.0 200 OK\r\n"
"Server: BaseHTTP/0.3 Python/2.7.15rc1\r\n"
"Date: Tue, 21 May 2019 01:44:38 GMT\r\n"
"Content-type: text/xml\r\n"
"Transfer-Encoding: chunked\r\n"
"\r\n"
"25\r\n"
"This is the data in the first chunk, \r\n"
"1B\r\n"
"and this is the second one.\r\n"
"0\r\n\r\n";

uint8_t test_resp_data_chunked_ref [] = 
"This is the data in the first chunk, "
"and this is the second one.";

uint32_t test_http_read_cnt = 0;
uint32_t test_http_read_pos = 0;
uint8_t http_rx_buf[8 * 1024];

int32_t test_http_read(http_client_t *hc, int8_t *data)
{
	int32_t cnt = 0;

	printf("TEST: test_http_read_cnt=%d \n", test_http_read_cnt);

	if (test_http_read_cnt == 0) {
		cnt = strlen("HTTP/1.0 200 OK\r\n");
		memcpy(&hc->r_buf[hc->r_len], &test_resp_data[test_http_read_pos], cnt);
	}
	else if (test_http_read_cnt == 1) {
		cnt = strlen("HTTP/1.0 200 OK\r\n"
			"Server: BaseHTTP/0.3 Python/2.7.15rc1\r\n"
			"Date: Tue, 21 May 2019 01:44:38 GMT\r\n"
			"Content-type: text/xml\r\n"
			"Content-length: 15") - test_http_read_pos;

		memcpy(&hc->r_buf[hc->r_len], &test_resp_data[test_http_read_pos], cnt);
	}
	else if (test_http_read_cnt == 2) {
		cnt = strlen("HTTP/1.0 200 OK\r\n"
			"Server: BaseHTTP/0.3 Python/2.7.15rc1\r\n"
			"Date: Tue, 21 May 2019 01:44:38 GMT\r\n"
			"Content-type: text/xml\r\n"
			"Content-length: 152\r\n"
			"\r\n"
			"<?xml version='1.0'?>\n"
			"<methodResponse>") - test_http_read_pos;

		memcpy(&hc->r_buf[hc->r_len], &test_resp_data[test_http_read_pos], cnt);
	} else if (test_http_read_cnt == 3) {
		cnt = strlen("HTTP/1.0 200 OK\r\n"
			"Server: BaseHTTP/0.3 Python/2.7.15rc1\r\n"
			"Date: Tue, 21 May 2019 01:44:38 GMT\r\n"
			"Content-type: text/xml\r\n"
			"Content-length: 152\r\n"
			"\r\n"
			"<?xml version='1.0'?>\n"
			"<methodResponse>\n"
			"<params>\n"
			"<param>\n"
			"<value><base64>\n"
			"QkNVIDEuMiAtLSB1cGRhdGUK\n"
			"</base64></value>\n"
			"</param>\n"
			"</params>\n") - test_http_read_pos;

		memcpy(&hc->r_buf[hc->r_len], &test_resp_data[test_http_read_pos], cnt);
	} else if (test_http_read_cnt == 4) {
		cnt = strlen("HTTP/1.0 200 OK\r\n"
			"Server: BaseHTTP/0.3 Python/2.7.15rc1\r\n"
			"Date: Tue, 21 May 2019 01:44:38 GMT\r\n"
			"Content-type: text/xml\r\n"
			"Content-length: 152\r\n"
			"\r\n"
			"<?xml version='1.0'?>\n"
			"<methodResponse>\n"
			"<params>\n"
			"<param>\n"
			"<value><base64>\n"
			"QkNVIDEuMiAtLSB1cGRhdGUK\n"
			"</base64></value>\n"
			"</param>\n"
			"</params>\n"
			"</methodResponse>\n") - test_http_read_pos;

		memcpy(&hc->r_buf[hc->r_len], &test_resp_data[test_http_read_pos], cnt);
	}
	
	/* case 2 */
	if (test_http_read_cnt == 10) {
		cnt = strlen("HTTP/1.0 200 OK\r\n"
			"Server: BaseHTTP/0.3 Python/2.7.15rc1\r\n"
			"Date: Tue, 21 May 2019 01:44:38 GMT\r\n"
			"Content-type: text/xml\r");
		memcpy(&hc->r_buf[hc->r_len], &test_resp_data_chunked[test_http_read_pos], cnt);
	} else if (test_http_read_cnt == 11) {
		cnt = strlen("HTTP/1.0 200 OK\r\n"
			"Server: BaseHTTP/0.3 Python/2.7.15rc1\r\n"
			"Date: Tue, 21 May 2019 01:44:38 GMT\r\n"
			"Content-type: text/xml\r\n"
			"Transfer-Encoding: chunked\r\n"
			"\r") - test_http_read_pos;
		memcpy(&hc->r_buf[hc->r_len], &test_resp_data_chunked[test_http_read_pos], cnt);
	} else if (test_http_read_cnt == 12) {
		cnt = strlen("HTTP/1.0 200 OK\r\n"
			"Server: BaseHTTP/0.3 Python/2.7.15rc1\r\n"
			"Date: Tue, 21 May 2019 01:44:38 GMT\r\n"
			"Content-type: text/xml\r\n"
			"Transfer-Encoding: chunked\r\n"
			"\r\n"
			"25\r") - test_http_read_pos;
		memcpy(&hc->r_buf[hc->r_len], &test_resp_data_chunked[test_http_read_pos], cnt);
	} else if (test_http_read_cnt == 13) {
		cnt = strlen("HTTP/1.0 200 OK\r\n"
			"Server: BaseHTTP/0.3 Python/2.7.15rc1\r\n"
			"Date: Tue, 21 May 2019 01:44:38 GMT\r\n"
			"Content-type: text/xml\r\n"
			"Transfer-Encoding: chunked\r\n"
			"\r\n"
			"25\r\n"
			"This is the data ") - test_http_read_pos;
		memcpy(&hc->r_buf[hc->r_len], &test_resp_data_chunked[test_http_read_pos], cnt);
	} else if (test_http_read_cnt == 14) {
		cnt = strlen("HTTP/1.0 200 OK\r\n"
			"Server: BaseHTTP/0.3 Python/2.7.15rc1\r\n"
			"Date: Tue, 21 May 2019 01:44:38 GMT\r\n"
			"Content-type: text/xml\r\n"
			"Transfer-Encoding: chunked\r\n"
			"\r\n"
			"25\r\n"
			"This is the data in the first chunk, \r") - test_http_read_pos;
		memcpy(&hc->r_buf[hc->r_len], &test_resp_data_chunked[test_http_read_pos], cnt);
	} else if (test_http_read_cnt == 15) {
		cnt = strlen("HTTP/1.0 200 OK\r\n"
			"Server: BaseHTTP/0.3 Python/2.7.15rc1\r\n"
			"Date: Tue, 21 May 2019 01:44:38 GMT\r\n"
			"Content-type: text/xml\r\n"
			"Transfer-Encoding: chunked\r\n"
			"\r\n"
			"25\r\n"
			"This is the data in the first chunk, \r\n"
			"1A\r\n") - test_http_read_pos;
		memcpy(&hc->r_buf[hc->r_len], &test_resp_data_chunked[test_http_read_pos], cnt);
	} else if (test_http_read_cnt == 16) {
		cnt = strlen("HTTP/1.0 200 OK\r\n"
			"Server: BaseHTTP/0.3 Python/2.7.15rc1\r\n"
			"Date: Tue, 21 May 2019 01:44:38 GMT\r\n"
			"Content-type: text/xml\r\n"
			"Transfer-Encoding: chunked\r\n"
			"\r\n"
			"25\r\n"
			"This is the data in the first chunk, \r\n"
			"1A\r\n"
			"and this is the se") - test_http_read_pos;
		memcpy(&hc->r_buf[hc->r_len], &test_resp_data_chunked[test_http_read_pos], cnt);
	} else if (test_http_read_cnt == 17) {
		cnt = strlen("HTTP/1.0 200 OK\r\n"
			"Server: BaseHTTP/0.3 Python/2.7.15rc1\r\n"
			"Date: Tue, 21 May 2019 01:44:38 GMT\r\n"
			"Content-type: text/xml\r\n"
			"Transfer-Encoding: chunked\r\n"
			"\r\n"
			"25\r\n"
			"This is the data in the first chunk, \r\n"
			"1A\r\n"
			"and this is the second one.\r\n") - test_http_read_pos;
		memcpy(&hc->r_buf[hc->r_len], &test_resp_data_chunked[test_http_read_pos], cnt);
	} else if (test_http_read_cnt == 18) {
		cnt = strlen("HTTP/1.0 200 OK\r\n"
			"Server: BaseHTTP/0.3 Python/2.7.15rc1\r\n"
			"Date: Tue, 21 May 2019 01:44:38 GMT\r\n"
			"Content-type: text/xml\r\n"
			"Transfer-Encoding: chunked\r\n"
			"\r\n"
			"25\r\n"
			"This is the data in the first chunk, \r\n"
			"1A\r\n"
			"and this is the second one.\r\n"
			"0\r\n") - test_http_read_pos;
		memcpy(&hc->r_buf[hc->r_len], &test_resp_data_chunked[test_http_read_pos], cnt);
	} else if (test_http_read_cnt == 19) {
		cnt = strlen("HTTP/1.0 200 OK\r\n"
			"Server: BaseHTTP/0.3 Python/2.7.15rc1\r\n"
			"Date: Tue, 21 May 2019 01:44:38 GMT\r\n"
			"Content-type: text/xml\r\n"
			"Transfer-Encoding: chunked\r\n"
			"\r\n"
			"25\r\n"
			"This is the data in the first chunk, \r\n"
			"1A\r\n"
			"and this is the second one.\r\n"
			"0\r\n\r\n") - test_http_read_pos;
		memcpy(&hc->r_buf[hc->r_len], &test_resp_data_chunked[test_http_read_pos], cnt);
	}

	test_http_read_cnt++;
	test_http_read_pos += cnt;
	return cnt;
}

void data_store_cb(const int8_t *ptr, int32_t size, int32_t acc_size, int32_t finish, int8_t *param)
{
	int8_t *buf = param;
	memcpy(&buf[acc_size], ptr, size);
	printf("UT: data_store_cb() acc_len=%d, finished %d%::%s\n", acc_size, buf, finish);
}



void file_pass_(const int8_t *ptr, int32_t size, int32_t acc_size, int32_t finish, int8_t *filename)
{
	
	
}
bool http_client_test(void)
{
	int8_t url[128]="http://10.193.248.173:30501";
	int8_t* param1="submit_vehicle_manifest";
	int8_t* param2="S32G-RDB2";
	int8_t* param3="GATEWAY";
   //repo_uptane/metadata/dr/download/
	int8_t path[128] = "root.der";
    int8_t* param1_dl = "http://10.193.248.173:30301/metadata/root.der";

	/* param1 = rpc command, param2 = vin,
	* param3 = primary_ecu_esrial, para4 = case no */
	uint8_t *manifest_path = repo_get_manifest_path();
    printf("manifest_path:%s\n",manifest_path);	
	/* param1 = rpc command, param2 = vin, param3 = case no */
    //snprintf(url, sizeof(url), "%s:30501", get_factory_dr_url(0));
	printf("url:%s\n",url);
	if (ERR_NOK == http_rpc_method(url, file_pass_,\
											manifest_path, param1, \
										    "string", param2, \
											"string", param3, \
											"base64", manifest_path,NULL \
											)) {
				printf_it(LOG_ERROR, "rpc response error: url=%s\n", url);
			}
	else
	{
		printf_it(LOG_INFO, "rpc connect success: url=%s\n", url);
	}
	printf("********************************\n");
	printf("param1:%s\n",param1_dl);
	printf("path:%s\n",path);
	if (ERR_NOK == http_download(param1_dl, file_dl, path))
	 {
		printf_it(LOG_ERROR, "download response error\n");
	 }
	else
	{
		printf_it(LOG_INFO, "download file success\n");
	}


	return true;

}

