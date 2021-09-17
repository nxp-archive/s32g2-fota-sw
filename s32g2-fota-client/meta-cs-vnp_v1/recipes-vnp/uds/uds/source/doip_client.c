/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "pl_log.h"
#include "pl_errno.h"
#include "pl_stdlib.h"
#include "doip_feature.h"
#include "doip_client.h"

struct doipc {
	struct uds_tp tp;
	uint32_t ip;
	uint16_t sa;
	uint16_t ta;
	int fd;
	bool ativated;
	struct tp_buff tx_tpb;
	struct tp_buff rx_tpb;
};

int doipc_init(void)
{
	return 0;
}

void doipc_uninit(void)
{

}

static void fill_hdr(struct doip_hdr *hdr, uint16_t payload_type, uint32_t payload_length)
{
	hdr->ver = DOIP_VERSION;
	hdr->inv_ver = DOIP_VERSION ^ 0xff;
	hdr->payload_type = htons(payload_type);
	hdr->payload_length = htonl(payload_length);
}

static int check_hdr(struct doip_hdr *hdr, uint16_t payload_type, uint32_t payload_length)
{
	if (hdr->ver != (hdr->inv_ver ^ 0xFF))
		return -11;

	if (ntohs(hdr->payload_type) != payload_type)
		return -12;
	
	if (ntohl(hdr->payload_length) != payload_length)
		return -13;

	return 0;
}

int block_send(int fd, void *buff, int length)
{
	int ret;
	int remain = length;
	
	while (remain) {
		ret = send(fd, buff, remain, 0);
		if (ret == -1) {
			if (errno != EAGAIN)
				return -errno;
			continue;
		}
		remain -= ret;
		buff += ret;
	}
	
	return length;
}

static int do_vehicle_id_req(int fd, uint8_t *rx_buf, uint16_t *rx_len)
{
	struct timeval time = {
		.tv_sec = 3,
	};
	struct doip_hdr hdr;
	int ret;
	
	fill_hdr(&hdr, DOIP_PT_vehicle_id_req,0);
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&time, sizeof(time));
	
	ret = send(fd, &hdr, sizeof(hdr), 0);
	if (ret <= 0) {
		perror("do_vehicle_id_req: send()");
		return -errno;
	}

	while (1) {
		ret = recv(fd, rx_buf, *rx_len, 0);
		if (ret <= 0) {
			perror("do_vehicle_id_req: recv()");
			return -errno;
		}
		if (ret < (DOIP_HDR_LEN + sizeof(struct doip_pt_vehicle_id_rsp)))
			continue;

		if (check_hdr((struct doip_hdr *)rx_buf, DOIP_PT_vehicle_id_rsp, ret - DOIP_HDR_LEN) != 0)
			continue;
		*rx_len = ret;
		break;
	}
	return 0;
}

int doipc_get_ta_by_ip(uint32_t ip, uint16_t *ta)
{
	struct sockaddr_in serv_addr;
	uint8_t rx_buf[200];
	uint16_t rx_len = (uint16_t)sizeof(rx_buf);
	struct doip_pt_vehicle_id_rsp *vid_rsp = (void*)&rx_buf[DOIP_HDR_LEN];	
	int fd;
	int ret;

	if (ip == 0 || ip == 0xffffffff || !ta)
		return -1;
	
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket()");
		return -1;
	}
	
	memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(DOIP_UDP_DISCOVERY_PORT);
    serv_addr.sin_addr.s_addr = htonl(ip);

	ret = connect(fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	if (ret == -1) {
		perror("doipc_get_ta_by_ip: connect()");
		close(fd);
		return ret;
	}

	ret = do_vehicle_id_req(fd, rx_buf, &rx_len);
	if (ret == 0) {
		*ta = (uint16_t)vid_rsp->la[0] << 8 | vid_rsp->la[1];
	}
	close(fd);
	return ret;
}

