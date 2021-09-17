/*
 * Copyright 2018-2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause

 */
#pragma once
#define FILE__PL_TCPIP_H
#include "pl_types.h"

#ifdef __cplusplus
extern "C" {
#endif

enum tcpip_domain {
	PL_INET,
	PL_INET6,
	PL_PACKET
};

enum tcpip_sock_type {
	PL_SOCK_STREAM,
	PL_SOCK_DGRAM,
	PL_SOCK_RAW
};

struct pl_sockaddr_ip {
	uint32_t ip;
	uint16_t port;
};

struct pl_sockaddr_ip6 {
	uint32_t ip[4];
	uint16_t port;
};

struct pl_sockaddr_raw {
	uint32_t if_index;
	uint32_t packet_type;
};

struct pl_sockaddr {
	enum {
		PL_SOCKADDR_IP,
		PL_SOCKADDR_IP6,
		PL_SOCKADDR_RAW
	} type;
	
	union {
		struct pl_sockaddr_ip ip;
		struct pl_sockaddr_ip6 ip6;
		struct pl_sockaddr_raw raw;
	};
};

#define PL_FLAG_DONTWAIT 0x1

#if (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define pl_htons(x)
#define pl_ntohs(x)
#define pl_htonl(x)
#define pl_ntohl(x)
#else
#define pl_htons(x) ((((x) & 0x00ffUL) << 8) | (((x) & 0xff00UL) >> 8))
#define pl_ntohs(x) pl_htons(x)
#define pl_htonl(x) ((((x) & 0x000000ffUL) << 24) | \
                     (((x) & 0x0000ff00UL) <<  8) | \
                     (((x) & 0x00ff0000UL) >>  8) | \
                     (((x) & 0xff000000UL) >> 24))
#define pl_ntohl(x) pl_htonl(x)
#endif

int pl_inet_aton(const char *cp, uint32_t *addr);
char* pl_inet_ntoa(uint32_t addr_in);

int pl_socket(
	enum tcpip_domain domain,
	enum tcpip_sock_type type,
	int flag
	);

int pl_getsockopt_err(int socket, int32_t *err);

ret_t pl_connect(int socket, struct pl_sockaddr *addr);

ret_t pl_bind_ip(int sockfd, uint32_t ip, uint16_t port);

ret_t pl_listen(int sockfd, uint32_t depth);

int pl_accept(int sockfd, struct pl_sockaddr *addr);

int pl_recvfrom(int sockfd,
		void *buf, uint32_t length, 
		int flag,
		struct pl_sockaddr *addr);

int pl_recv(int sockfd,
		void *buf, uint32_t length, 
		int flag);

int pl_send(int sockfd, const void *buf, uint32_t len, int flags);

int pl_sendto(int sockfd, const void *buf, uint32_t len, int flags,
                      const struct pl_sockaddr *addr);

int pl_close_socket(int sockfd);

int pl_get_mac_address(int sockfd, const char *ifname, uint8_t mac[6]);

int pl_get_if_index(int sockfd, const char *ifname, int *p_index);

int pl_bind_device(int sockfd, const char *ifname);

#ifdef __cplusplus
}
#endif
