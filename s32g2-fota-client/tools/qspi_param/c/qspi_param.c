/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct flash_write_unit {
	uint32_t header;
	uint32_t addr;
	uint32_t data;
};

typedef struct {
	uint32_t Header; /* 0h 4 Should be 5A5A5A5Ah. */
	uint32_t MCR; /* 4h 4 MCR values for targeted operation.*/
	uint32_t FLSHCR; /* 8h 4 FLSHCR values for targeted operation.*/
	uint32_t BUFGENCR; /*  Ch 4 BUFGENCR values for targeted operation.*/
	uint32_t DLLCR; /*  10h 4 Configuration to be programmed in DLLCRA or DLLCRB register
				depending on whether flash memory is connected to Port A or B.
				Only the DLLCR[30:4] bits are used—bit 31 and bits 0–3 are
				masked out. */
	uint32_t PARITYCR; /* 14h 4 Values to be programmed in case flash memory is enabled for
				parity. */
	uint32_t SFACR; /* 18h 4 Holds value to be configured in SFACR register. */
	uint32_t SMPR; /* 1Ch 4 Holds value to be configured in SMPR register for Phase 2
				configurations. */
	uint32_t DLCR; /* 20h 4 Data Learning Control Register configuration required as per
				desired Phase 2 configurations. */
	uint32_t SFLASH_1_SIZE; /* 24h 4 Flash 1 top address, applicable to either Port A or B. Only one
				port is supported during boot. Port selection depends on the
				value in BOOT_CFG[9]. */
	uint32_t SFLASH_2_SIZE; /* 28h 4 Flash 2 top address, applicable to either Port A or B. Only one
				port is supported during boot. Port selection depends on the
				value in BOOT_CFG[9]. */
	uint32_t DLPR; /* 2Ch 4 Data Learning Pattern Register, as applicable for required Phase
				2 configurations by user. */
	uint32_t SFAR; /* 30h 4 Serial flash memory address to be configured in Phase 2
				configuration. */
	uint32_t IPCR; /* 34h 4 Content to be programmed in IP Configuration register. */
	uint32_t TBDR; /* 38h 4 Data to be written in TBDR register. */
	uint8_t  DLL_BYPASS_EN; /* 3Ch 1 Enable DLL Bypass mode based operation in Phase 2 if above
				configurations correspond to this phase.
				0: Disable
				1: Enable */
	uint8_t  DLL_SLV_UPD_EN; /* 3Dh 1 Enable DLL SLV Update mode based operation in Phase 2 if
				above configurations correspond to this phase.
				0: Disable
				1: Enable */
	uint8_t  DLL_AUTO_UPD_EN; /* 3Eh 1 Enable DLL Auto Update mode based operation in Phase 2 if
				above configurations correspond to this phase.
				0: Disable
				1: Enable */
	uint8_t  IPCR_TRIGGER_EN; /* 3Fh 1 Writes IPCR field to IPCR register only if IPCR_TRIGGER_EN is
				1. */
	uint8_t SFLASH_CLK_FREQ; /* 40h 1 User-provided frequency (in MHz) for Phase 2 QuadSPI
				configuration. This frequency corresponds to the QSPI_1X_CLK
				clock. */
	uint8_t Reserved_41; /* 41h 1 Reserved */
	uint8_t Reserved_42; /* 42h 1 Reserved */
	uint8_t Reserved_43; /* 43h 1 Reserved */
	uint8_t COMMAND_SEQ[320]; /* 44h 320 User-provided LUT configuration to be used for read operations
				over the AHB interface. The LUT should be programmed as per
				requirements of the flash memory connected and the mode of
				operation selected, including clock, DDR, SDR, 1-bit, 4-bit, or 8-
				bit operation. The LUT sequence to be invoked during a read is
				controlled by the configuration provided in BUFGENCR. */
	struct flash_write_unit FLASH_WRITE_DATA[10]; /* 184h 120 An array of 10 structures. Each structure contains details of a
				command and related parameters to be sent to flash memory
				after Phase 1 and before Phase 2 */
	uint32_t pad;
} quadspi_param_t;

