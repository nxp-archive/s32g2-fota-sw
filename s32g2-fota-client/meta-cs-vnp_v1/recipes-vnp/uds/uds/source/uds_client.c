/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#include "pl_log.h"
#include "pl_string.h"
#include "pl_stdlib.h"
#include "uds_client.h"

#define MAX(a,b) ((a) >= (b) ? (a) : (b))
#define MIN(a,b) ((a) <= (b) ? (a) : (b))

struct udsc {
	struct uds_tp *tp;
	uint32_t trans_block_size;
	uint8_t trans_block_seq;
};

static int receive_with_pending(struct uds_tp *tp, struct tp_buff *tpb, uint8_t routine)
{
	int ret;

RECEIVE_AGAIN:
	ret = tp->ops->recv(tp, tpb);
	if (ret < 0) {
		PL_WARN_LOC("recv fail. ret\n", ret);
		return ret;
	}

	if (tpb->len == 3
		&& tpb->data[0] == 0x7F
//		&& tpb->data[1] == routine
		&& tpb->data[2] == 0x78     /* server side is requesting more time */
		&& routine != OTA_INSTALL_FW /* do NOT wait in case INSTALL command */) {

        PL_WARN_LOC("data[0-2]: %02x %02x %02x\n", tpb->data[0], tpb->data[1], tpb->data[2]);

		goto RECEIVE_AGAIN;
		}
	
	return 0;
}

int udsc_do_start_routine(struct udsc *uds, enum UDS_RoutineID rid, 
				const uint8_t *opt, uint16_t opt_len,
				uint8_t *output, uint16_t *output_len)
{
	int ret;
	struct uds_tp *tp = uds->tp;
	struct tp_buff *tpb;

	tpb = tp->ops->alloc(tp, TP_TX);
	if (!tpb)
		return -2;
	tpb->data[0] = UDS_SI_RoutineControl;
	tpb->data[1] = UDS_SUB_FI_startRoutine;
	tpb->data[2] = rid >> 8;
	tpb->data[3] = (uint8_t)rid;
	tpb->len = 4;
	if (opt && opt_len) {
		memcpy(&tpb->data[4], opt, opt_len);
		tpb->len += opt_len;
	}
	
	ret = tp->ops->send(tp, tpb);
	if (ret < 0)
		return ret;
//	PL_INFO_LOC("send success\n");
	/* check reply message */
	tpb = tp->ops->alloc(tp, TP_RX);

	ret = receive_with_pending(tp, tpb, opt[UDS_TRANSFER_ID_LEN]);
	if (ret)
		goto FREE;
	
	if (rid == UDS_RID_get_cur_process)
		tpb->data[0] |= 0x40;
	
	if (tpb->data[0] != (0x40 | UDS_SI_RoutineControl)) {
		PL_WARN_LOC("data[0-2]: %02x %02x %02x\n", tpb->data[0], tpb->data[1], tpb->data[2]);
		ret = -3;
		goto FREE;
	}

	/* return responsed data */
	if (output && output_len) {
		const uint16_t len = MIN(tpb->len, *output_len);

		memcpy(output, tpb->data, len);
		*output_len = len;
	}
	ret = 0;
FREE:
	tp->ops->free(tp, tpb);
	return ret;
}

/*
static int valid_byte_num(uint32_t val)
{
	int num = 0;

	do {
		val >>= 8;
		num++;
	} while (val);
	return num;
}
*/
static int fill_trans_file_req(uint8_t *data, uint8_t mode, struct uds_file *file)
{
	const int name_len = strlen(file->name);
	
	data[0] = UDS_SI_RequestFileTransfer;
	data[1] = mode;
	data[2] = (uint8_t)(name_len >> 8);
	data[3] = (uint8_t)name_len;
	memcpy(&data[4], file->name, name_len);
	if (mode != UDS_RFT_MODE_AddFile
		&& mode != UDS_RFT_MODE_ReplaceFile)
		return 4 + name_len;
	
	data[4 + name_len] = 0x00; /* uncompressed */
	
	if (file->uncompressed_size & 0xffff0000) {
		data[5 + name_len] = 4;

		/* uncompressed size */
		data[6 + name_len] = file->uncompressed_size >> 24;
		data[7 + name_len] = file->uncompressed_size >> 16;
		data[8 + name_len] = file->uncompressed_size >> 8;
		data[9 + name_len] = file->uncompressed_size;

		/* compressed size */
		data[10 + name_len] = data[6 + name_len];
		data[11 + name_len] = data[7 + name_len];
		data[12 + name_len] = data[8 + name_len];
		data[13 + name_len] = data[9 + name_len];
		return 14 + name_len;
	} else {
		data[5 + name_len] = 2;

		/* uncompressed size */
		data[6 + name_len] = file->uncompressed_size >> 8;
		data[7 + name_len] = file->uncompressed_size;

		/* compressed size */
		data[8 + name_len] = data[6 + name_len];
		data[9 + name_len] = data[7 + name_len];
		return 10 + name_len;
	}
}

