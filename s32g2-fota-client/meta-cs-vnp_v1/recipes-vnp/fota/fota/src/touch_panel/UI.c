/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
/**Includes*********************************************************************/
#include "UI.h"
#include "cmd.h"

/**
  * @brief     UI thread function
  * @param     arg
  * @retval    None
  */
void *UI_Thread(void *arg)
{
	bool customer_reply_flag = true;
	bool firmware_successful_install_flag = true;
	printf("access UI Thread\n");
	pthread_detach(pthread_self());

	initialUart();

	initmap();
	clearAllScreenLog();
	updateAllScreenString("Vechcle network", "BUSY");

	//FOTADemo_EScreen simulation
	setFOTADemo_EScreen("S32G274A Version:1.0",
						"S32K Version:1.0",
						"ECU1 Version:2.0",
						"ECU2 Version:1.0",
						"ECU3 Version:1.0");
	enableFOTADemoModule();
	switchScreen(fotademo_e_screen_id);
	usleep(5000000);

	//Request_E screen simulation
	switchScreen(request_e_screen_id);
	customer_reply_flag = setRequest_EScreen("S32G firmware Download info");
	if (customer_reply_flag == false)
	{
		switchScreen(fotademo_e_screen_id);
		return ;
	}

	//Downloading1file_e screen simulation
	switchScreen(downloading1file_screen_id);
	setDownloading1file_EScreen(10, "Download File1: 10KB");
	setLogText(downloading1file_screen_id, "Download File1: 10KB");
	usleep(1000000);
	setDownloading1file_EScreen(20, "Download File2: 10KB");
	setLogText(downloading1file_screen_id, "Download File2: 10KB");
	usleep(1000000);
	setDownloading1file_EScreen(80, "Download File3: 60KB");
	setLogText(downloading1file_screen_id, "Download File3: 60KB");
	usleep(1000000);
	setDownloading1file_EScreen(100, "Download File4: 20KB");
	setLogText(downloading1file_screen_id, "Download File4: 20KB");
	usleep(1000000);

	//Verifying1file_e_screen simulation
	switchScreen(verifying1file_e_screen_id);
	setVerifying1file_EScreen(20, "Verify File1: 20KB");
	setLogText(verifying1file_e_screen_id, "Verify File1: 20KB");
	usleep(1000000);
	setVerifying1file_EScreen(30, "Verify File2: 10KB");
	setLogText(verifying1file_e_screen_id, "Verify File2: 10KB");
	usleep(1000000);
	setVerifying1file_EScreen(70, "Verify File3: 40KB");
	setLogText(verifying1file_e_screen_id, "Verify File3: 40KB");
	usleep(1000000);
	setVerifying1file_EScreen(100, "Verify File4: 30KB");
	setLogText(verifying1file_e_screen_id, "Verify File4: 20KB");
	usleep(1000000);

	//VerifyingDone_e_screen simulation
	switchScreen(verifydone_e_screen_id);
	customer_reply_flag = setVerifyDone_EScreen();
	if (customer_reply_flag == false)
	{
		switchScreen(fotademo_e_screen_id);
		return;
	}

	//Install1file_e_screen simulation
	switchScreen(installing1file_e_screen_id);
	setInstalling1file_EScreen(30, "Install File1: 30KB");
	setLogText(installing1file_e_screen_id, "Install File1: 30KB");
	usleep(1000000);
	setInstalling1file_EScreen(60, "Install File2: 30KB");
	setLogText(installing1file_e_screen_id, "Install File2: 30KB");
	usleep(1000000);
	setInstalling1file_EScreen(100, "Install File3: 40KB");
	setLogText(installing1file_e_screen_id, "Install File3: 40KB");
	usleep(1000000);

	//Activate_e_screen simulation
	switchScreen(activate_e_screen_id);
	customer_reply_flag = setActivate_EScreen();
	if (customer_reply_flag == false)
	{
		switchScreen(fotademo_e_screen_id);
		return ;
	}
	else
	{
		firmware_successful_install_flag = true;
	};

	//
	if (firmware_successful_install_flag == true)
	{
		switchScreen(successful_e_screen_id);
	}
	else
	{
		switchScreen(failed_e_screen_id);
	}
	usleep(10000000);

	pthread_exit(NULL);
}
