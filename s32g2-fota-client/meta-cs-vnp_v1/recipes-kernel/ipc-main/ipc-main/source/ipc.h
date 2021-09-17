/*
 * Copyright 2019-2021 NXP
 *
 * SPDX-License-Identifier:  GPL-2.0
*/
/*
 * ipc.h
 *
 *  Created on: Mar 23, 2020
 *      Author: nxf50888
 */

#ifndef IPC_H_
#define IPC_H_
#include "ipc-shm.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ipc_rx_func_t)(void *arg, int ch, void *buff, uint32_t size);

int ipc_reg_rx_callback(uint32_t ch, ipc_rx_func_t func, void *arg);
bool ipc_ready(void);
#ifdef __cplusplus
}
#endif
#endif /* IPC_H_ */
