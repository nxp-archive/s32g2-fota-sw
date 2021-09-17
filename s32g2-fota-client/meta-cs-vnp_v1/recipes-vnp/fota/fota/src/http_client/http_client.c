/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#if 0
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#include "../config.h"
#include "../fotav.h"
#else
#include "pl_types.h"
#include "pl_tcpip.h"
#include "pl_stdlib.h"
#include "pl_string.h"
#include "pl_stdio.h"
#include "pl_stdarg.h"
#endif
#include "../fotav.h"
#include "http_client.h"
#include "../base64/base64.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <errno.h>

bool http_read_time_out_flag = false;
/**
 * @brief brief
 *
 * @param 1 comments
 * @param 2 comments
 * @return comment
 *   @retval value
 * @see link
 * @attention attention
 * @warning warning
 */
error_t http_init(http_client_t *hc, store_cb storage, int8_t *param)
{
	memset(hc, 0, sizeof(http_client_t));
	if (storage != NULL) {
		hc->store = storage;
		hc->store_param = param;
	}
    	return ERR_OK;
}

static int8_t *strtoken(int8_t *src, int8_t *dst, int32_t size)
{
	int8_t *p, *st, *ed;
	int32_t len = 0;

	// l-trim
	p = src;

	while (true) {
		if ((*p == '\n') || (*p == 0))
			return NULL; /* value is not exists */
		if ((*p != ' ') && (*p != '\t'))
			break;
		p++;
	}

	st = p;
	while (true) {
		ed = p;
		if (*p == ' ') {
			p++;
			break;
		}
		if ((*p == '\n') || (*p == 0))
			break;
		p++;
	}

	// r-trim
	while (true) {
		ed--;
		if (st == ed)
			break;
		if ((*ed != ' ') && (*ed != '\t'))
			break;
	}

	len = (int32_t)(ed - st + 1);
	if ((size > 0) && (len >= size))
		len = size - 1;

	strncpy(dst, st, len);
	dst[len] = 0;

	return p;
}

/**
 * @brief brief
 *
 * @param 1 comments
 * @param 2 comments
 * @return comment
 *   @retval value
 * @see link
 * @attention attention
 * @warning warning
 */
static int32_t parse_url(int8_t *src_url, int32_t *https, int8_t *host, int8_t *port, int8_t *url)
{
	int8_t *p1, *p2;
	int8_t str[1024];

	memset(str, 0, sizeof(str));

	/* protocol */
	if (strncmp(src_url, "http://", 7) == 0) {
		p1 = &src_url[7];
		*https = 0;
	} else if (strncmp(src_url, "https://", 8) == 0) {
		p1 = &src_url[8];
		*https = 1;
	} else {
		p1 = &src_url[0];
		*https = 0;
	}

	if ((p2 = strstr(p1, "/")) == NULL) {
		sprintf(str, "%s", p1);
		sprintf(url, "/");
	} else {
		strncpy(str, p1, p2 - p1);
		snprintf(url, 256, "%s", p2);
	}

	if ((p1 = strstr(str, ":")) != NULL) {
		*p1 = 0;
		snprintf(host, 256, "%s", str);
		snprintf(port, 8, "%s", p1 + 1);
	} else {
		snprintf(host, 256, "%s", str);

		if (*https == 0)
			snprintf(port, 5, "80");
		else
			snprintf(port, 5, "443");
	}

	return ERR_OK;
}

