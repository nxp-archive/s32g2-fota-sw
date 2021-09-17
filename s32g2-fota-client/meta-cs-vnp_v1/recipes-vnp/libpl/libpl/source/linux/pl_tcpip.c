/*
 * Copyright 2018-2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause

 */
#include "pl_tcpip.h"
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <fcntl.h>
#include <net/if.h>

int pl_inet_aton(const char *cp, uint32_t *addr)
{
	return inet_aton(cp, (struct in_addr *)addr);
}

char* pl_inet_ntoa(uint32_t addr_in)
{
	struct in_addr addr;
	addr.s_addr = addr_in;
	return inet_ntoa(addr);
}
static int pl_to_linux_domain(enum tcpip_domain domain)
{
	switch (domain) {
	case PL_PACKET:
		return AF_PACKET;
	case PL_INET:
		return AF_INET;
	default:
		return AF_INET6;
	}
}

static int pl_to_linux_sock_type(enum tcpip_sock_type type)
{
	switch (type) {
	case PL_SOCK_DGRAM:
		return SOCK_DGRAM;
	case PL_SOCK_STREAM:
		return SOCK_STREAM;
	default:
		return SOCK_RAW;
	}
}

int pl_socket(
	enum tcpip_domain domain,
	enum tcpip_sock_type type,
	int flag
	)
{
	int fd = socket(pl_to_linux_domain(domain),
					pl_to_linux_sock_type(type),
					0);
	if (fd < 0)
		return -errno;
	
	if (flag & PL_FLAG_DONTWAIT) {
		int flags = fcntl(fd, F_GETFL, 0);

		if (flags == -1) {
			close(fd);
			return -errno;
		}
		flags |= O_NONBLOCK;
		fcntl(fd, F_SETFL, flags);
	}
	return fd;
}

int pl_getsockopt_err(int socket, int32_t *err)
{
	socklen_t slen;

	slen = sizeof(int32_t);

	return getsockopt(socket, SOL_SOCKET, SO_ERROR, (void *)err, &slen);
}

ret_t pl_connect(int socket, struct pl_sockaddr *addr)
{
	struct sockaddr_in laddr;
	socklen_t addr_len = sizeof(laddr);

	laddr.sin_addr.s_addr = addr->ip.ip;
	laddr.sin_family = AF_INET;
	laddr.sin_port = htons(addr->ip.port);

	return connect(socket, (struct sockaddr *)(&laddr), addr_len);
}

ret_t pl_bind_ip(int socket, uint32_t ip, uint16_t port)
{
	struct sockaddr_in addr;
	const socklen_t addr_len = sizeof(addr);

	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(ip);
	if (bind(socket, (struct sockaddr*)&addr, addr_len) < 0)
		return -errno;
	return 0;
}

ret_t pl_listen(int socket, uint32_t depth)
{
	return listen(socket, (int)depth);
}

int pl_accept(int socket, struct pl_sockaddr *addr)
{
	struct sockaddr_in laddr;
	socklen_t addr_len = sizeof(laddr);
	int new_fd = accept(socket, (struct sockaddr*)&laddr, &addr_len);
	
	if (new_fd < 0)
		return -errno;

	addr->type = PL_SOCKADDR_IP;
	addr->ip.ip = ntohl(laddr.sin_addr.s_addr);
	addr->ip.port = ntohs(laddr.sin_port);
	return 0;
}

int pl_recvfrom(int socket,
		void *buf, uint32_t length, 
		int flag,
		struct pl_sockaddr *addr)
{
	union {
		uint32_t _u32[16];
		uint16_t _u16[32];
		uint8_t   _u8[64];
	} addr_buf;
	socklen_t addr_len = sizeof(addr_buf);
	int ret;
	int low_flags = flag & PL_FLAG_DONTWAIT ? MSG_DONTWAIT : 0;
	
	ret = recvfrom(socket, buf, length, low_flags, (struct sockaddr*)&addr_buf, &addr_len);
	if (ret < 0)
		return ret;

