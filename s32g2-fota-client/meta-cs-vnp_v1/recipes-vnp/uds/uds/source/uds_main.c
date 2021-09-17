/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <getopt.h>

#include "doip_client.h"
#include "uds_client.h"
#include "uds_file_linux.h"

#define IP_ADDR(a, b, c, d) ((a)<<24 | (b) <<16 | (c) << 8 | (d))

static int on_doip_entity(uint32_t ip, struct doip_pt_vehicle_id_rsp *rsp)
{
	struct in_addr inaddr = {
		.s_addr = htonl(ip),
	};
	
	printf("ip: %s, ta: %04x\n", inet_ntoa(inaddr), (uint16_t)rsp->la[0]<<8 | rsp->la[1]);
	return 0;
}

static void show_doip_entity(uint32_t bc_ip)
{
	if (bc_ip != 0xFFFFFFFF)
		(void)doipc_for_each_entity(bc_ip, on_doip_entity);
	else {
		struct ifaddrs * ifAddrStruct=NULL;
		struct ifaddrs * ifa=NULL;
		struct sockaddr_in *sin_addr;
		
		getifaddrs(&ifAddrStruct);
		for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
			if (!ifa->ifa_addr)
				continue;
			
		    if (ifa ->ifa_addr->sa_family==AF_INET) { // check it is IP4
		        // is a valid IP4 Address
		        sin_addr = (struct sockaddr_in *)ifa->ifa_broadaddr;
				
				printf("Broadcast IP : %s\n", inet_ntoa(sin_addr->sin_addr));
				doipc_for_each_entity(ntohl(sin_addr->sin_addr.s_addr), on_doip_entity);
		    } else if (ifa->ifa_addr->sa_family==AF_INET6) { // check it is IP6
		    }
		}
		if (ifAddrStruct)
			freeifaddrs(ifAddrStruct);
	}
}

static uint8_t g_routine_opts[UDS_TRANSFER_ID_LEN + 1] = {
	UDS_TRANSFER_ID
};

static int set_ota_state(struct udsc *uds, tOTAState state)
{
	g_routine_opts[UDS_TRANSFER_ID_LEN] = (uint8_t)state;
	return udsc_do_start_routine(uds, UDS_RID_set_cur_process, g_routine_opts, sizeof(g_routine_opts), NULL, NULL);
}

int get_ota_state(struct udsc *uds, uint8_t *errcode, tOTAState *state, uint8_t *progress)
{
	int ret;
	uint8_t buf[7];
	uint16_t len = sizeof(buf);
	
	ret = udsc_do_start_routine(uds, UDS_RID_get_cur_process, g_routine_opts, UDS_TRANSFER_ID_LEN, buf, &len);
	if (ret)
		return ret;
	*errcode = buf[4];
	*state = buf[5];
	*progress = buf[6];
	return 0;
}

int main(int argc, char *argv[])
{
	int ret;
	struct uds_tp *tp;
	struct udsc *uds;
	int opt;
	const char *server_ip = NULL;
	const char *filepath = NULL;
	int file_action = 0;
	
	struct in_addr in_server;
	
	while ((opt = getopt(argc, argv, "c:a:r:d:iefsb")) != -1) {
		switch (opt) {
		case 'c':
			server_ip = optarg;
			break;
		case 'a':
		case 'r':
		case 'd':
			filepath  = optarg;
			file_action = opt;
			break;
		case 'i': /*install fw*/
		case 'e':
		case 'f':
		case 's':
        case 'b':
			file_action = opt;
			break;
			
		default:
			printf("%s option argument\n", argv[0]);
			printf("\t -c server_ip : IP address of server, default is 127.0.0.1\n"
				   "\t -a filename : add file to server\n"
				   "\t -r filename : replace file on server\n"
				   "\t -d filename : delete file on server\n"
				   "\t -b          : Set OTA state to idle\n"
				   "\t -i          : Set OTA state to install\n"
				   "\t -e          : Set OTA state to activate\n"
				   "\t -f          : Set OTA state to finish\n"
				   "\t -s          : Get OTA State\n"
				);
			return -1;
		}
	}

	if (!server_ip) {
		printf("Please specify a server IP\n");
		show_doip_entity(0xFFFFFFFF);
		return 0;
	}

	if (inet_aton(server_ip, &in_server) == 0) {
		fprintf(stderr, "Invalid server ip: %s\n", server_ip);
		return -1;
	}
	
	doipc_init();
	udsc_init();
	
	tp = doipc_open(999, ntohl(in_server.s_addr));
	if (!tp) {
		printf("doipc_open fail\n");
		return -1;
	}
	
	uds = udsc_open(tp);
	if (!uds) {
		printf("udsc_open fail\n");
		return -2;
	}

	switch (file_action) {
	case 'a':
	case 'r':
		{
			struct uds_file *udsfile;
			uint8_t file_num = 1 + (argc - optind);
			int i;
			
			g_routine_opts[UDS_TRANSFER_ID_LEN] = file_num;
			ret = udsc_do_start_routine(uds, UDS_RID_download_file, g_routine_opts, sizeof(g_routine_opts), NULL, NULL);
			if (ret == 0) {		
				printf("udsc_do_start_routine ok\n");
			} else {
				printf("udsc_do_start_routine.ret = %d\n", ret);
				break;
			}

			for (i = 0; i < file_num; i++) {
				if (i > 0)
					filepath = argv[optind + (i-1)];
				
				udsfile = linux_open_uds_file(filepath, UDS_FILE_FLAG_READ);
				if (!udsfile) {
					printf("linux_open_uds_file(%s) fail\n", filepath);
					break;
				}
				ret = file_action == 'a' ? udsc_add_file(uds, udsfile, NULL) : udsc_replace_file(uds, udsfile, NULL);
				printf("%s file .ret = %d\n", file_action == 'a' ? "Add" : "Replace", ret);
				linux_close_uds_file(udsfile);
			}
		}
		break;
	
	case 'd':
		ret = udsc_delete_file(uds, filepath);
		break;
	case 'b':
		ret = set_ota_state(uds, OTA_IDLE);
		if (ret) {
			fprintf(stderr, "set ota state to IDLE fail %d\n", ret);
			break;
		}
		break;
	case 'i':
		ret = set_ota_state(uds, OTA_INSTALL_FW);
		if (ret) {
			fprintf(stderr, "set ota state to INSTALL fail %d\n", ret);
			break;
		}
		break;
	case 'e':
		ret = set_ota_state(uds, OTA_ACTIVATE);
		if (ret) {
			fprintf(stderr, "set ota state to ACTIVATE fail %d\n", ret);
			break;
		}
		break;
	case 'f':
		ret = set_ota_state(uds, OTA_FINISH);
		if (ret) {
			fprintf(stderr, "set ota state to FINISH fail %d\n", ret);
		}
		break;
	case 's':
		{
			uint8_t errcode;
			tOTAState state;
			uint8_t process;
			
			ret = get_ota_state(uds, &errcode, &state, &process);
			if (ret) {
				fprintf(stderr, "get_ota_state.ret = %d\n", ret);
				break;
			}
			printf("%d %d %d\n", errcode, state, process);
		}
		break;
	}

	udsc_close(uds);
	doipc_close(tp);

	udsc_uninit();
	doipc_uninit();
	return 0;
}