/*---------------------------------------------------------------------*/
static int32_t http_header_tokens(http_client_t *hc, int8_t *param)
{
	int8_t *token;
	int8_t t1[256], t2[256];
	int32_t len;

	token = param;

	if ((token = strtoken(token, t1, 256)) == 0)
		return ERR_NOK;
	if ((token = strtoken(token, t2, 256)) == 0)
		return ERR_NOK;

	if (strncasecmp(t1, "HTTP", 4) == 0) {
		hc->response.status = atoi(t2);
	} else if (strncasecmp(t1, "set-cookie:", 11) == 0) {
		snprintf(hc->response.cookie, 512, "%s", t2);
	} else if (strncasecmp(t1, "location:", 9) == 0) {
		len = (int32_t)strlen(t2);
		strncpy(hc->response.location, t2, len);
		hc->response.location[len] = 0;
	} else if (strncasecmp(t1, "content-length:", 15) == 0) {
		hc->response.content_length = atoi(t2);
	} else if (strncasecmp(t1, "transfer-encoding:", 18) == 0) {
		if (strncasecmp(t2, "chunked", 7) == 0) {
			hc->response.chunked = true;
		}
	} else if (strncasecmp(t1, "connection:", 11) == 0) {
		if (strncasecmp(t2, "close", 5) == 0) {
			hc->response.close = true;
		}
	} else if (strncasecmp(t1, "content-type:", 13) == 0) {
		len = (int32_t)strlen(t2);
		strncpy(hc->response.content_type, t2, len);
		hc->response.content_type[len] = 0;
	}

	return ERR_OK;
}

/*---------------------------------------------------------------------*/
static int32_t http_header_parse(http_client_t *hc)
{
	int8_t *p1, *p2;
	int32_t len;

	if (hc->r_len <= 0)
		return ERR_NOK;

	p1 = hc->r_buf;
	while (1) {
	
		if (hc->header_end == false) {
			/* header parsing */
			if ((p2 = strstr(p1, "\r\n")) != NULL) {
				len = (int32_t)(p2 - p1);
				*p2 = 0;

				if (len > 0) {
					http_header_tokens(hc, p1);
					p1 = p2 + 2; /* skip CR+LF */
				} else {
					hc->header_end = true; // reach the header-end.

					p1 = p2 + 2; /* skip CR+LF */

					len = hc->r_len - (p1 - hc->r_buf);
					if (len > 0) {
						/* leave the body data in r_buf */
						memmove(hc->r_buf, p1, len);
						hc->r_buf[len] = 0;
						hc->r_len = len;
					} else {
						/* empty r_buf */
						hc->r_len = 0;
					}

					if (hc->response.chunked == true) {
						/* need further parsing in http body */
						hc->length = -1;
					} else {
						hc->length = hc->response.content_length;
					}

					break;
				}
			} else {
				/* header is not complete */
				len = hc->r_len - (p1 - hc->r_buf);
				if (len > 0) {
					/* keep the partial header data ... */
					memmove(hc->r_buf, p1, len);
					hc->r_buf[len] = 0;
					hc->r_len = len;
				} else {
					hc->r_len = 0;
				}

				break;
			}
		}
	}

	printf_ut(LOG_INFO, "header parser: length=%d, r_len=%d::%s", hc->length, hc->r_len, p1);

	return ERR_OK;
}

/**
 * @brief brief
 *
 * @param 1 comments
 * @param 2 comments
 * @return comment
 *   @retval value
 * @see link
 * @attention attention
 * @warning warning
 */