quadspi_param_t g_quadspi_param_dtr = {
	.Header 		= 0x5a5a5a5a,
	.MCR 			= 0x010f00cc,
	.FLSHCR 		= 0x00010303,
	.BUFGENCR 		= 0,
	.DLLCR 			= 0x82607004,
	.PARITYCR 		= 0,
	.SFACR 			= 0x00020000,
	.SMPR 			= 0x44000000,
	.DLCR 			= 0x40ff40ff,
	.SFLASH_1_SIZE  = 0x20000000,
	.SFLASH_2_SIZE  = 0x20000000,
	.DLPR 			= 0xaa553443,
	.SFAR 			= 0,
	.IPCR 			= 0,
	.TBDR 			= 0,
	.DLL_BYPASS_EN  = 0,
	.DLL_SLV_UPD_EN = 1,
	.DLL_AUTO_UPD_EN = 0,
	.IPCR_TRIGGER_EN = 0,
	.SFLASH_CLK_FREQ = 200,
	.COMMAND_SEQ = {
		/*
		CMD_DDR 0xEE, CMD_DDR 0x11
		ADDR_DDR 0x20, DUMMY 20,
		READ_DDR, STOP
		*/
		0xee, 0x47, 0x11, 0x47,
        0x20, 0x2b, 0x14, 0x0f,
		0x10, 0x3b, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
	},
	.FLASH_WRITE_DATA = {
		{0x00000006, 0x0, 0x0},
		{0x81800072, 0x0, 0x00000002},
	},
};

quadspi_param_t g_quadspi_param_str = {
	.Header 		= 0x5a5a5a5a,
	.MCR 			= 0x010f004c,
	.FLSHCR 		= 0x00000303,
	.BUFGENCR 		= 0,
	.DLLCR 			= 0x01200006,
	.PARITYCR 		= 0,
	.SFACR 			= 0x00000800,
	.SMPR 			= 0x00000020,
	.DLCR 			= 0x40ff40ff,
	.SFLASH_1_SIZE  = 0x20000000,
	.SFLASH_2_SIZE  = 0x20000000,
	.DLPR 			= 0xaa553443,
	.SFAR 			= 0,
	.IPCR 			= 0,
	.TBDR 			= 0,
	.DLL_BYPASS_EN  = 1,
	.DLL_SLV_UPD_EN = 0,
	.DLL_AUTO_UPD_EN = 0,
	.IPCR_TRIGGER_EN = 0,
	.SFLASH_CLK_FREQ = 200,
	.COMMAND_SEQ = {
		/*
		CMD_DDR 0xEE, CMD_DDR 0x11
		ADDR_DDR 0x20, DUMMY 20,
		READ_DDR, STOP
		*/
		0xec, 0x07, 0x13, 0x07,
        0x20, 0x0b, 0x14, 0x0f,
		0x01, 0x1f, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
	},
	.FLASH_WRITE_DATA = {
		{0x00000006, 0x0, 0x0},
		{0x81800072, 0x0, 0x00000001},
	},
};

static void help(FILE *out, const char *app)
{
	fprintf(out, "useage: %s [-d] [-c <clock MHz>] output_file\n", app);
	fprintf(out, "		-d : ddr mode, default is sdr\n");
	fprintf(out, "		-c clock : specify frequency of clock, max 200\n");
}

int main(int argc, char *argv[])
{
	int opt;
	bool ddr_mode = false;
	uint32_t clock_freq = 133;
	quadspi_param_t *param;
	
	while ((opt = getopt(argc, argv, "dc:")) != -1) {
       switch (opt) {
       case 'd':
           ddr_mode = true;
           break;
       case 'c':
           clock_freq = atoi(optarg);
		   if (clock_freq > 200)
		   		clock_freq = 200;
           break;
       default: /* '?' */
           help(stderr, argv[0]);
           return -1;
       }
	}
	
	if (optind >= argc) {
		fprintf(stderr, "Expected output file name after options\n");
		return -1;
	}

	param = ddr_mode ? &g_quadspi_param_dtr : &g_quadspi_param_str;

	param->SFLASH_CLK_FREQ = (uint8_t)clock_freq;
	memset(&param->COMMAND_SEQ[20], 0xff, sizeof(param->COMMAND_SEQ)-20);
	
	FILE *out_fp = fopen(argv[optind], "wb");

	if (!out_fp) {
		fprintf(stderr, "open file <%s> for write fail: %s\n", argv[1], strerror(errno));
		return -2;
	}

	if (fwrite(param, sizeof(*param), 1, out_fp) != 1) {
		fprintf(stderr, "write file <%s> fail: %s\n", argv[1], strerror(errno));
		return -3;
	}

	fclose(out_fp);
	
	return 0;
}

