/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#include "cmd.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wait.h>
#include <errno.h>
#include <netdb.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <stdint.h>
#include <fcntl.h>
struct textqueue* map_screenid2queueid[256];
bool map_init_flag = false;
char cmd_rec[9];
int screen_id_later_flag=0;
int duration_later = 5000000;//us

bool getScreenCMD(void)
{
    read(fd,cmd_rec,9);
    return true;
}
bool sendBeginCMD(void)
{
    char cmd_begin[]={0XEE};
    write(fd,cmd_begin,1);
}
bool sendENDCMD(void)
{
    char cmd_end[]={0XFF,0XFC,0XFF,0XFF};
    write(fd,cmd_end,4);
}
void setTextValue(int screen_id,int control_id,char *str)
{
    if(str == NULL)
    {
        return ;
    }
    else
    {          
        char cmd_kind[]={0XB1,0x10};
        char screen_id_cmd[]={0x00,screen_id};
        char control_id_cmd[]={0x00,control_id};
        sendBeginCMD();
        write(fd,cmd_kind,2);
        write(fd,screen_id_cmd,2);
        write(fd,control_id_cmd,2);
        write(fd,str,strlen(str));
        sendENDCMD();
    }
    return ;
}
void switchScreen(int screen_id)
{
    char cmd_kind[]={0XB1,0x00};
    char screen_id_cmd[]={0x00,screen_id};
    sendBeginCMD();
    write(fd,cmd_kind,2);
    write(fd,screen_id_cmd,2);
    sendENDCMD();
    return ;
}
bool setIcons(int screen_id,int control_id,int frame_id)
{
    char cmd_kind[] = {0xB1,0x23};
    char screen_id_cmd[]={0x00,screen_id};
    char control_id_cmd[]={0x00,control_id};
    char frame_id_cmd[]={frame_id};
    sendBeginCMD();
    write(fd,cmd_kind,2);
    write(fd,screen_id_cmd,2);
    write(fd,control_id_cmd,2);
    write(fd,frame_id_cmd,1);
    sendENDCMD();
    return true;
}
void setProgressBarValue(int screen_id,int control_id,int bar_value)
{
    if(bar_value<0)
    {
        return ;
    }
    else
    {
        char cmd_kind[]={0XB1,0x10};
        char screem_id_cmd[]={0x00,screen_id};
        char control_id_cmd[]={0x00,control_id};
        char bar_value_cmd[]={0x00,0x00,0x00,bar_value};
        sendBeginCMD();
        write(fd,cmd_kind,2);
        write(fd,screem_id_cmd,2);
        write(fd,control_id_cmd,2);
        write(fd,bar_value_cmd,4);
        sendENDCMD();
        return ;
    }
}
bool compareCMD(char cmd1[], char cmd2[], int cmd_length)
{
    for(int i=0;i<cmd_length;i++)
    { 
        if(cmd1[i] != cmd2[i])
        {
            return false;
        }
    }
    return true;
}
bool scrollText(int screen_id,struct textqueue* text_queue, char* message)
{
    cmd_queue_push(text_queue,message);
    setTextValue(screen_id,23,text_queue->data[0]);
    setTextValue(screen_id,24,text_queue->data[1]);
    setTextValue(screen_id,25,text_queue->data[2]);
    return true;
}
void return2LaterScreen(void)
{
    switchScreen(screen_id_later_flag);
    return ;
}

/**************************
 * scroll the text of log box 
 *  ************************/
void initmap(void)
{
    map_screenid2queueid[fotademo_e_screen_id]=&fotademo_e_queue;
    map_screenid2queueid[request_e_screen_id]=&request_e_queue;
    map_screenid2queueid[downloading1file_screen_id]=&downloading1file_e_queue;
    map_screenid2queueid[verifying1file_e_screen_id]=&verifying1file_e_queue;
    map_screenid2queueid[verifydone_e_screen_id]=&verifydone_e_queue;
    map_screenid2queueid[installing1file_e_screen_id]=&installing1file_e_queue;
    map_screenid2queueid[activate_e_screen_id]=&acitvate_e_queue;
    map_screenid2queueid[failed_e_screen_id]=&acitvate_e_queue;
    map_init_flag = true;
}
bool setLogText(int screen_id, char* message)
{
    if(map_init_flag)
    {
        scrollText(screen_id,map_screenid2queueid[screen_id],message);
    }
    else
    {
        printf("Please use initmap() function firstly!");
    }
    return true;
}

void clearAllLogText(void)
{
    int lines = 0;
    
    for (lines = 0; lines < 4; lines++) {
        setLogText(request_e_screen_id, " ");
        setLogText(downloading1file_screen_id, " ");
        setLogText(verifying1file_e_screen_id, " ");
        setLogText(verifydone_e_screen_id, " ");
        setLogText(installing1file_e_screen_id, " ");
        setLogText(activate_e_screen_id, " ");
        setLogText(failed_e_screen_id, " ");
    }
}