error_t http_body_parse(http_client_t *hc)
{
	int8_t *p1, *p2;
	int32_t len;

	if (hc->r_len <= 0)
		return ERR_NOK;

	p1 = hc->r_buf;

	while (true) {
		if (hc->response.chunked == true && hc->length == -1) {
			/* check if the chunked date is complete */
			len = hc->r_len - (p1 - hc->r_buf);
			if (len > 0) {
				if ((p2 = strstr(p1, "\r\n")) != NULL) {
					*p2 = 0;

					if (p2 == p1) {
						/* pattern: \r\n\r\n */
						hc->response.chunked = false;
						printf_ut(LOG_INFO, "http chunk receive end");
						return HTTP_RX_DONE;	/* all data received */
					}

					if ((hc->length = strtol(p1, NULL, 16)) > 0) {
						/* all chunk data end */
						hc->response.content_length += hc->length;
					} else {
						hc->length = -1;
					}

					p1 = p2 + 2; // skip CR+LF
				} else {
					// copy the remain data as chunked size ...
					memmove(hc->r_buf, p1, len);
					hc->r_buf[len] = 0;
					hc->r_len = len;
					hc->length = -1;
					break;
				}
			} else {
				hc->r_len = 0;
				break;
			}
		} else {
			if (hc->length > 0) {
				/* get data length */
				len = hc->r_len - (p1 - hc->r_buf);
				printf_ut(LOG_INFO, "http body parser start: length=%d, r_len=%d, len=%d::%s", hc->length, hc->r_len, len, p1);
				/* the rx data length more than the target? */
				if ((hc->response.chunked == true) && (len > hc->length)) {
					/* the data is complete, copy the data for response .. */
					if (hc->store != NULL)
						hc->store(p1, hc->length, hc->recv_len, -1, hc->store_param);
					hc->recv_len += hc->length;

					p1 += hc->length;	/* p1 should point to \r\n */
					len -= hc->length;

					hc->length = 0;

					if ((p2 = strstr(p1, "\r\n")) != NULL) {
						if (p2 == p1) {
							/* p1 points to \r\n */
							p1 += 2;	/* skip \r\n, pointing to next chunk */
							hc->length = -1;
						} else if (len > 0) {
							/* there is leaving data, but no room to store */
							memmove(hc->r_buf, p1, len);
							hc->r_buf[len] = 0;
							hc->r_len = len;
							break;
						}
					} else if (len > 0) {
						/* corner case, the rx data ends with \r */
						memmove(hc->r_buf, p1, len);
						hc->r_buf[len] = 0;
						hc->r_len = len;
						break;
					}
				} else {
					/* chunked data is not complete, copy the data for response .. */
					if (hc->store != NULL)
						hc->store(p1, len, hc->recv_len, hc->length, hc->store_param);
					hc->recv_len += len;

					hc->length -= len;
					hc->r_len = 0;

					if (hc->response.chunked == false)
						if (hc->length == 0) {
							printf_ut(LOG_INFO, "http body data receive end");
							return HTTP_RX_DONE;	/* all data received */
						} else if (hc->length < 0) {
							printf_ut(LOG_ERROR, "http resp length error: length=%d, len=%d, r_len=%d", hc->length, len, hc->r_len);
							return ERR_CODE(ERR_MODULE_HTTP, E_ERR_CODE_HTTP_RESP_LEN);
						}
					
					break;
				}
			} else {
				/*  chunked size check .. */
				if (hc->response.chunked == false)
					return ERR_CODE(ERR_MODULE_HTTP, E_ERR_CODE_HTTP_RESP_LEN);

				if ((hc->r_len > 2) && (memcmp(p1, "\r\n", 2) == 0)) {
					p1 += 2;
					hc->length = -1;
				} else {
					hc->length = -1;
					hc->r_len = 0;
				}
			}
		}
	}

	printf_ut(LOG_INFO, "http body parser: length=%d, r_len=%d::%s", hc->length, hc->r_len, hc->r_buf);
	printf_ut(LOG_INFO, "http body parser: recv_len=%d", hc->recv_len);

	return ERR_OK;
}


/**
 * @brief brief
 *
 * @param 1 comments
 * @param 2 comments
 * @return comment
 *   @retval value
 * @see link
 * @attention attention
 * @warning warning
 */
error_t http_close(http_client_t *hc)
{
	if (hc->socket_desc != 0) {
		pl_close_socket(hc->socket_desc);
		hc->socket_desc = 0;
	}
    	return ERR_OK;
}

/**
 * @brief brief
 *
 * @param 1 comments
 * @param 2 comments
 * @return comment
 *   @retval value
 * @see link
 * @attention attention
 * @warning warning
 */
static error_t http_connect(http_client_t *hc)
{
	int32_t portno;
	int32_t sock_fd = hc->socket_desc;
	uint32_t addr;
	struct pl_sockaddr laddr;

	if (pl_inet_aton(hc->url.host, &addr) != 0) {
		laddr.ip.ip = addr;
	} else {
#if 0
		server = gethostbyname(hc->url.host);
		if (server == NULL) {
			printf_dbg(LOG_ERROR, "can not get host address");
			return ERR_CODE(ERR_MODULE_HTTP, E_ERR_CODE_HOST_ADDR);
		}

		server_addr.sin_addr = *((struct in_addr *)server->h_addr);
#endif
	}

	laddr.type = PL_SOCKADDR_IP;
	laddr.ip.port = atol(hc->url.port);

	if (pl_connect(sock_fd, &laddr) < 0) {
		printf_dbg(LOG_ERROR, "can not connect to host");
		return ERR_CODE(ERR_MODULE_HTTP, E_ERR_CODE_CONNECT);
	}

	return ERR_OK;
}

