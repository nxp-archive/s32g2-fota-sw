/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#ifndef _CMD_H
#define _CMD_H
#include<stdio.h>
#include<stdint.h>
#include<stdbool.h>
#include"uart.h"
#include"cmd_config.h"
#include"cmd_queue.h"

enum screen_id_sets{
   master_screen_id=0,
   discription_e_screen_id=1,
   discription_c_screen_id=2,
   fotademo_e_screen_id=3,
   request_e_screen_id=4,
   request_c_screen_id=5,
   downloading1file_screen_id=6,
   verifying1file_e_screen_id=7,
   verifydone_e_screen_id=8,
   installing1file_e_screen_id=9,
   activate_e_screen_id=10,
   successful_e_screen_id=11,
   failed_e_screen_id=12,
   reseting_e_screen_id=13
};
enum customer_reply_flag{
    customer_agree_flag=0,
    customer_later_agree_flag=1,
    customer_disagree_flag=2,
    error_rec_message_flag=3
};
void initmap(void);
void setTextValue(int screen_id,int control_id,char *str);
void switchScreen(int screen_id);
bool updateAllScreenString(char* title_string, char* state_string);
bool setMasterScrenn(void);
bool setDiscription_EScreen(char* discription_string);
bool setDiscription_CScreen(void);
bool setFOTADemo_EScreen(char* GW_string,char* DCU_string,char* ECU1_string,
                        char* ECU2_string,char* ECU3_string);
bool setRequest_EScreen(char* donwload_info_string);
bool setRequest_CScreen(void);
bool setDownloading1file_EScreen(int download_progress_value,char* download_file_info_string);
bool setVerifying1file_EScreen(int download_progress_value,char* download_file_info_string);
bool setVerifyDone_EScreen(void);
bool setInstalling1file_EScreen(int download_progress_value,char* download_file_info_string);
bool setActivate_EScreen(void);
bool setSuccessful_EScreen(void);
bool setFailed_EScreen(void);
bool setLogText(int screen_id, char* message);
void clearAllLogText(void);


bool getScreenCMD(void);
enum customer_reply_flag getMsgFromRequest_EScreen(void);
enum customer_reply_flag getMsgFromVerifyDone_EScreen(void);
enum customer_reply_flag getMsgFromActivateScreen(void);
//bool processCMD(char* CMD);

void clearAllScreenLog(void);
#endif