int doipc_for_each_entity(uint32_t bc_ip, int (*callback)(uint32_t ip, struct doip_pt_vehicle_id_rsp *))
{
	struct sockaddr_in serv_addr;
	socklen_t addr_len;
	union {
		uint8_t buf[200];
		struct doip_hdr hdr;
	} xdata;
	struct doip_pt_vehicle_id_rsp *rsp = (void*)&xdata.buf[DOIP_HDR_LEN];
	int fd;
	int ret;
	const int so_broadcast = 1;
	const struct timeval time = {
		.tv_sec = 2,
	};

	if (bc_ip == 0 || !callback)
		return -1;
	
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket()");
		return -1;
	}

	ret = setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &so_broadcast,sizeof(so_broadcast));
    if(ret ==-1)
    {
    	close(fd);
        perror("setsockopt(SO_BROADCAST)\n");
        return -1;
    }

	ret = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&time, sizeof(time));
	if(ret ==-1)
	{
		close(fd);
		perror("setsockopt(SO_RCVTIMEO)\n");
		return -1;
	}

	fill_hdr(&xdata.hdr, DOIP_PT_vehicle_id_req,0);
	
	memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(DOIP_UDP_DISCOVERY_PORT);
    serv_addr.sin_addr.s_addr = htonl(bc_ip);
	ret = sendto(fd, &xdata.hdr, sizeof(xdata.hdr), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	if (ret <= 0) {
		close(fd);
		perror("do_vehicle_id_req: send()");
		return -errno;
	}

	while (1) {
		addr_len = sizeof(serv_addr);
		ret = recvfrom(fd, xdata.buf, sizeof(xdata.buf), 0, (struct sockaddr *)&serv_addr, &addr_len);
		if (ret <= 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			
			perror("do_vehicle_id_req: recv()");
			return -errno;
		}
		if (ret < (DOIP_HDR_LEN + sizeof(struct doip_pt_vehicle_id_rsp)))
			continue;

		if (check_hdr(&xdata.hdr, DOIP_PT_vehicle_id_rsp, ret - DOIP_HDR_LEN) != 0)
			continue;
		
		if (callback(ntohl(serv_addr.sin_addr.s_addr), rsp) != 0)
			break;
	}
	
	close(fd);
	return 0;
}

int doipc_get_ip_by_ta(uint16_t ta, uint32_t *ip)
{
	return -1;
}

static int doipc_send_raw(struct doipc *doip, void *buf, uint16_t len)
{
	return send(doip->fd, buf, len, 0);
}

static int doipc_recv_raw(struct doipc *doip, void *buf, uint16_t len)
{
	int recv_len = 0;
	int ret;
	struct doip_hdr *hdr = buf;
	uint32_t payload_length = 0;
	
	while (1) {
		ret = recv(doip->fd, buf + recv_len, len - recv_len, 0);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			perror("doipc_recv_raw: recv()");
			return -errno;
		}

		recv_len += ret;
		
		if (!payload_length && (recv_len >= DOIP_HDR_LEN)) {
			if (hdr->ver != (hdr->inv_ver ^ 0xff))
				return -2;
			payload_length = ntohl(hdr->payload_length);
		}
		
		if (payload_length
			&& (recv_len >= (DOIP_HDR_LEN + payload_length)
			 || recv_len >= len))
			break;
	}
	
	return recv_len;
}