/**
 * @brief brief
 *
 * @param 1 comments
 * @param 2 comments
 * @return comment
 *   @retval value
 * @see link
 * @attention attention
 * @warning warning
 */
error_t http_open(http_client_t *hc, const int8_t *url)
{
	int8_t host[256], port[10], dir[1024];
	int32_t sock_fd, https, verify;
	int32_t ret, opt;

	if (NULL == hc)
		return ERR_CODE(ERR_MODULE_HTTP, E_ERR_CODE_NULL_PTR);

	/* close the opened http client */
	if (hc->socket_desc != 0)
		http_close(hc);

	parse_url((int8_t *)url, &https, host, port, dir);

	if ((hc->url.https != https) ||
	    	(strcmp(hc->url.host, host) != 0) ||
	    	(strcmp(hc->url.port, port) != 0)) {
		http_close(hc);
		http_init(hc, hc->store, hc->store_param);
	} else {
		sock_fd = hc->socket_desc;

		if ((pl_getsockopt_err(sock_fd, (void *)&opt) < 0) || (opt > 0)) {
			http_close(hc);
			http_init(hc, hc->store, hc->store_param);
		}
		else {
			return ERR_OK;
		}
	}

	sock_fd = pl_socket(PL_INET, PL_SOCK_STREAM, 0);
	if (sock_fd < 0) {
		printf_dbg(LOG_ERROR, "cocket cerate fail");
		return ERR_CODE(ERR_MODULE_HTTP, E_ERR_CODE_SOCK_NULL);
	}

	hc->socket_desc = sock_fd;
	hc->url.https = https;

	strncpy(hc->url.host, host, strlen(host));
	strncpy(hc->url.port, port, strlen(port));
	strncpy(hc->url.path, dir, strlen(dir));

	printf_ut(LOG_INFO, "host=%s, port=%s, path=%s", hc->url.host, hc->url.port, hc->url.path);

	if ((ret = http_connect(hc)) < 0) {
		http_close(hc);
		printf_dbg(LOG_ERROR, "connect failure");
		return ret;
	}

	return ERR_OK;
}

/**
 * @brief brief
 *
 * @param 1 comments
 * @param 2 comments
 * @return comment
 *   @retval value
 * @see link
 * @attention attention
 * @warning warning
 */