	if (addr) {
		switch (addr_buf._u16[0]) {
		case AF_INET:
			{
				struct sockaddr_in *paddr_in = (struct sockaddr_in*)addr_buf._u8;

				addr->ip.ip = ntohl(paddr_in->sin_addr.s_addr);
				addr->type = PL_SOCKADDR_IP;
				addr->ip.port = ntohs(paddr_in->sin_port);
			}
			break;
		case AF_INET6:
			{
				struct sockaddr_in6 *paddr_in6 = (struct sockaddr_in6*)addr_buf._u8;

				addr->ip6.ip[0] = ntohl(paddr_in6->sin6_addr.s6_addr32[0]);
				addr->ip6.ip[1] = ntohl(paddr_in6->sin6_addr.s6_addr32[1]);
				addr->ip6.ip[2] = ntohl(paddr_in6->sin6_addr.s6_addr32[2]);
				addr->ip6.ip[3] = ntohl(paddr_in6->sin6_addr.s6_addr32[3]);
				addr->type = PL_SOCKADDR_IP6;
				addr->ip6.port = ntohs(paddr_in6->sin6_port);
			}
			break;
		case AF_PACKET:
			{
				struct sockaddr_ll *paddr_ll = (struct sockaddr_ll*)addr_buf._u8;

				addr->type = PL_SOCKADDR_RAW;
				addr->raw.if_index = paddr_ll->sll_ifindex;
				addr->raw.packet_type = paddr_ll->sll_pkttype;
			}
			break;
		}
	}
	return ret;
}

int pl_recv(int socket,
		void *buf, uint32_t length, 
		int flag)
{
	int low_flags = flag & PL_FLAG_DONTWAIT ? MSG_DONTWAIT : 0;
	
	return recv(socket, buf, length, low_flags);
}

int pl_read(int socket, void *buf, int32_t size)
{
	return read(socket, buf, size);
}

int pl_send(int socket, const void *buf, uint32_t len, int flags)
{
	int low_flags = flags & PL_FLAG_DONTWAIT ? MSG_DONTWAIT : 0;

	return send(socket, buf, len, low_flags);
}

int pl_sendto(int sockfd, const void *buf, uint32_t len, int flags,
                      const struct pl_sockaddr *addr)
{
	int low_flags = flags & PL_FLAG_DONTWAIT ? MSG_DONTWAIT : 0;

	if (!addr)
		return send(sockfd, buf, len, low_flags);

	if (addr->type == PL_SOCKADDR_IP) {
		struct sockaddr_in laddr;

		bzero(&laddr, sizeof(laddr));
		laddr.sin_family = addr->type == PL_SOCKADDR_IP ? AF_INET : AF_INET6;
		laddr.sin_port = htons(addr->ip.port);
		laddr.sin_addr.s_addr = htonl(addr->ip.ip);
		
		return sendto(sockfd, buf, len, low_flags,
			(struct sockaddr*)&laddr, sizeof(laddr));
	}

	if (addr->type == PL_SOCKADDR_RAW) {
		struct sockaddr_ll laddr;

		bzero(&laddr, sizeof(laddr));
		laddr.sll_ifindex = addr->raw.if_index;	
		return sendto(sockfd, buf, len, low_flags,
			(struct sockaddr*)&laddr, sizeof(laddr));
	}
	return -1;
}

int pl_close_socket(int socket)
{
	return close(socket);
}

int pl_get_mac_address(int so, const char *ifname, uint8_t mac[6])
{
	int fd = so;
	struct ifreq ifreq_mac;
	int ret;

	if (!ifname || !mac)
		return -EINVAL;
	
	if (so < 0)
		fd = socket(AF_INET, SOCK_DGRAM, htons(ETH_P_ALL));
	if (fd < 0)
		return -errno;

	memset(&ifreq_mac, 0, sizeof(ifreq_mac));
	strncpy(ifreq_mac.ifr_name, ifname, IFNAMSIZ-1);
	ret = ioctl(fd, SIOCGIFHWADDR, &ifreq_mac);
	if (so < 0)
		close(fd);
	memcpy(mac, ifreq_mac.ifr_addr.sa_data, 6);
	return ret;
}

int pl_get_if_index(int sockfd, const char *ifname, int *p_index)
{
	int fd = sockfd;
	struct ifreq ifreq_idx;
	int ret;

	if (!ifname || !p_index)
		return -EINVAL;
	
	if (sockfd < 0)
		fd = socket(AF_INET, SOCK_DGRAM, htons(ETH_P_ALL));
	if (fd < 0)
		return -errno;

	memset(&ifreq_idx, 0, sizeof(ifreq_idx));
	strncpy(ifreq_idx.ifr_name, ifname, IFNAMSIZ-1);
	ret = ioctl(fd, SIOCGIFINDEX, &ifreq_idx);
	if (sockfd < 0)
		close(fd);
	*p_index = ifreq_idx.ifr_ifindex;
	return ret;
}


int pl_bind_device(int sockfd, const char *ifname)
{
#if 0
	struct sockaddr_ll laddr;
	int index;
	
	pl_get_if_index(sockfd, ifname, &index);
	bzero(&laddr, sizeof(laddr));
	laddr.sll_ifindex = index;	
	return bind(sockfd, (const struct sockaddr *)&laddr, sizeof(laddr));
#else
	return setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, ifname, strlen(ifname));
#endif
}