bool updateAllScreenString(char* title_string, char* state_string)
{
    enum screen_id_sets screen_id= fotademo_e_screen_id;
    setTextValue(screen_id,5,title_string);
    setTextValue(screen_id,22,state_string);
    return true;
}
bool setMasterScreen(void)
{
    
    return true;
};
bool setDiscription_EScreen(char* discription_string)
{
    int screen_id=1;
    int control_id=8;
    setTextValue(screen_id,control_id,discription_string);
    return true;
};
bool setDiscription_CScreen(void);



bool setFOTADemo_EScreen(char* GW_string,char* DCU_string,char* ECU1_string,
                        char* ECU2_string,char* ECU3_string)
{
    int screen_id=fotademo_e_screen_id;
    int DCU_control_id=11;
    int ECU1_control_id=12;
    int ECU2_control_id=13;
    int ECU3_control_id=14;
    int GW_control_id=15;

    int DCU_icon_control_id=7;
    int ECU1_icon_control_id=8;
    int ECU2_icon_control_id=9;
    int ECU3_icon_control_id=10;
    if(GW_string !=NULL)
    {
        setTextValue(screen_id,GW_control_id,GW_string);
    }
    if(DCU_string != NULL)
    {
        setTextValue(screen_id,DCU_control_id,DCU_string);
        setIcons(screen_id,DCU_icon_control_id,1);
    }
    else
    {
        setTextValue(screen_id,DCU_control_id,"Not connected");
        setIcons(screen_id,DCU_icon_control_id,0);
    }
    
    if(ECU1_string != NULL)
    {
        setTextValue(screen_id,ECU1_control_id,ECU1_string);
        setIcons(screen_id,ECU1_icon_control_id,1);
    }
    else
    {
        setTextValue(screen_id,ECU1_control_id,"Not connected");
        setIcons(screen_id,ECU1_icon_control_id,0);
    }
    
    if(ECU2_string != NULL)
    {
        setTextValue(screen_id,ECU2_control_id,ECU2_string);
        setIcons(screen_id,ECU2_icon_control_id,1);
    }
    else
    {
        setTextValue(screen_id,ECU2_control_id,"Not connected");
        setIcons(screen_id,ECU2_icon_control_id,0);
    }
    
    if(ECU3_string != NULL)
    {
        setTextValue(screen_id,ECU3_control_id,ECU3_string);
        setIcons(screen_id,ECU3_icon_control_id,1);
    }
    else
    {
        setTextValue(screen_id,ECU3_control_id,"Not connected");
        setIcons(screen_id,ECU3_icon_control_id,0);
    } 
    return true;
};



enum customer_reply_flag getMsgFromRequest_EScreen(void)
{
    getScreenCMD();
    if(compareCMD(msg_rec_S32G_updates,cmd_rec,9))
    {
        return customer_agree_flag; 
    }
    else if(compareCMD(msg_rec_S32G_later_updates,cmd_rec,9))
    {
        return customer_later_agree_flag;
    }
    else if(compareCMD(msg_rec_S32G_unupdates,cmd_rec,9))
    {
        return customer_disagree_flag;
    }
    else
    {
        return error_rec_message_flag;
    } 
};
bool setRequest_EScreen(char* donwload_info_string)
{
    int screen_id=request_e_screen_id;
    int donwload_info_control_id=7;
    setTextValue(screen_id,donwload_info_control_id,donwload_info_string);
    while(1)
    {
        enum customer_reply_flag customer_reply=getMsgFromRequest_EScreen();
        if(customer_reply == customer_agree_flag)
        {
            printf("customer agree to updates firmware\n");
            return true;
        }
        else if(customer_reply == customer_disagree_flag)
        {
            printf("customer disagree to updates firmware\n");
            return false;
        }
        else if(customer_reply == customer_later_agree_flag)
        {
            printf("customer later agree to updates firmware\n");
            switchScreen(master_screen_id);
            usleep(duration_later);
            switchScreen(request_e_screen_id);
        }
        else
        {
             printf("error receive message\n");
        }
    }
};
bool setRequest_CScreen(void)
{
    return true;
};

bool setDownloading1file_EScreen(int download_progress_value,char* download_file_info_string)
{
    int screen_id=downloading1file_screen_id;
    int download_progress_control_id=8;
    int download_file_info_control_id=7;
    setProgressBarValue(screen_id,download_progress_control_id,download_progress_value);
    setTextValue(screen_id,download_file_info_control_id,download_file_info_string);
    return true ;
};
bool setVerifying1file_EScreen(int download_progress_value,char* download_file_info_string)
{
    int screen_id=verifying1file_e_screen_id;
    int download_progress_control_id=8;
    int download_file_info_control_id=7;
    setProgressBarValue(screen_id,download_progress_control_id,download_progress_value);
    setTextValue(screen_id,download_file_info_control_id,download_file_info_string);
    return true ;
};