static int udsc_do_trans_file_req(struct udsc *uds, uint8_t mode, struct uds_file *file)
{
	int ret;
	struct uds_tp *tp = uds->tp;
	struct tp_buff *tpb;
		
	/* send file transfer requestion */
	tpb = tp->ops->alloc(tp, TP_TX);
	tpb->len = fill_trans_file_req(tpb->data, mode, file);

	ret = tp->ops->send(tp, tpb);
	if (ret < 0) {
		PL_WARN_LOC("send() ret = %d\n", ret);
		return ret;
	} else if (ret == 'x') {
        goto FREE;
	}

    
	tpb = tp->ops->alloc(tp, TP_RX);
	
	ret = receive_with_pending(tp, tpb, UDS_SI_RequestFileTransfer);
	if (ret)
		goto FREE;
	
	if (tpb->data[0] != (0x40 | UDS_SI_RequestFileTransfer)
		|| tpb->data[1] != mode) {
		PL_WARN_LOC("reply() data[0-2] = %02x %02x %02x\n", tpb->data[0], tpb->data[1], tpb->data[2]);
		ret = -3;
		goto FREE;
	}
	ret = 0;

	if (mode != UDS_RFT_MODE_DeleteFile) {
		int byte_num = tpb->data[2];
		int i;
	
		uds->trans_block_size = tpb->data[3];
		for (i = 1; i < byte_num; i++) {
			uds->trans_block_size <<= 8;
			uds->trans_block_size |= tpb->data[3 + i];
		}
		uds->trans_block_size = MIN(uds->trans_block_size, uds->tp->mtu - 2);
		uds->trans_block_seq = 0x1;
	}
FREE:
	tp->ops->free(tp, tpb);
	return ret;
}

static int udsc_do_trans_data_req(struct udsc *uds, struct tp_buff *tpb)
{
	int ret;
	struct uds_tp *tp = uds->tp;
	struct tp_buff *rx_tpb;
	
	tpb->data[0] = UDS_SI_TransferData;
	tpb->data[1] = uds->trans_block_seq;

	ret = tp->ops->send(tp, tpb);
	if (ret < 0) {
		PL_WARN_LOC("send() ret = %d\n", ret);
		return ret;
	}

	rx_tpb = tp->ops->alloc(tp, TP_RX);
	
	ret = receive_with_pending(tp, rx_tpb, UDS_SI_TransferData);
	if (ret)
		goto FREE;
	
	if (rx_tpb->data[0] != (0x40 | UDS_SI_TransferData)
		|| rx_tpb->data[1] != uds->trans_block_seq) {
		PL_WARN_LOC("recv() data[0-2] = %02x %02x %02x\n", rx_tpb->data[0], rx_tpb->data[1], rx_tpb->data[2]);
		ret = -3;
		goto FREE;
	}
	ret = 0;
	uds->trans_block_seq++;
FREE:
	tp->ops->free(tp, rx_tpb);
	return ret;
}

static int udsc_do_trans_exit(struct udsc *uds)
{
	int ret;
	struct uds_tp *tp = uds->tp;
	struct tp_buff *tpb;

	tpb = tp->ops->alloc(tp, TP_TX);
	tpb->data[0] = UDS_SI_RequestTransferExit;
	tpb->len = 1;
	ret = tp->ops->send(tp, tpb);
	if (ret < 0) {
		PL_WARN_LOC("send() ret = %d\n", ret);
		return ret;
	}

	tpb = tp->ops->alloc(tp, TP_RX);
	
	ret = receive_with_pending(tp, tpb, UDS_SI_RequestTransferExit);
	if (ret)
		goto FREE;
	
	if (tpb->data[0] != (0x40 | UDS_SI_RequestTransferExit)) {
		PL_WARN_LOC("recv data[0] = %02x\n", tpb->data[0]);
		ret = -3;
		goto FREE;
	}
	ret = 0;

FREE:
	tp->ops->free(tp, tpb);
	return ret;
}