static int32_t http_read(http_client_t *hc, int8_t *buf, int32_t size)
{
	int32_t ret;
	struct timeval tv;
    tv.tv_sec = 2; //wait time is 2 secs
    tv.tv_usec = 0;
	setsockopt(hc->socket_desc, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
	ret = pl_recv(hc->socket_desc, buf, size, 0);
	if(ret<0)
	{
		if(errno == EAGAIN)
		{
			printf_dbg(LOG_ERROR, "http read time out");
			http_read_time_out_flag = true;
			return ret;
		}
		printf_dbg(LOG_ERROR, "http read error");
	}
	http_read_time_out_flag = false;
	return ret;
}


/**
 * @brief brief
 *
 * @param 1 comments
 * @param 2 comments
 * @return comment
 *   @retval value
 * @see link
 * @attention attention
 * @warning warning
 */
int32_t http_recv(http_client_t *hc)
{
	int32_t ret;
	error_t err;

	if (NULL == hc)
		return ERR_NOK;

	hc->response.status = 0;
	hc->response.content_length = 0;
	hc->response.close = 0;

	hc->r_len = 0;
	hc->header_end = 0;
	hc->recv_len = 0;

	while (1) {
		ret = http_read(hc, &hc->r_buf[hc->r_len], (int32_t)HTTP_READ_SIZE);
		if(http_read_time_out_flag == true) 
		{
			return ERR_NOK;
		}
		if (ret < 0 && http_read_time_out_flag == false) {
			http_close(hc);
			printf_dbg(LOG_ERROR, "http header read error %d", ret);
			return ERR_NOK;
		} else if (ret == 0) {
			break;
		}
	
		hc->r_len += ret;
		hc->r_buf[hc->r_len] = 0;

		if ((http_header_parse(hc) != ERR_OK) || (hc->header_end == true))
			break;

	}

	printf_ut(LOG_INFO, "HTTP response header parser:\n"
		"method:%s\n"
		"status:%d\n"
		"content_type:%s\n"
		"content_length:%d\n"
		"chunked:%d\n"
		"close:%d\n"
		"location:%s\n"
		"referrer:%s\n"
		"cookie:%s\n"
		"boundary:%s\n"
		, hc->response.method, hc->response.status, 
		hc->response.content_type, hc->response.content_length,
		hc->response.chunked, hc->response.close,
		hc->response.location, hc->response.referrer,
		hc->response.cookie, hc->response.boundary);

	/* check content-type, only accept binary or text */
	if ( (hc->response.content_type[0] != 0) && 
		(strstr(hc->response.content_type, "application/octet-stream") == NULL) && 
		(strstr(hc->response.content_type, "application/json") == NULL) &&
		(strstr(hc->response.content_type, "text/xml") == NULL) && 
		(strstr(hc->response.content_type, "text/plain") == NULL)) {
		printf_dbg(LOG_ERROR, "http content type not supported: %s", 
				hc->response.content_type);
		http_close(hc);
		return hc->response.status;
	}

	/* HTTP response body rx and parsing */
	while (hc->response.status == HTTP_STATUS_OK) {
		/* response OK */
		ret = http_read(hc, &hc->r_buf[hc->r_len], (int32_t)(HTTP_SESSION_R_BUFF_SIZE - hc->r_len));
		if (ret < 0) {
			http_close(hc);
			printf_dbg(LOG_ERROR, "http body read error %d", ret);
			return ret;
		} else if (ret == 0) {
			break;
		}

		hc->r_len += ret;
		hc->r_buf[hc->r_len] = 0;

		if ((err = http_body_parse(hc)) != ERR_OK) {
			printf_ut(LOG_ERROR, "http_body_parse() returns %d", err);
			break;
		}
	}

	if (hc->response.close == 1) {
		printf_dbg(LOG_INFO, "http closed");
		http_close(hc);
	}

	return hc->response.status;
}

/**
 * @brief brief
 *
 * @param 1 comments
 * @param 2 comments
 * @return comment
 *   @retval value
 * @see link
 * @attention attention
 * @warning warning
 */
error_t http_get(http_client_t *hc, const int8_t *url)
{
	int8_t request[1024];
	int32_t sock_fd, verify;
	int32_t ret, len;

	if (NULL == hc)
		return ERR_CODE(ERR_MODULE_HTTP, E_ERR_CODE_NULL_PTR);

	if (hc->store == NULL)
		return ERR_NOK;

	if (http_open(hc, url) != ERR_OK)
		return ERR_NOK;

	/* Send HTTP request. */
	len = snprintf(request, 1024,
		       "GET %s HTTP/1.1\r\n"
		       "User-Agent: FotaV0\r\n"
		       "Host: %s:%s\r\n"
		       "Content-Type: application/octet-stream; charset=utf-8\r\n"
		       "Accept: application/octet-stream,text/xml,text/plain\r\n"
		       "Connection: close\r\n\r\n",
		       hc->url.path, hc->url.host, hc->url.port);

	printf_ut(LOG_INFO, "http get: %s", request);

	if ((ret = http_write(hc, request, len)) != len) {
		http_close(hc);
		printf_dbg(LOG_ERROR, "http req write error");
		return ERR_NOK;
	}

	return ERR_OK;
}

/**
 * @brief brief
 *
 * @param 1 comments
 * @param 2 comments
 * @return comment
 *   @retval value
 * @see link
 * @attention attention
 * @warning warning
 */
error_t http_post(http_client_t *hc, const int8_t *url, int8_t *data)
{
	int8_t request[1024];
	int32_t ret, len;

	if (NULL == hc)
		return ERR_NOK;

	if (hc->store == NULL)
		return ERR_NOK;

	if (http_open(hc, url) != ERR_OK)
		return ERR_NOK;

	/* Send HTTP request. */
	len = snprintf(request, 1024,
		       "POST %s HTTP/1.1\r\n"
		       "User-Agent: FotaV0\r\n"
		       "Host: %s:%s\r\n"
		       "Connection: Keep-Alive\r\n"
		       "Accept: text/plain,text/xml\r\n"
		       "Content-Type: text/xml; charset=utf-8\r\n"
		       "Content-Length: %d\r\n"
		       "Cookie: %s\r\n"
		       "%s",
		       hc->url.path, hc->url.host, hc->url.port, (int32_t)strlen(data),
		       hc->request.cookie, data);

	if ((ret = http_write(hc, request, len)) != len) {
		http_close(hc);
		return ERR_NOK;
	}

	return ERR_OK;
}


/**
 * @brief brief
 *
 * @param 1 comments
 * @param 2 comments
 * @return comment
 *   @retval value
 * @see link
 * @attention attention
 * @warning warning
 */
error_t http_upload(const int8_t *url, const int8_t *filename, int8_t *data, int32_t size)
{
	int8_t *request, *header, *body;
	int32_t ret, len_body, len_request, len_write;
	int8_t boundary[32];
	int32_t boundary_len;
	static int32_t boundary_cnt = 0;

	http_client_t hc;

	http_init(&hc, NULL, 0);

	if (http_open(&hc, url) != ERR_OK)
		return ERR_NOK;

	body = (uint8_t *)malloc(512);
	if (body == NULL) {
		printf_dbg(LOG_ERROR, "malloc error");
		return ERR_NOK;
	}

	request = (uint8_t *)malloc(1024 + size);
	if (request == NULL) {
		printf_dbg(LOG_ERROR, "malloc error");
		return ERR_NOK;
	}

	boundary_len = snprintf(boundary, 32, "--boundary%d", boundary_cnt);
	boundary_cnt++;

	/* body */
	len_body = snprintf(body, 512,
		       "--%s\r\n"
		       "Content-Disposition: form-data; name=\"key1\"; filename=\"%s\"\r\n"
			"Content-Type:application/octet-stream\r\n\r\n"
			"\r\n"
			"--%s--\r\n",
		       boundary, filename, boundary);

	len_body += size; /* total body length */



	/* request */
	len_request = snprintf(request, 1024 + size,
		       "POST %s HTTP/1.1\r\n"
		       "User-Agent: FotaV0\r\n"
		       "Host: %s:%s\r\n"
		       "Connection: close\r\n"
		       "Accept: text/plain,text/xml\r\n"
		       "Content-Type: multipart/form-data;boundary=%s\r\n"
		       "Content-Length: %d\r\n\r\n"
		       "%s",
		       hc.url.path, hc.url.host, hc.url.port, boundary,
		       len_body, body);

	/* fill data */
	memcpy(&request[len_request - boundary_len - 8], data, size);

	/* re-fill tail */
	snprintf(&request[len_request - boundary_len- 8 + size], 1024 + size,
		       "\r\n"
		       "--%s--\r\n",
		       boundary);

	printf_ut(LOG_INFO, "upload request: %s\n", request);

	len_write = 0;
	do {
		if ((ret = http_write(&hc, request + len_write, len_request + size - len_write)) < 0) {
			http_close(&hc);
			free(body);
			free(request);

			printf_dbg(LOG_ERROR, "write error len_write=%d, ret=%d", len_write, ret);
			return ERR_NOK;
		}
		len_write += ret;
	} while(len_write < (len_request + size));

	free(body);
	free(request);

	/* get response */
	if (HTTP_STATUS_OK != http_recv(&hc))
		return ERR_NOK;

	http_close(&hc);

	return ERR_OK;
}

/**
 * @brief brief
 *
 * @param 1 http_client_t
 * @param 2 URL
 * @param 3 method
 * @param 4 argp
 * @return comment
 *   @retval value
 * @see link
 * @attention attention
 * @warning warning
 */
error_t http_rpc(http_client_t *hc, const int8_t *url, const int8_t *method, va_list argp)
{
	int8_t request[2048];
	int8_t xml[2048];
	int32_t xml_len;
	int32_t ret, len;
	const int8_t *type;
	const int8_t *value;
	const int8_t *value2base64;

	if (NULL == hc)
		return ERR_NOK;

	if (http_open(hc, url) != ERR_OK)
		return ERR_NOK;

	/* method */
	xml_len = snprintf(xml, sizeof(xml),
		"<?xml version='1.0'?>\r\n"
		"<methodCall>\r\n"
		"<methodName>%s</methodName>\r\n"
		"<params>\r\n"
		"<param>\r\n",
		method);

	/* parameters table */
	do {
		type = va_arg(argp, const int8_t*);
		if (type != NULL) {
			value = va_arg(argp, const int8_t*);	
			
			if(type == "base64")
			{
				FILE *fp;
				size_t size; 
				unsigned int file_len = 0 ,i = 0, len = 0;
				char file_conent[4096],value2base64[4096];

				if ((fp = fopen(value,"rb")) == NULL) {
					printf(LOG_ERROR, "[http_rpc] can not open file %s", value);
				}

				  /* Read up to the buffer size */
				size = fread(file_conent, 1, 4096, fp);  

				if(!size) {
					printf_dbg(LOG_ERROR, " Metadata_t:%s: Empty or broken\n", fp);
				}

				fclose(fp);
				b64_encode(file_conent, size, value2base64);
				xml_len += snprintf(&xml[xml_len], sizeof(xml) - xml_len, 
				"<value><%s>%s</%s></value>\r\n",
				type, value2base64, type);		  
				}
				else
				{
					xml_len += snprintf(&xml[xml_len], sizeof(xml) - xml_len, 
					"<value><%s>%s</%s></value>\r\n",
					type, value, type);
				}
			
			
		}
	} while (type);

	/* tail */
	xml_len += snprintf(&xml[xml_len], sizeof(xml) - xml_len, 
			"</param>\r\n"
			"</params>\r\n"
			"</methodCall>\r\n");

	/* Send HTTP request. */
	len = snprintf(request, 2048,
		       "POST /RPC2 HTTP/1.1\r\n"
		       "User-Agent: FotaV0\r\n"
		       "Host: %s:%s\r\n"
		       "Connection: Keep-Alive\r\n"
		       "Accept: text/plain,text/xml\r\n"
		       "Content-Type: text/xml\r\n"
		       "Content-Length: %d\r\n\r\n"
		       "%s",
		       hc->url.host, hc->url.port, xml_len, xml);
	printf_ut(LOG_INFO, "XML-RPC Request\n%s\n", request);

	if ((ret = http_write(hc, request, len)) != len) {
		http_close(hc);
		return ERR_NOK;
	}

	return ERR_OK;
}

/**
 * @brief brief
 *
 * @param 1 comments
 * @param 2 comments
 * @return comment
 *   @retval value
 * @see link
 * @attention attention
 * @warning warning
 */
error_t http_rpc_method(const int8_t *server_url, store_cb cust_store, int8_t *cust_param, const int8_t *rpc_method, ...)
{
	http_client_t client;
	va_list valist;

	http_init(&client, cust_store, cust_param);

	va_start(valist, rpc_method);
	

	if (http_rpc(&client, server_url, rpc_method, valist) < 0) {
		
		va_end(valist);
		return ERR_NOK;
	}

	va_end(valist);

	if (HTTP_STATUS_OK != http_recv(&client))
		return ERR_NOK;

	http_close(&client);

	return ERR_OK;
}

/**
 * @brief brief
 *
 * @param 1 comments
 * @param 2 comments
 * @return comment
 *   @retval value
 * @see link
 * @attention attention
 * @warning warning
 */
error_t http_download(const int8_t *server_url, store_cb cust_store, int8_t *param)
{
	http_client_t client;
	if(http_init(&client, cust_store, param) != ERR_OK)
	return ERR_NOK;
	while(1)
	{
		if (http_get(&client, server_url) != ERR_OK)
		return ERR_NOK;  
	    if( HTTP_STATUS_OK != http_recv(&client))
		{
			if( http_read_time_out_flag == true )
			{
				printf_dbg(LOG_INFO,"Recv timeout, try to recv again\n");
				http_close(&client);
				continue;
			}
			else
			{
				return ERR_NOK;
			}
		}
		else
		{
			break;
		}

	}
	http_close(&client);

	return ERR_OK;
}

/**
 * @brief brief
 *
 * @param 1 comments
 * @param 2 comments
 * @return comment
 *   @retval value
 * @see link
 * @attention attention
 * @warning warning
 */
error_t http_write_header(http_client_t *hc)
{
	int8_t request[4096];
	int32_t ret, len, l;

	if (NULL == hc)
		return ERR_NOK;

	/* Send HTTP request. */
	len = snprintf(request, 1024,
		       "%s %s HTTP/1.1\r\n"
		       "User-Agent: FotaV0\r\n"
		       "Host: %s:%s\r\n"
		       "Content-Type: %s\r\n",
		       hc->request.method, hc->url.path, hc->url.host,
		       hc->url.port, hc->request.content_type);

	if (hc->request.referrer[0] != 0) {
		len += snprintf(&request[len], HTTP_FIELD_SIZE, "Referer: %s\r\n",
				hc->request.referrer);
	}

	if (hc->request.chunked == true) {
		len += snprintf(&request[len], HTTP_FIELD_SIZE,
				"Transfer-Encoding: chunked\r\n");
	} else {
		len += snprintf(&request[len], HTTP_FIELD_SIZE,
				"Content-Length: %ld\r\n",
				hc->request.content_length);
	}

	if (hc->request.close == true) {
		len += snprintf(&request[len], HTTP_FIELD_SIZE,
				"Connection: close\r\n");
	} else {
		len += snprintf(&request[len], HTTP_FIELD_SIZE,
				"Connection: Keep-Alive\r\n");
	}

	if (hc->request.cookie[0] != 0) {
		len += snprintf(&request[len], HTTP_FIELD_SIZE, "Cookie: %s\r\n",
				hc->request.cookie);
	}

	len += snprintf(&request[len], HTTP_FIELD_SIZE, "\r\n");

	printf_ut(LOG_INFO, "INFO: request\n %s", request);

	if ((ret = http_write(hc, request, len)) != len) {
		http_close(hc);
		printf_dbg(LOG_ERROR, "write error");
		return ERR_NOK;
	}

	return ERR_OK;
}

/**
 * @brief brief
 *
 * @param 1 comments
 * @param 2 comments
 * @return comment
 *   @retval value
 * @see link
 * @attention attention
 * @warning warning
 */
int32_t http_write(http_client_t *hc, const int8_t *data, int32_t len)
{
	int8_t str[10];
	int32_t ret, l;

	if (NULL == hc || len <= 0)
		return ERR_CODE(ERR_MODULE_HTTP, E_ERR_CODE_NULL_PTR);

	if (hc->request.chunked == true) {
		l = snprintf(str, 10, "%x\r\n", len);
		if (pl_send(hc->socket_desc, str, l, 0) != l) {
			printf_dbg(LOG_ERROR, "http write error 1");
		}
	}

	ret = pl_send(hc->socket_desc, data, len, 0);
	if (ret != len) {
		printf_dbg(LOG_ERROR, "write error 2");
	}

	if (hc->request.chunked == true) {
		if (pl_send(hc->socket_desc, "\n\r", 2, 0) != 2) {
			printf_dbg(LOG_ERROR, "http write error 3");
		}
	}

	return ret;
}

/**
 * @brief brief
 *
 * @param 1 comments
 * @param 2 comments
 * @return comment
 *   @retval value
 * @see link
 * @attention attention
 * @warning warning
 */
int32_t http_write_end(http_client_t *hc)
{
	int8_t str[10];
	int32_t ret, len;

	if (NULL == hc)
		return -1;

	if (hc->request.chunked == true) {
		len = snprintf(str, 10, "0\r\n\r\n");
	} else {
		len = snprintf(str, 10, "\r\n\r\n");
	}

	if ((ret = http_write(hc, str, len)) != len) {
		http_close(hc);
		printf_dbg(LOG_ERROR, "write error");
		return ERR_NOK;
	}

	return len;
}

