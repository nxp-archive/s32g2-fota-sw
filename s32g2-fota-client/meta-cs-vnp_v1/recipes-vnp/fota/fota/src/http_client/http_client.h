/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/

#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#define HTTP_FIELD_SIZE	(512)
#define HTTP_READ_SIZE	(64)
#define HTTP_SESSION_R_BUFF_SIZE (2048)

#define HTTP_RX_DONE	((int16_t)200)
#define HTTP_STATUS_OK	(200)

typedef struct {
	int8_t method[8];
	int32_t status;
	int8_t content_type[HTTP_FIELD_SIZE];
	int32_t content_length;
	bool chunked;
	bool close;
	int8_t location[HTTP_FIELD_SIZE];
	int8_t referrer[HTTP_FIELD_SIZE];
	int8_t cookie[HTTP_FIELD_SIZE];
	int8_t boundary[HTTP_FIELD_SIZE];
} http_head_t;

typedef struct {
	int32_t https;
	int8_t host[256];
	int8_t port[8];
	int8_t path[HTTP_FIELD_SIZE];
} http_url_t;

typedef struct {
	int32_t socket_desc;	/**< socket FD */
	http_url_t url;		/**< URL attributes */
	http_head_t request;	/**< request header attributes */
	http_head_t response;	/**< response header attibutes */
	int32_t length;		/**< For non-chunk mode, this is the contend length; For chunk mode, this is temp chunk size */
	int8_t r_buf[HTTP_SESSION_R_BUFF_SIZE];	/**< temp data buffer for response package parsing */
	int32_t r_len;			/**< length of r_buf */
	bool header_end;	/**< temp variable for response package parsing */
	int32_t recv_len;	/**< length of received data of http response body */
	store_cb store;		/**< callback function for received data storage */
	int8_t *store_param;
} http_client_t;

error_t http_init(http_client_t *hc, store_cb storage, int8_t *param);
error_t http_close(http_client_t *hc);
error_t http_open(http_client_t *hc, const int8_t *url);
error_t http_get(http_client_t *hc, const int8_t *url);
error_t http_post(http_client_t *hc, const int8_t *url, int8_t *data);

error_t http_write_header(http_client_t *hc);
int32_t http_write(http_client_t *hc, const int8_t *data, int32_t len);
int32_t http_write_end(http_client_t *hc);
int32_t http_recv(http_client_t *hc);

error_t http_rpc(http_client_t *hc, const int8_t *url, const int8_t *method, va_list argp);

#endif //HTTPS_CLIENT_HTTPS_H