static int do_routing_act_req(struct doipc *doip)
{
	union {
		uint8_t tx_buf[DOIP_HDR_LEN + sizeof(struct doip_pt_routing_act_req)];
		uint8_t rx_buf[200];
		struct doip_hdr hdr;
	} xdata;
	struct doip_pt_routing_act_req *req = (void*)&xdata.tx_buf[DOIP_HDR_LEN];
	//struct doip_pt_routing_act_rsp *rsp = (void*)&xdata.rx_buf[DOIP_HDR_LEN];
	int ret;

	req->sa[0] = doip->sa >> 8;
	req->sa[1] = (uint8_t)doip->sa;
	req->act_type[0] = DOIP_ROUTINE_ACT_DEFAULT >> 8;
	req->act_type[1] = (uint8_t)DOIP_ROUTINE_ACT_DEFAULT;
	bzero(req->reserved, sizeof(req->reserved));
	
	fill_hdr(&xdata.hdr, DOIP_PT_routine_act_req, sizeof(struct doip_pt_routing_act_req));
	
	ret = doipc_send_raw(doip, xdata.tx_buf, sizeof(xdata.tx_buf));
	if (ret != sizeof(xdata.tx_buf))
		return -2;

	while (1) {
		ret = doipc_recv_raw(doip, xdata.rx_buf, sizeof(xdata.rx_buf));
		if (ret < 0)
			return -3;
		
		if (check_hdr(&xdata.hdr, DOIP_PT_routine_act_rsp, ret - DOIP_HDR_LEN) != 0)
			return -4;
		break;
	}
	return 0;
}

static struct tp_buff *doipc_alloc(struct uds_tp *tp, TP_DIR dir)
{
	struct doipc *doip = (struct doipc *)tp;
	struct tp_buff *tpb = (dir == TP_TX ? &doip->tx_tpb : &doip->rx_tpb);

	tpb->data = tpb->head + DOIP_HDR_LEN + DOIP_SA_TA_LEN;
	tpb->len = 0;
	return tpb;
}

static void doipc_free(struct uds_tp *tp, struct tp_buff *tpb)
{

}

static int init_tpb(struct doipc *doip)
{
	if (!doip->tx_tpb.head) {
		doip->tx_tpb.head = pl_malloc(DOIP_HDR_LEN + DOIP_SA_TA_LEN + doip->tp.mtu);
		if (!doip->tx_tpb.head)
			return -ENOMEM;
		doip->tx_tpb.end = doip->tx_tpb.head + (DOIP_HDR_LEN + DOIP_SA_TA_LEN + doip->tp.mtu);
	}
	
	if (!doip->rx_tpb.head) {
		doip->rx_tpb.head = pl_malloc(DOIP_HDR_LEN + DOIP_SA_TA_LEN + doip->tp.mtu);
		if (!doip->rx_tpb.head)
			return -ENOMEM;
		doip->rx_tpb.end = doip->rx_tpb.head + (DOIP_HDR_LEN + DOIP_SA_TA_LEN + doip->tp.mtu);
	}
	
	return 0;
}

