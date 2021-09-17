/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>  

#include "../fotav.h"
#include "it_test/inte_test.h"
#include "it_test/it_deamon.h"
#include "metadata_test.h"

#if UNIT_TEST > 0

#define HTTP_CLIENT_TEST	ON
#define CJSON_TEST		OFF
#define REPO_TEST		OFF
#define METADATA_TEST		OFF
#define METADATA_VERIFICATION_TEST OFF

extern int32_t http_client_test();
extern int32_t json_test();
extern int32_t repo_test(void);
extern int32_t metadata_test(void);
extern int32_t metadata_verify(void);
#endif

int main(int argc, char *argv[])
{
#if (UNIT_TEST == OFF)

	fota_main();

	fota_inte_test();
	
	test_thread(NULL);
	
#elif (UNIT_TEST == ON)
	/* http client test */
#if (HTTP_CLIENT_TEST == ON)
	http_client_test();
#endif

#if (CJSON_TEST) > 0
	json_test();
#endif

#if (REPO_TEST > 0)
	repo_test();
#endif

#if (METADATA_TEST > 0)
	metadata_test();
#endif

#if(METADATA_VERIFICATION_TEST>0)
metadata_verify();
#endif

#endif
}