enum customer_reply_flag getMsgFromVerifyDone_EScreen(void)
{
    getScreenCMD();
    if(compareCMD(msg_rec_S32G_install,cmd_rec,9))
    {
        return customer_agree_flag; 
    }
    else if(compareCMD(msg_rec_S32G_later_install,cmd_rec,9))
    {
        return customer_later_agree_flag;
    }
    else if(compareCMD(msg_rec_S32G_uninstall,cmd_rec,9))
    {
        return customer_disagree_flag;
    }
    else
    {
        return error_rec_message_flag;
    } 
};
bool setVerifyDone_EScreen(void)
{
    while(1)
    {
        enum customer_reply_flag customer_reply=getMsgFromVerifyDone_EScreen();
        if(customer_reply == customer_agree_flag)
        {
            printf("customer agree to install firmware\n");
            return true;
        }
        else if(customer_reply == customer_disagree_flag)
        {
            printf("customer disagree to install firmware\n");
            return false;
        }
        else if(customer_reply == customer_later_agree_flag)
        {
            printf("customer later agree to install firmware\n");
            switchScreen(master_screen_id);
            usleep(duration_later);
            switchScreen(verifydone_e_screen_id);
        }
        else
        {
             printf("error receive message\n");
        }
    }
    return true;
};
bool setInstalling1file_EScreen(int download_progress_value,char* download_file_info_string)
{
    int screen_id=installing1file_e_screen_id;
    int download_progress_control_id=8;
    int download_file_info_control_id=7;
    setTextValue(screen_id,download_file_info_control_id,download_file_info_string);
    setProgressBarValue(screen_id,download_progress_control_id,download_progress_value);
    return true ;
};


enum customer_reply_flag getMsgFromActivateScreen(void)
{
    getScreenCMD();
    if(compareCMD(msg_rec_S32G_active,cmd_rec,9))
    {
        return customer_agree_flag; 
    }
    else if(compareCMD(msg_rec_S32G_later_active,cmd_rec,9))
    {
        return customer_later_agree_flag;
    }
    else if(compareCMD(msg_rec_S32G_inactive,cmd_rec,9))
    {
        return customer_disagree_flag;
    }
    else
    {
        return error_rec_message_flag;
    } 
};
bool setActivate_EScreen(void)
{
    while(1)
    {
        enum customer_reply_flag customer_reply=getMsgFromActivateScreen();
        if(customer_reply == customer_agree_flag)
        {
            printf("customer agree to activate firmware\n");
            return true;
        }
        else if(customer_reply == customer_disagree_flag)
        {
            printf("customer disagree to activate firmware\n");
            return false;
        }
        else if(customer_reply == customer_later_agree_flag)
        {
            printf("customer later agree to activate firmware\n");
            switchScreen(master_screen_id);
            usleep(duration_later);
            switchScreen(activate_e_screen_id);
        }
        else
        {
             printf("error receive message\n");
        }
    }
    return true;
};
bool setSuccessful_EScreen(void);
bool setFailed_EScreen(void);

void clearAllScreenLog(void)
{

    //Clear Fotademo_e_screen
	setLogText(fotademo_e_screen_id," ");
	setLogText(fotademo_e_screen_id," ");
	setLogText(fotademo_e_screen_id," ");

    //Clear Downloading1file_EScreen 
	setDownloading1file_EScreen(0," ");
	setLogText(downloading1file_screen_id," ");
	setLogText(downloading1file_screen_id," ");
	setLogText(downloading1file_screen_id," ");
	//Clear Verifying1file_e_screen 
	setVerifying1file_EScreen(0," ");
	setLogText(verifying1file_e_screen_id," ");
	setLogText(verifying1file_e_screen_id," ");
	setLogText(verifying1file_e_screen_id," ");
	
    //Clear Install1file_e_screen ;
	setInstalling1file_EScreen(0," ");
	setLogText(installing1file_e_screen_id," ");
	setLogText(installing1file_e_screen_id," ");
	setLogText(installing1file_e_screen_id," ");

    fotademo_e_queue.tail=0;
    request_e_queue.tail=0;
    downloading1file_e_queue.tail = 0;
    verifying1file_e_queue.tail = 0;
    verifydone_e_queue.tail = 0;
    installing1file_e_queue.tail = 0;
    acitvate_e_queue.tail = 0;
    successful_e_queue.tail = 0;
    failed_e_queue.tail = 0;
}
// void clearScreenLog(int screen_id)
// {
//     if(screen_id == downloading1file_screen_id || screen_id == verifying1file_e_screen_id
//       || screen_id == installing1file_e_screen_id)
//     {
//         setLogText(screen_id," ");
// 	    setLogText(screen_id," ");
// 	    setLogText(screen_id," ");
//     }
//     else
//     {
//         printf("error screen id to clean log\n");
//     }
    
// }







