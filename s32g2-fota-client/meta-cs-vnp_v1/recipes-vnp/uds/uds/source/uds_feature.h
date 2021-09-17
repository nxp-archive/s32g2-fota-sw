/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

enum {
	/*Diagnostic and Communication Management functional unit*/
	UDS_SI_DiagSessionControl = 0x10,
	UDS_SI_ECUReset = 0x11,
	UDS_SI_SecurityAccess = 0x27,
	UDS_SI_CommunicationControl = 0x28,
	UDS_SI_TeseterPresent = 0x3E,
	UDS_SI_AccessTimingParameter = 0x83,
	UDS_SI_SecuredDataTransmission = 0x84,
	UDS_SI_ControlDTCSet = 0x85,
	UDS_SI_ResponseOnEvent = 0x86,
	UDS_SI_LinkControl = 0x87,

	/*Data Transmission functional unit*/
	UDS_SI_ReadDataByIdentifier = 0x22,
	UDS_SI_ReadMemoryByAddress = 0x23,
	UDS_SI_ReadScalingDataByIdentifier = 0x24,
	UDS_SI_ReadDataByPeriodicIdentifier = 0x2A,
	UDS_SI_DynamicallyDefineDataIdentifier = 0x2C,
	UDS_SI_WriteDataByIdentifier = 0x2E,
	UDS_SI_WriteMemoryByAddress = 0x3D,

	/*Stored Data Transmission functional unit*/
	UDS_SI_ClearDiagnosticInformation = 0x14,
	UDS_SI_ReadDTCInformation = 0x19,

	/*InputOutput Control functional unit*/
	UDS_SI_InputOutputControlByIdentifier = 0x2F,

	/*Routine functional unit*/
	UDS_SI_RoutineControl = 0x31,

	/*Upload Download functional unit*/
	UDS_SI_RequestDownload = 0x34,
	UDS_SI_RequestUpload = 0x35,
	UDS_SI_TransferData = 0x36,
	UDS_SI_RequestTransferExit = 0x37,
	UDS_SI_RequestFileTransfer = 0x38
};

enum RoutineControl_SUB_FI {
	UDS_SUB_FI_startRoutine = 0x01,
	UDS_SUB_FI_stopRoutine = 0x02,
	UDS_SUB_FI_requestRoutineResults = 0x03
};

struct UDS_RoutineControl_req {
	uint8_t sid;
	uint8_t sub_func;
	uint8_t routine_id;
};

/*valid range: 0x0200 - 0xDFFF */
enum UDS_RoutineID {
	UDS_RID_check_crc = 0x0202,
	UDS_RID_download_file = 0x0304,
	UDS_RID_get_cur_process = 0x0305,
	UDS_RID_set_cur_process = 0x0306,
	UDS_RID_get_ecu_info = 0x03FF,
	UDS_RID_erase_flash = 0xFF00,
	UDS_RID_write_fw = 0xFF01
};

enum RequestFileTransfer_modeOfOperation {
	UDS_RFT_MODE_AddFile = 0x1,
	UDS_RFT_MODE_DeleteFile = 0x2,
	UDS_RFT_MODE_ReplaceFile = 0x3,
	UDS_RFT_MODE_ReadFile = 0x4,
	UDS_RFT_MODE_ReadDir = 0x5
};

enum ECUReset_resetType {
	UDS_ECU_hardReset = 0x01,
	UDS_ECU_keyOffOnReset = 0x02,
	UDS_ECU_softReset = 0x03,
	UDS_ECU_enableRapidPowerShutDown = 0x04,
	UDS_ECU_disableRapidPowerShutDown = 0x05
};

#define UDS_TRANSFER_ID_LEN 16
#define UDS_TRANSFER_ID 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, \
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,

typedef enum
{
	OTA_IDLE,
	OTA_PROCESSING,
	OTA_PROCESSED,
	OTA_INSTALL_FW,
	OTA_INSTALLING_FW,
	OTA_INSTALLED_FW,
	OTA_ACTIVATE,
	OTA_ACTIVATING,
	OTA_ACTIVATED,
	OTA_FINISH,
} tOTAState;

#ifdef __cplusplus
}
#endif