static int init_tcp_socket(struct doipc *doip)
{
	struct sockaddr_in serv_addr;
	socklen_t addr_len = sizeof(serv_addr);
	int ret;
	
	if (doip->fd != -1)
		close(doip->fd);

	if ((doip->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("init_tcp_socket: socket()");
		return -2;
	}
	
	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(DOIP_TCP_DATA_PORT);
	serv_addr.sin_addr.s_addr = htonl(doip->ip);

	ret = connect(doip->fd, (struct sockaddr*)&serv_addr, addr_len);
	if (ret == -1) {
		perror("init_tcp_socket: connect()");
		close(doip->fd);
		doip->fd = -1;
		return ret;
	}

	struct timeval timeout = {
		.tv_sec = 10,
	};

	ret = setsockopt(doip->fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
	if (ret == -1)
		perror("setsockopt");
	return 0;
}

static int doipc_activate(struct uds_tp *tp)
{
	struct doipc *doip = (struct doipc *)tp;
	int ret;
	
	ret = init_tpb(doip);
	if (ret)
		return ret;

	ret = init_tcp_socket(doip);
	if (ret)
		return ret;

	return do_routing_act_req(doip);
}

static int doipc_send(struct uds_tp *tp, struct tp_buff *tpb)
{
	struct doipc *doip = (struct doipc *)tp;
	int ret;
	struct doip_hdr *hdr;

	if (!doip->ativated) {
		ret = doipc_activate(tp);
		if (ret)
			return ret;
		doip->ativated = true;
	}
	
	if (tpb->sa == 0)
		tpb->sa = doip->sa;
	if (tpb->ta == 0)
		tpb->ta = doip->ta;
	
	tpb_push(tpb, DOIP_HDR_LEN + DOIP_SA_TA_LEN);
	tpb->data[DOIP_HDR_LEN + 0] = tpb->sa >> 8;
	tpb->data[DOIP_HDR_LEN + 1] = (uint8_t)tpb->sa;
	tpb->data[DOIP_HDR_LEN + 2] = tpb->ta >> 8;
	tpb->data[DOIP_HDR_LEN + 3] = (uint8_t)tpb->ta;
	fill_hdr((struct doip_hdr *)tpb->data, DOIP_PT_diag_msg_req, tpb->len - DOIP_HDR_LEN);
	ret = doipc_send_raw(doip, tpb->data, tpb->len);
	if (ret != tpb->len)
		return -3;
	
	/* Check diagnostic message ACK */
	ret = doipc_recv_raw(doip, tpb->data, tpb->end - tpb->data);
	if (ret < DOIP_HDR_LEN + DOIP_SA_TA_LEN + 1)
		return -4;
	
	hdr = (void*)tpb->data;
	if (ntohs(hdr->payload_type) == DOIP_PT_diag_msg_pos_ack) {
		return 0;
	} else {
	    PL_WARN_LOC("doip header: type: %x\n", ntohs(hdr->payload_type));
	}
	
	/* detail */
	return -5;
}

static int doipc_recv(struct uds_tp *tp, struct tp_buff *tpb)
{
	int ret;
	struct doip_hdr *hdr;
	struct doipc *doip = (struct doipc *)tp;
	
	tpb->data = tpb->head;
	hdr = (void*)tpb->data;
	ret = doipc_recv_raw(doip, tpb->data, tpb->end - tpb->data);
	if (ret < 0)
		return -1;
	tpb->len = ret;
	if (ntohs(hdr->payload_type) != DOIP_PT_diag_msg_req) {
		PL_WARN_LOC("payload_type 0x%04x\n", ntohs(hdr->payload_type));
		return -2;
	}
	tpb->tp = tp;
	tpb->sa = (uint16_t)tpb->data[DOIP_HDR_LEN + 0] << 8 | tpb->data[DOIP_HDR_LEN + 1];
	tpb->ta = (uint16_t)tpb->data[DOIP_HDR_LEN + 2] << 8 | tpb->data[DOIP_HDR_LEN + 3];
	tpb_pull(tpb, DOIP_HDR_LEN + DOIP_SA_TA_LEN);
	return tpb->len;
}

static struct uds_tp_ops g_doip_ops = {
	.alloc = doipc_alloc,
	.free = doipc_free,
	.activate = doipc_activate,
	.send = doipc_send,
	.recv = doipc_recv,
};

struct uds_tp *doipc_open(uint16_t sa, uint32_t ip)
{
	struct doipc *doip;
	uint16_t ta;
	
	if (sa == 0 || ip == 0 || ip == 0xffffffff) {
		errno = -EINVAL;
		return NULL;
	}
	
	if (doipc_get_ta_by_ip(ip, &ta) != 0) {
		errno = -ENOENT;
		return NULL;
	}
	
	doip = pl_malloc_zero(sizeof(struct doipc));
	if (!doip) {
		errno = -ENOMEM;
		return NULL;
	}

	doip->tp.mtu = 1400;
	doip->tp.type = UDS_TP_IP;
	doip->tp.ops = &g_doip_ops;
	doip->tp.rx_callback = NULL;
	doip->ip = ip;
	doip->sa = sa;
	doip->ta = ta;
	doip->fd = -1;
	doip->ativated = false;

	init_tpb(doip);
	
	return &doip->tp;
}

void doipc_close(struct uds_tp *tp)
{
	struct doipc *doip = (struct doipc *)tp;

	if (doip->fd != -1)
		close(doip->fd);
	
	pl_free(doip->tx_tpb.head);
	pl_free(doip->rx_tpb.head);
	pl_free(doip);
}