static int udsc_download_file(struct udsc *uds, struct uds_file *file, udsc_cb callback)
{
	int ret;
	uint32_t remain_len;
	struct tp_buff *tpb;
	
	remain_len = file->uncompressed_size;
	while (remain_len) {
		tpb = uds->tp->ops->alloc(uds->tp, TP_TX);
		
		/*reserved 2 for SID and block sequence counter */
		ret = file->read(file, tpb->data + 2, uds->trans_block_size);
		if (ret < 0) {
			uds->tp->ops->free(uds->tp, tpb);
			return -2;
		}
		tpb->len = ret + 2;
		remain_len -= ret;
		
		ret = udsc_do_trans_data_req(uds, tpb);
		if (ret < 0)
			return -3;

        if (callback != NULL) {
            int progress;
            progress = 100 - ((remain_len * 100) / file->uncompressed_size);
            callback(file->name, progress);
        }
	}

	/* exit trans */
	ret = udsc_do_trans_exit(uds);
	if (ret == 0) {
		uint8_t opts[4];

		opts[0] = (uint8_t)(file->crc >> 24);
		opts[1] = (uint8_t)(file->crc >> 16);
		opts[2] = (uint8_t)(file->crc >> 8);
		opts[3] = (uint8_t)file->crc;
		ret = udsc_do_start_routine(uds, UDS_RID_check_crc, opts, 4, NULL, NULL);
	}
	return ret;
}

int udsc_add_file(struct udsc *uds, struct uds_file *file, udsc_cb callback)
{
	int ret;
	
	if (!file || !file->read || !file->uncompressed_size)
		return -1;
	
	ret = udsc_do_trans_file_req(uds, UDS_RFT_MODE_AddFile, file);
	if (ret < 0) {
		PL_WARN_LOC("udsc_do_trans_file_req.ret = %d\n", ret);
		return ret;
	} else if (ret == 'x') {
        PL_WARN_LOC("server canceled the transfer\n");
        return 0;
	}

	return udsc_download_file(uds, file, callback);
}

int udsc_replace_file(struct udsc *uds, struct uds_file *file, udsc_cb callback)
{
	int ret;
	
	if (!file || !file->read || !file->uncompressed_size)
		return -1;
	
	ret = udsc_do_trans_file_req(uds, UDS_RFT_MODE_ReplaceFile, file);
	if (ret < 0){
		PL_WARN_LOC("udsc_do_trans_file_req.ret = %d\n", ret);
		return ret;
	} else if (ret == 'x') {
        PL_WARN_LOC("server canceled the transfer\n");
        return 0;
	}

	return udsc_download_file(uds, file, callback);
}

int udsc_delete_file(struct udsc *uds, const char *filename)
{
	struct uds_file file;
	int ret;
	
	file.name = filename;
	ret = udsc_do_trans_file_req(uds, UDS_RFT_MODE_DeleteFile, &file);
	if (ret){
		PL_WARN_LOC("udsc_do_trans_file_req.ret = %d\n", ret);
	}
	return ret;
}

int udsc_reset_ecu(struct udsc *uds, enum ECUReset_resetType type)
{
	int ret;
	struct uds_tp *tp = uds->tp;
	struct tp_buff *tpb;
		
	tpb = tp->ops->alloc(tp, TP_TX);
	tpb->data[0] = UDS_SI_ECUReset;
	tpb->data[1] = (uint8_t)type; /* hard reset */
	tpb->len = 2;
	ret = tp->ops->send(tp, tpb);
	if (ret < 0) {
		PL_WARN_LOC("send() ret = %d\n", ret);
		return ret;
	}

	tpb = tp->ops->alloc(tp, TP_RX);
RETRY:
	ret = receive_with_pending(tp, tpb, UDS_SI_ECUReset);
	if (ret)
		goto FREE;
	
	if (tpb->data[0] == 0x7F) {
		if (tpb->data[1] == 0x00 && tpb->data[2] == 0x78)
			goto RETRY;
		
		PL_WARN_LOC("Negtive rsp recv data[1] = %02x, data[2] = %02x\n", tpb->data[1], tpb->data[2]);
		ret = -3;
		goto FREE;
	}
	
	if (tpb->data[0] != (0x40 | UDS_SI_RequestTransferExit)
		|| tpb->data[1] != type) {
		PL_WARN_LOC("recv data[0] = %02x, data[1] = %02x\n", tpb->data[0], tpb->data[1]);
		ret = -3;
		goto FREE;
	}
	ret = 0;

FREE:
	tp->ops->free(tp, tpb);
	return ret;
}

int udsc_init(void)
{
	return 0;
}

void udsc_uninit(void)
{

}

struct udsc *udsc_open(struct uds_tp *tp)
{
	struct udsc *uds;
	
	if (!tp)
		return NULL;

	uds = pl_malloc(sizeof(struct udsc));
	if (!uds)
		return NULL;
	
	uds->tp = tp;
	return uds;
}

void udsc_close(struct udsc *uds)
{
	pl_free(uds);
}

