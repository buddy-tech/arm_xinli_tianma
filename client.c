#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>
#include <errno.h>

#include "client.h"
#include "rwini.h"
#include "recvcmd.h"
#include "pgos/MsOS.h"
#include "packsocket/packsocket.h"
#include "json/cJSON.h"
#include "util/debug.h"
#include "xmlparse/xmlparser.h"
#include "loop/loop.h"
#include "comStruct.h"
#include "pgDB/pgLocalDB.h"
#include "threadpool/threadpool.h"
#include "fpgaFunc.h"

static int  heartBackTimerId;
static loop_list_t s_recv_file_list = { 0 };

//ARM程序版本
#define ARM_SW_ID1   0
#define ARM_SW_ID2   0
#define ARM_SW_ID3   2

#define ARM_SW_DESC	"Release: "
//HX8394, 
#define IC_SUPPORT_LIST	"OTM8019A, ILI9806E, ICN9706."


#define HARDWARE_MODEL	"ETS-1011"
#define HARDWARE_VER	"1.0.0"

#define RELEASEDATE     __DATE__
#define RELEASETIME     __TIME__

void show_version()
{
	char swTxt[256] = "";

	//Feb 10 2017:16:14:47
	char datetimeTxt[100];
	sprintf(datetimeTxt, "%s %s\r\n", __DATE__, __TIME__);
	strcat(swTxt, datetimeTxt);

	//arm:1.1.15
	char armTxt[100];
	sprintf(armTxt, "arm: %d.%d.%d\r\n", ARM_SW_ID1, ARM_SW_ID2, ARM_SW_ID3);
	strcat(swTxt, armTxt);

	printf("\nSoftwareVersion: \n");
	printf("%s \n", swTxt);
}

static time_t getDateFromMacro(char const *time) 
{
    char s_month[5];
    int month, day, year;
    struct tm t = {0};
    static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

    sscanf(time, "%s %d %d", s_month, &day, &year);

    month = (strstr(month_names, s_month)- month_names) / 3;

    t.tm_mon = month;
    t.tm_mday = day;
    t.tm_year = year - 1900;
    t.tm_isdst = -1;

    return mktime(&t);
}

static int get_data_time_from_macro(char* str_new_date) 
{
    char s_month[5];
    int month = 0;
	int day = 0;
	int year = 0;

	if (str_new_date == NULL)
		return -1;
	
    static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

    sscanf(__DATE__, "%s %d %d", s_month, &day, &year);

    month = (strstr(month_names, s_month)- month_names) / 3 + 1;

	sprintf(str_new_date, "%d/%d/%d %s", year, month, day, __TIME__);
	
    return 0;
}



void recv_file_list_init()
{
    loop_create(&s_recv_file_list, 10, 0);
}

void recv_file_list_insert(void *pData)
{
    loop_push_to_tail(&s_recv_file_list, pData);
}

static int searchRecvConn(const void *precvConn, const void *precv)
{
    void* src = NULL;
	void* dst = NULL;
	
    if(!precvConn || !precv)
    {
        return 0;
    }
	
    src = precvConn;
    dst = precv;
	
    if(src == dst)
    {
        return 1;
    }
	
    return 0;
}

void *recv_file_list_get(void *pData)
{
    void  *p = loop_search( &s_recv_file_list, pData, searchRecvConn );
    return p;
}

void recv_file_list_del(void *pData)
{
    void  *p = loop_search( &s_recv_file_list, pData, searchRecvConn );
    loop_remove(&s_recv_file_list,p);
}

static cliService_t cliservice = { 0 };
cliService_t* get_client_service_data()
{
	return &cliservice;
}

void client_register(char *pSoftVer,char *pHardVer,char *pgCode)
{
    int ipnum = 0;
	
    sync_info_t sync_info;
    read_sync_config(&sync_info);
	
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "syncStatus",sync_info.syncProcess);
    cJSON_AddNumberToObject(root, "syncTimeStamp",sync_info.timeStamp);
    cJSON_AddStringToObject(root, "pgCode",pgCode);
    cJSON_AddStringToObject(root, "hardVersion",pHardVer);
    cJSON_AddStringToObject(root, "softVersion",pSoftVer);
	
    char *out=cJSON_Print(root);
    cJSON_Delete(root);

	printf("client_register: send msg CLIENT_MSG_REGISTER.\n");
	if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd, CLIENT_MSG_REGISTER, ipnum, 0, 0, (unsigned char*)out, strlen(out));
	
    free(out);
}

void client_register_ex(unsigned int fpga_version, unsigned int box_version, unsigned int power_version, char *pgCode)
{
    int ipnum = 0;

	char str_arm_version[32] = "";	
	char str_build_data_time[32] = "";
	char str_fpga_version[32] = "";
	char str_power_version[32] = "";
	char str_box_version[32] = "";

	sprintf(str_arm_version, "%d.%d.%d", ARM_SW_ID1, ARM_SW_ID2, ARM_SW_ID3);
	get_data_time_from_macro(str_build_data_time);

	sprintf(str_fpga_version, "%d.%d.%d.%d", (fpga_version>>12)&0xf, (fpga_version>>8)&0xf, 
													(fpga_version>>4)&0xf, (fpga_version)&0xf);
	sprintf(str_box_version, "%d.%d.%d", (box_version>>16)&0xff, (box_version>>8)&0xff, (box_version)&0xff);
	sprintf(str_power_version, "%d.%d.%d", (power_version>>16)&0xff, (power_version>>8)&0xff, (power_version)&0xff);
	
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "arm_ver", str_arm_version);
    cJSON_AddStringToObject(root, "fpga_ver", str_fpga_version);
	cJSON_AddStringToObject(root, "box_ver", str_box_version);
    cJSON_AddStringToObject(root, "power_ver", str_power_version);

	cJSON_AddStringToObject(root, "build_time", str_build_data_time);
	cJSON_AddStringToObject(root, "soft_desc", 	ARM_SW_DESC);
	cJSON_AddStringToObject(root, "ic_list", IC_SUPPORT_LIST);

	cJSON_AddStringToObject(root, "hard_model", HARDWARE_MODEL);
	cJSON_AddStringToObject(root, "hard_ver", HARDWARE_VER);
	cJSON_AddStringToObject(root, "SN", "0000-0000-000000");
	
	
    char *out=cJSON_Print(root);
    cJSON_Delete(root);

	printf("client_register: send msg CLIENT_MSG_REGISTER_EX.\n");
	if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd, CLIENT_MSG_REGISTER_EX, ipnum, 0, 0, (unsigned char*)out, strlen(out));
	
    free(out);
}


static int find_file_from_local_db(char *pFileName,int u32FileSize,int u32FileTime)
{
    local_file_info_t *plocal_file_info = localDBGetRec(pFileName);
	
    //通过比较去判断是否更新文件
    if(plocal_file_info)
    {
        if( (plocal_file_info->fileTime == u32FileTime)
			&& (plocal_file_info->fileSize == u32FileSize) )
        {

            return 1;
        }
        else
        {
            printf("File: %s, time: %d-%d, size: %d-%d\n", pFileName, plocal_file_info->fileTime, u32FileTime, 
					u32FileSize, plocal_file_info->fileSize);
        }
    }
    else
    {
        printf("File %s is not exit.\n", pFileName);
    }
	
    return 0;
}

int client_getFileFromSet(void *infoSet)
{
    recv_file_info_t* recv_file_info = 0;
    recv_file_info_set_t *recv_file_info_set = infoSet;
	
    while(recv_file_info = recv_file_info_set->fn_get_cur_file_info(recv_file_info_set))
    {
        if(!find_file_from_local_db(recv_file_info->rfiName, recv_file_info->rfiSize, recv_file_info->rfiTime))
        {
            break;
        }
		
        recv_file_info_set->fn_add_file_index(recv_file_info_set);
    }
	
    if(recv_file_info)
    {
		printf("Req: get file %s.\n", recv_file_info->rfiName);
		client_getFile(recv_file_info->rfiName, infoSet);
        return 0;
    }
	
	return -1;
}

void client_getFile(char *pFileName,void *infoSet)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "fileName", pFileName);
    char *out=cJSON_Print(root);
    cJSON_Delete(root);
	
    if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd, CLIENT_MSG_SYNCFILE, infoSet, 0, 0, (unsigned char*)out, strlen(out));

	free(out);
}

void client_syncStatu()
{
    int ipnum = 0;
    sync_info_t sync_info;
    read_sync_config(&sync_info);
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "syncStatus",sync_info.syncProcess);
    cJSON_AddNumberToObject(root, "syncTimeStamp",sync_info.timeStamp);
    char *out=cJSON_Print(root);
    cJSON_Delete(root);

	printf("client_syncStatu: cliservice.send_fd = %d.\n", cliservice.send_fd);
    if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd,CLIENT_MSG_SYNCPROC,ipnum,0,0,(unsigned char*)out,strlen(out));

	free(out);
}

void client_syncFinish()
{
    int ipnum = 0;
	
    if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd,CLIENT_MSG_SYNCFIN,ipnum,0,0,0,0);
	
    write_cur_module_name();
    load_cur_module();
}

void client_sendEdid()
{
    int ipnum = 0;
    if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd,CLIENT_MSG_SENDEDID,ipnum,0,0,0,0);
}

void client_sendVcom(int channel,int vcom, int otp,int flick)
{
    int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "channel", channel);
	cJSON_AddNumberToObject(item, "vcom", vcom);
	cJSON_AddNumberToObject(item, "otp", otp);
	cJSON_AddNumberToObject(item, "flick", flick);

	printf("client_sendVcom: flick: %d.\n", flick);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);
    if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd,CLIENT_MSG_SENDVCOM,ipnum,0,0,(unsigned char*)out,strlen(out));

	free(out);
}

void client_sendOtp(int channel, int otp_result, int max_otp_times) //回应OTP已经结束, 2017-5-8
{
	socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "channel", channel);
	cJSON_AddNumberToObject(item, "otp_result", otp_result);
	cJSON_AddNumberToObject(item, "max_otp_times", max_otp_times);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);
	pSock_cmd_class->sendSocketCmd(cliservice.send_fd, CLIENT_MSG_OTP, ipnum, 0, 0,(unsigned char*)out,strlen(out));
	free(out);
}

void client_send_id_Otp_end(int channel, int result, int max_otp_times) 
{
	socket_cmd_class_t *pSock_cmd_class = &cliservice.socketCmdClass;
	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "channel", channel);
	cJSON_AddNumberToObject(item, "result", result);	
	cJSON_AddNumberToObject(item, "max_otp_times", max_otp_times);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);
	pSock_cmd_class->sendSocketCmd(cliservice.send_fd, CLIENT_MSG_WRITE_ID_OTP_INFO, ipnum, 0, 0,(unsigned char*)out,strlen(out));
	free(out);
}

void client_end_rgbw(int channel, int rgbw, double x, double y, double Lv)
{
	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "channel", channel);
	cJSON_AddNumberToObject(item, "rgbw", rgbw);
	cJSON_AddNumberToObject(item, "x", x);
	cJSON_AddNumberToObject(item, "y", y);
	cJSON_AddNumberToObject(item, "Lv", Lv);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);

	if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd,CLIENT_MSG_RGBW,ipnum,0,0,(unsigned char*)out,strlen(out));

	free(out);
}

void client_end_gamma_write(int channel, int write_type)
{
	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "channel", channel);
	cJSON_AddNumberToObject(item, "write_type", write_type);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);

	if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd,CLIENT_MSG_GAMMA,ipnum,0,0,(unsigned char*)out,strlen(out));

	free(out);
}

void client_end_read_gamma(int channel, int len, unsigned char* data, int otpp, int otpn, int read_type)
{
	char str_data[1000];
	memset(str_data, 0, sizeof(str_data));
	int i;
	for(i=0; i < len; i++)
	{
		char cc[20];
		sprintf(cc, "%02X;", data[i]);
		strcat(str_data, cc);
	}

	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "channel", channel);
	cJSON_AddStringToObject(item, "data", str_data);
	cJSON_AddNumberToObject(item, "otpp", otpp);
	cJSON_AddNumberToObject(item, "otpn", otpn);
	cJSON_AddNumberToObject(item, "read_type", read_type);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);
	
	if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd, CLIENT_MSG_READ_GAMMA, ipnum, 0, 0, 
									(unsigned char*)out, strlen(out));
	free(out);
}

void client_end_gamma_otp(int channel, char* msg)
{	
	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "channel", channel);
	cJSON_AddStringToObject(item, "msg", msg);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);

	if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd,CLIENT_MSG_GAMMA_OTP,ipnum,0,0,(unsigned char*)out,strlen(out));

	free(out);
}

void client_end_gamma_otp_info(int channel, int error_no, int vcom, int flick, 
									double f_x, double f_y, double f_lv)
{
	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "channel", channel);
	cJSON_AddNumberToObject(item, "error_no", error_no);
	cJSON_AddNumberToObject(item, "vcom", vcom);
	cJSON_AddNumberToObject(item, "flick", flick);
	cJSON_AddNumberToObject(item, "x", f_x);
	cJSON_AddNumberToObject(item, "y", f_y);
	cJSON_AddNumberToObject(item, "lv", f_lv);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);

	printf("client_end_gamma_otp_info: x: %f, y: %f, lv: %f.\n", f_x, f_y, f_lv);

	if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd, CLIENT_MSG_GAMMA_OTP, ipnum, 0, 0, 
									(unsigned char*)out, strlen(out));
	free(out);
}


void client_end_realtime_control(int channel, int len, unsigned char* data)
{
	char str_data[1000];
	memset(str_data, 0, sizeof(str_data));
	int i;
	for(i=0; i < len; i++)
	{
		char cc[20];
		sprintf(cc, "%02X;", data[i]);
		strcat(str_data, cc);
	}
	
	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "channel", channel);
	cJSON_AddStringToObject(item, "data", str_data);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);

	if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd,CLIENT_MSG_REALTIME_CONTROL,ipnum,0,0,(unsigned char*)out,strlen(out));
	
	free(out);
}

void client_end_firmware_upgrade(int upgrade_type, int upgrade_state)
{
	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "upgrade_type", upgrade_type);
	cJSON_AddNumberToObject(item, "upgrade_state", upgrade_state);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);

	if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd, CLIENT_MSG_UPGRADE_END, 
    											ipnum, 0, 0, (unsigned char*)out, strlen(out));
	free(out);
}

void client_sendPower(int channel, sByPtnPwrInfo *pwrInfo)
{
	int ipnum = 0;
    cJSON *item = cJSON_CreateObject();
    cJSON_AddNumberToObject(item, "Channel",channel);

    cJSON_AddNumberToObject(item, "VDD",  pwrInfo->VDD);
    cJSON_AddNumberToObject(item, "VDDh", pwrInfo->VDDh);
    cJSON_AddNumberToObject(item, "VDDl", pwrInfo->VDDl);
    cJSON_AddNumberToObject(item, "iVDD", pwrInfo->iVDD);
    cJSON_AddNumberToObject(item, "iVDDh",pwrInfo->iVDDh);
    cJSON_AddNumberToObject(item, "iVDDl",pwrInfo->iVDDl);

    cJSON_AddNumberToObject(item, "VDDIO", pwrInfo->VDDIO);
    cJSON_AddNumberToObject(item, "VDDIOh",pwrInfo->VDDIOh);
    cJSON_AddNumberToObject(item, "VDDIOl",pwrInfo->VDDIOl);
    cJSON_AddNumberToObject(item, "iVDDIO",pwrInfo->iVDDIO);
    cJSON_AddNumberToObject(item, "iVDDIOh",pwrInfo->iVDDIOh);
    cJSON_AddNumberToObject(item, "iVDDIOl",pwrInfo->iVDDIOl);

    cJSON_AddNumberToObject(item, "ELVDD", pwrInfo->ELVDD);
    cJSON_AddNumberToObject(item, "ELVDDh",pwrInfo->ELVDDh);
    cJSON_AddNumberToObject(item, "ELVDDl",pwrInfo->ELVDDl);
    cJSON_AddNumberToObject(item, "iELVDD",pwrInfo->iELVDD);
    cJSON_AddNumberToObject(item, "iELVDDh",pwrInfo->iELVDDh);
    cJSON_AddNumberToObject(item, "iELVDDl",pwrInfo->iELVDDl);

    cJSON_AddNumberToObject(item, "ELVSS", pwrInfo->ELVSS);
    cJSON_AddNumberToObject(item, "ELVSSh",pwrInfo->ELVSSh);
    cJSON_AddNumberToObject(item, "ELVSSl",pwrInfo->ELVSSl);
    cJSON_AddNumberToObject(item, "iELVSS",pwrInfo->iELVSS);
    cJSON_AddNumberToObject(item, "iELVSSh",pwrInfo->iELVSSh);
    cJSON_AddNumberToObject(item, "iELVSSl",pwrInfo->iELVSSl);

    cJSON_AddNumberToObject(item, "VBL", pwrInfo->VBL);
	cJSON_AddNumberToObject(item, "iVBL",pwrInfo->iVBL);

    cJSON_AddNumberToObject(item, "VBLh", 0);
    cJSON_AddNumberToObject(item, "VBLl", 0);
    cJSON_AddNumberToObject(item, "iVBLh", 0);
    cJSON_AddNumberToObject(item, "iVBLl", 0);

    cJSON_AddNumberToObject(item, "VSP", pwrInfo->VSP);
    cJSON_AddNumberToObject(item, "VSPh",pwrInfo->VSPh);
    cJSON_AddNumberToObject(item, "VSPl",pwrInfo->VSPl);
    cJSON_AddNumberToObject(item, "iVSP",pwrInfo->iVSP);
    cJSON_AddNumberToObject(item, "iVSPh",pwrInfo->iVSPh);
    cJSON_AddNumberToObject(item, "iVSPl",pwrInfo->iVSPl);

    cJSON_AddNumberToObject(item, "VSN", pwrInfo->VSN);
    cJSON_AddNumberToObject(item, "VSNh",pwrInfo->VSNh);
    cJSON_AddNumberToObject(item, "VSNl",pwrInfo->VSNl);
    cJSON_AddNumberToObject(item, "iVSN",pwrInfo->iVSN);
    cJSON_AddNumberToObject(item, "iVSNh",pwrInfo->iVSNh);
    cJSON_AddNumberToObject(item, "iVSNl",pwrInfo->iVSNl);

    cJSON_AddNumberToObject(item, "MTP", pwrInfo->MTP);
    cJSON_AddNumberToObject(item, "MTPh",pwrInfo->MTPh);
    cJSON_AddNumberToObject(item, "MTPl",pwrInfo->MTPl);
    cJSON_AddNumberToObject(item, "iMTP",pwrInfo->iMTP);
    cJSON_AddNumberToObject(item, "iMTPh",pwrInfo->iMTPh);
    cJSON_AddNumberToObject(item, "iMTPl",pwrInfo->iMTPl);
    char *out=cJSON_Print(item);
    cJSON_Delete(item);
	
    if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd, CLIENT_MSG_SENDPOW, ipnum, 
    												0, 0, (unsigned char*)out, strlen(out));
    free(out);
}

void client_sendReg(int regAddr,int regValue)
{
    int ipnum = 0;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "regAddr",regAddr);
    cJSON_AddNumberToObject(root, "regValue",regValue);
    char *out=cJSON_Print(root);
    cJSON_Delete(root);
	
    if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd,CLIENT_MSG_SENDREG,ipnum,0,0,(unsigned char*)out,strlen(out));
	
    free(out);
}

void client_sendRegStatus(int regAddr,int regStatu)
{
    int ipnum = 0;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "regAddr",regAddr);
    cJSON_AddNumberToObject(root, "regStatu",regStatu);
    char *out=cJSON_Print(root);
    cJSON_Delete(root);
	
    if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd,CLIENT_MSG_REGSTAT,ipnum,0,0,(unsigned char*)out,strlen(out));
	
    free(out);
}

void client_updateStatus()
{
    int ipnum = 0;
    if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd,CLIENT_MSG_UPDSTAT,ipnum,0,0,0,0);
}

void client_rebackHeart()
{
    int ipnum = 0;

	if (cliservice.send_fd != -1)
	{
		if (cliservice.socketCmdClass.sendSocketCmd)
		{
			int ret = cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd, CLIENT_MSG_REBACK, ipnum, 0, 0, 0, 0);
			if (ret != 0)
			{
				shutdown(cliservice.send_fd, SHUT_RDWR);
				cliservice.send_fd = -1;
			}
		}
	}
    //DBG("reback the heart %d",time(NULL));
}

void client_rebackPower(int powerOn)
{
    int ipnum = 0;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "whatInfo",1); //power = 1
    cJSON_AddNumberToObject(root, "powerOn",powerOn);
    char *out=cJSON_Print(root);
    cJSON_Delete(root);
	
    if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd,(REBACK_POWER_STATUS<<16)|CLIENT_MSG_REBACK,ipnum,0,0,(unsigned char*)out,strlen(out));
	
    free(out);
}

void client_notify_msg(int msg_flag, char* p_msg)
{
    int ipnum = 0;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "msg_flag", msg_flag);

	if (p_msg)
    	cJSON_AddStringToObject(root, "msg", p_msg);
	else
		cJSON_AddStringToObject(root, "msg", "OK!");
	
    char *out=cJSON_Print(root);
    cJSON_Delete(root);
	
    if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd, CLIENT_MSG_NOTIFY, 
    										ipnum, 0, 0,(unsigned char*)out, strlen(out));
	
    free(out);
}

void client_rebackError(int errorNo)
{
    int ipnum = 0;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "errorNo", errorNo); //power = 1
    char *out=cJSON_Print(root);
    cJSON_Delete(root);

	if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd,(REBACK_ERROR_INFO<<16)|CLIENT_MSG_REBACK,ipnum,0,0,(unsigned char*)out,strlen(out));
	
    free(out);
}

void client_rebackMipiCode(char *pMipiCode,int len)
{
    int ipnum = 0;

	if (!cliservice.socketCmdClass.getPackageMaxLen)
		return ;

    if(len>0)
    {
        int i,lastNo;
        int packetLen = cliservice.socketCmdClass.getPackageMaxLen();
        int validDataLen  = len;
        int remainDataLen = len;
        lastNo = validDataLen/packetLen;
        for(i=0;i<lastNo+1;i++)
        {
            int sendLen = 0;
            if(remainDataLen>=packetLen)
            {
                sendLen = packetLen;
            }
            else
            {
                sendLen = remainDataLen;
            }

			if (cliservice.socketCmdClass.sendSocketCmd)
    			cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd,(REBACK_MIPI_CODE<<16)|CLIENT_MSG_REBACK,ipnum,i,lastNo,(unsigned char*)&pMipiCode[i*packetLen],sendLen);
			
            remainDataLen -= sendLen;
        }
    }
}

void client_rebackFlick(char flick,char polar)
{
    int ipnum = 0;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "curflick",flick);
    cJSON_AddNumberToObject(root, "curpolar",polar);
    char *out=cJSON_Print(root);
    cJSON_Delete(root);	

	if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd,(REBACK_FLICK_INFO<<16)|CLIENT_MSG_REBACK,ipnum,0,0,(unsigned char*)out,strlen(out));
	
    free(out);
}

void client_rebackautoFlick(int channel,int index,int autoflick)
{
    int ipnum = 0;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "index",index);
    cJSON_AddNumberToObject(root, "autoflick",autoflick);
	cJSON_AddNumberToObject(root, "Channel",channel);
    char *out=cJSON_Print(root);
    cJSON_Delete(root);
	
    if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd,(REBACK_AUTO_FLICK<<16)|CLIENT_MSG_REBACK,ipnum,0,0,(unsigned char*)out,strlen(out));
	
    free(out);
}

void client_rebackFlickOver(int channel, int vcom, int flick)
{
	printf("client_rebackFlickOver: vcom: %d, flick: %d.\n");
	
	int ipnum = 0;
	cJSON *root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "Channel", channel);
	cJSON_AddNumberToObject(root, "vcom", vcom);
	cJSON_AddNumberToObject(root, "flicker", flick);
	char *out = cJSON_Print(root);
	cJSON_Delete(root);

	if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd,(REBACK_FLICK_OVER<<16)|CLIENT_MSG_REBACK,ipnum,0,0,(unsigned char*)out,strlen(out));
	
	free(out);
}

void client_sendPowerVersion(unsigned int channel, unsigned int version)
{
	int ipnum = 0;
	cJSON *root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "channel", channel);
	cJSON_AddNumberToObject(root, "version", version);
	char *out = cJSON_Print(root);
	cJSON_Delete(root);

	if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd,CLIENT_MSG_POWER_VER,ipnum,0,0,(unsigned char*)out,strlen(out));
	
	free(out);
}

void client_sendBoxVersion(unsigned int channel, unsigned int version)
{
	int ipnum = 0;
	cJSON *root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "channel", channel);
	cJSON_AddNumberToObject(root, "version", version);
	char *out = cJSON_Print(root);
	cJSON_Delete(root);
	
	if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd,CLIENT_MSG_BOX_VER,ipnum,0,0,(unsigned char*)out,strlen(out));
	
	free(out);
}

void client_lcd_read_reg_ack(int channel, int len, unsigned char* data)
{
	char str_data[1024 * 2];
	memset(str_data, 0, sizeof(str_data));
	
	int i;
	for(i=0; i < len; i++)
	{
		char cc[20];
		
		if (i != 0 && i % 16 == 0)
		{
			sprintf(cc, "\r\n");
			strcat(str_data, cc);
		}		
		
		sprintf(cc, "%02X ", data[i]);
		strcat(str_data, cc);

		
	}

	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "channel", channel);
	cJSON_AddStringToObject(item, "read_data", str_data);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);
	
	if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd, CLIENT_MSG_LCD_READ_REG, ipnum, 0, 0, 
									(unsigned char*)out, strlen(out));
	
	free(out);
}

void client_flick_test_ack(int channel, int vcom, double flick)
{
	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "mipi_channel", channel);
	cJSON_AddNumberToObject(item, "vcom", vcom);
	cJSON_AddNumberToObject(item, "flick", flick);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);
	
	if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd, CLIENT_MSG_FLICK_TEST, ipnum, 0, 0, 
									(unsigned char*)out, strlen(out));
	free(out);
}

// Open Short
void client_power_test_read_open_short_mode_ack(unsigned char mode)
{
	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "mode", mode);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);
	
	if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd, CLIENT_MSG_READ_OPEN_SHORT_MODE, 
    											ipnum, 0, 0, (unsigned char*)out, strlen(out));
	free(out);
}

// VCOM Ack
void client_power_test_read_vcom_volt_ack(short vcom_volt)
{
	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "volt_vcom", vcom_volt);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);
	
	if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd, CLIENT_MSG_POWER_TEST_READ_VCOM_VOLT, 
    											ipnum, 0, 0, (unsigned char*)out, strlen(out));
	free(out);
}

void client_power_test_read_vcom_status_ack(short vcom_status)
{
	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "vcom_enable", vcom_status);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);
	
	if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd, CLIENT_MSG_POWER_TEST_READ_VCOM_VOLT_STATUS, 
    											ipnum, 0, 0, (unsigned char*)out, strlen(out));
	free(out);
}

// OTP Ack
void client_power_test_read_otp_volt_ack(unsigned short otp_volt)
{
	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "volt_otp", otp_volt);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);
	
	if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd, CLIENT_MSG_POWER_TEST_READ_OTP_VOLT, 
    											ipnum, 0, 0, (unsigned char*)out, strlen(out));
	free(out);
}

void client_power_test_read_otp_status_ack(short otp_status)
{
	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "otp_enable", otp_status);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);
	
	if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd, CLIENT_MSG_POWER_TEST_READ_OTP_VOLT_STATUS, 
    											ipnum, 0, 0, (unsigned char*)out, strlen(out));
	free(out);
}

// read open short data
void client_power_test_read_open_short_ack(unsigned short open_short)
{
	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "open_short", open_short);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);
	
	if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd, CLIENT_MSG_POWER_TEST_READ_OPEN_SHORT_DATA, 
    											ipnum, 0, 0, (unsigned char*)out, strlen(out));
	free(out);
}

// read ntc_ag data
void client_power_test_read_ntc_ag_ack(unsigned short ntc_ag)
{
	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "ntc_ag", ntc_ag);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);
	
	if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd, CLIENT_MSG_POWER_TEST_READ_NTC_AG_DATA, 
    											ipnum, 0, 0, (unsigned char*)out, strlen(out));
	free(out);
}

void client_gpio_get_state_ack(unsigned char index, unsigned char pin, unsigned char dir, unsigned char value)
{
	int ipnum = 0;
	cJSON *item = cJSON_CreateObject();
	cJSON_AddNumberToObject(item, "index", index);
	cJSON_AddNumberToObject(item, "pin", pin);
	cJSON_AddNumberToObject(item, "dir", dir);
	cJSON_AddNumberToObject(item, "value", value);
	char *out = cJSON_Print(item);
	cJSON_Delete(item);
	
	if (cliservice.socketCmdClass.sendSocketCmd)
    	cliservice.socketCmdClass.sendSocketCmd(cliservice.send_fd, CLIENT_MSG_GPIO_GET_STATE, 
    											ipnum, 0, 0, (unsigned char*)out, strlen(out));
	free(out);
}

// ==============================================================================================
static int fill_host_addr(struct sockaddr_in *host, char * host_ip_addr, int port)
{
    if( port <= 0 || port > 65535)
	{
		printf("fill_host_addr error: Invalid port = %d.\n", port);
		return -1;
	}
	
    memset(host, 0, sizeof(struct sockaddr_in));
    host->sin_family = AF_INET;
	
    if(inet_addr(host_ip_addr) != -1)
    {
        host->sin_addr.s_addr = inet_addr(host_ip_addr);
    }
    else
    {
        struct hostent * server_hostent = NULL;
        if( (server_hostent = gethostbyname(host_ip_addr) ) != 0)
        {
            memcpy(&host->sin_addr, server_hostent->h_addr, sizeof(host->sin_addr));
        }
        else
        {
        	printf("fill_host_addr error: gethostbyname error! ip = %s.\n", host_ip_addr);
            return -2;
        }
    }
	
    host->sin_port = htons(port);
    return 1;
}

static int xconnect(struct sockaddr_in* saddr, int type)
{
    int set =0;
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if(s < 0)
    {
    	printf("xconnect error: create socket failed!\n");
        return -1;
    }

    //connect to the server
    if (connect(s, (struct sockaddr *)saddr, sizeof(struct sockaddr_in)) < 0)
    {
		close(s);
        printf("xconnect error: Can't connect to server %s\n", inet_ntoa(saddr->sin_addr));
        return -1;
    }
	
    return s;
}

static void* server_start(void *data);

static unsigned int s_server_ipaddr = 0;

unsigned int get_server_ip()
{
	return s_server_ipaddr;
}

void set_server_ip(unsigned int new_ip)
{
	s_server_ipaddr = new_ip;
}

void close_all_socket()
{
	printf("close_all_socket ...\n");

	if(cliservice.send_fd != -1)
	{
		printf("close send socket!\n");
		//close(cliservice.send_fd);
		shutdown(cliservice.send_fd, SHUT_RDWR);
		cliservice.send_fd = -1;
	}
	
	if(cliservice.client_socket != -1)
	{
		printf("close client socket!\n");
		//close(cliservice.client_socket);
		shutdown(cliservice.client_socket, SHUT_RDWR);
		cliservice.client_socket = -1;
	}
}

static void * server_guard(void* data)
{
    cliService_t *pcliSrv = (struct cliService_t*)data;
    while(pcliSrv->guard_working)
    {
		pthread_mutex_lock(&pcliSrv->m);
		
		pcliSrv->time_alive_timeout ++;
		
		if(pcliSrv->time_alive_timeout >= 5)
		{
			
			if (pcliSrv->link_on)
			{
				set_server_ip(0);

				// close all connect.
				close_all_socket();
				
				pcliSrv->link_on = 0;

				printf("server_guard: net link is break, close all socket!\n");
			}
		}
		
		pthread_mutex_unlock(&pcliSrv->m);
		
        sleep(1);
    }
	
    DBG("left the thread:server_guard\n");
	pthread_exit(0);
    return NULL;
}

static void* client_connect(void *data)
{
	cliService_t *pcliSrv = (cliService_t*)data;
	
	while(pcliSrv->srv_working)
	{
		//PG上位机IP地址
		while(pcliSrv->srv_working)
		{
			if(get_server_ip() > 0) 
				break;
			
			usleep(1000 * 200);
		}

		struct sockaddr_in pgServer;
		fill_host_addr(&pgServer, cliservice.server_ip, cliservice.server_port);
		cliservice.send_fd = xconnect(&pgServer, 1);
		if(cliservice.send_fd == -1)
		{
			printf("client_connect: xconnect failed!\n");
			
			cliservice.send_fd = -1;
			usleep(1000 * 200);
			continue;
		}

		printf("client_connect: connect to server[%s:%d] ok ...\n", cliservice.server_ip, cliservice.server_port);

		// send client register message.
		{
			//软件版本 ////////////////////////////////////////////////////////////////////////////////
			char swTxt[256] = "";

			char descTxt[100] = "";
			sprintf(descTxt, "%s\r\n", ARM_SW_DESC);
			strcat(swTxt, descTxt);

			//Feb 10 2017:16:14:47
			char datetimeTxt[100] = "";
			sprintf(datetimeTxt, "%s %s\r\n", __DATE__, __TIME__);
			strcat(swTxt, datetimeTxt);

			//arm:1.1.15
			char armTxt[100] = "";
			sprintf(armTxt, "arm: %d.%d.%d\r\n", ARM_SW_ID1, ARM_SW_ID2, ARM_SW_ID3);
			strcat(swTxt, armTxt);

			//fpga:1.0.0.3
			char fpgaTxt[100] = "";
			fpgaVersionInfo_t fpgaVersion = fpgaGetVersion();
			sprintf(fpgaTxt, "fpga: %d.%d.%d.%d\r\n", (fpgaVersion.version[1] >> 4)&0xf, (fpgaVersion.version[1])&0xf,
														(fpgaVersion.version[0] >> 4)&0xf, fpgaVersion.version[0]&0xf);
			strcat(swTxt, fpgaTxt);

			//pwr:01.01.05
			unsigned int pwrSoftwareVersion;
			if(pwrSoftwareVersion > 0)
			{
				unsigned int ver = pwrSoftwareVersion;
				char pwrTxt[100];
				sprintf(pwrTxt, "pwr: %d.%d.%d\r\n", (ver>>16)&0xff, (ver>>8)&0xff, (ver)&0xff);
				strcat(swTxt, pwrTxt);
			}

			//box:1.0.5
			unsigned int box_version = 0;
			box_version = box_get_version();
			
			if(box_version > 0)
			{
				char boxTxt[100];
				sprintf(boxTxt, "box: %d.%d.%d\r\n", (box_version>>16)&0xff, (box_version>>8)&0xff, (box_version)&0xff);
				strcat(swTxt, boxTxt);
			}

			// hardware version:
			char hwTxt[256];
			sprintf(hwTxt, "%s %s\r\n" HARDWARE_MODEL ": %d.%d.%d\r\n", __DATE__, __TIME__, 0, 0, 1);

			client_register(swTxt, hwTxt, HARDWARE_MODEL);
			client_register_ex(fpgaVersion.version[0] << 8 | fpgaVersion.version[1], pwrSoftwareVersion, box_version, HARDWARE_MODEL);
			printf("Version info:\n%s.\n", swTxt);
		}

		//检测读是否正常
		char buf[1024] = "";
		
		while(pcliSrv->srv_working)
		{
			int ret = recv(cliservice.send_fd, buf, sizeof(buf), 0);
			if(ret <= 0)
			{
				printf("client_connect error: recv error.\n");
				break;
			}
			
		}
		
		printf("client_connect: server is shutdown!\n");
		if(cliservice.send_fd != -1)
		{
			close(cliservice.send_fd);
			cliservice.send_fd = -1;
			//printf("client_connect: task end, close send_fd.\n");
		}
		
		set_server_ip(0);
	}
	
	pthread_exit(0);
	return NULL;
}

// route add -net 224.0.0.0 netmask 255.255.255.0 eth0
// route del -net 224.0.0.0 netmask 255.255.255.255 eth0
static void* client_mutilcast(void *data)
{
    cliService_t *pcliSrv = (cliService_t*)data;
    struct sockaddr_in local_addr;
    int err = -1;
	
	while(pcliSrv->mutil_working)
	{
		pcliSrv->muticast_fd = socket(AF_INET, SOCK_DGRAM, 0);
		if (pcliSrv->muticast_fd == -1)
		{
			printf("client_mutilcast error: create socket failed!\n");

			pcliSrv->muticast_fd = 0;
			usleep(500*1000);
			continue;
		}
		
		setsockopt(pcliSrv->muticast_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&err, sizeof(err));

		memset(&local_addr, 0, sizeof(local_addr));
		local_addr.sin_family = AF_INET;
		local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		local_addr.sin_port = htons(pcliSrv->multicast_port);
		printf("== Multicast Port: %d ==\n", pcliSrv->multicast_port);
		
		err = 1;
		err = bind(pcliSrv->muticast_fd,(struct sockaddr*)&local_addr, sizeof(local_addr)) ;
		if(err < 0)
		{
			printf("client_mutilcast error: bind socket failed!\n");
			int fd = pcliSrv->muticast_fd;
			pcliSrv->muticast_fd = 0;
			close(fd);
			usleep(500*1000);
			continue;
		}
		
		int loop = 1;
		err = setsockopt(pcliSrv->muticast_fd, IPPROTO_IP, IP_MULTICAST_LOOP,&loop, sizeof(loop));
		if(err < 0)
		{
			printf("client_mutilcast error: set socketopt(IP_MULTICAST_LOOP) failed!\n");
			int fd = pcliSrv->muticast_fd;
			pcliSrv->muticast_fd = 0;
			close(fd);
			usleep(500*1000);
			continue;
		}

		struct ip_mreq mreq;
		mreq.imr_multiaddr.s_addr = inet_addr(MCAST_ADDR);
		mreq.imr_interface.s_addr = htonl(INADDR_ANY);
		err = setsockopt(pcliSrv->muticast_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,&mreq, sizeof(mreq));
		if (err < 0)
		{
			system("route add -net 224.0.0.0 netmask 255.255.255.0 eth0");
			printf("client_mutilcast error: set socketopt(IP_ADD_MEMBERSHIP) failed! errno = %d.\n", errno);
			int fd = pcliSrv->muticast_fd;
			pcliSrv->muticast_fd = 0;
			close(fd);
			usleep(500*1000);
			continue;
		}

		//
		int addr_len = 0;
#define BUFF_SIZE 256
		char buff[BUFF_SIZE];
		int n = 0;
		
		while(pcliSrv->mutil_working)
		{
			//printf("===============================  Ready recv multicast data ========\n");
			addr_len = sizeof(local_addr);
			memset(buff, 0, BUFF_SIZE);
			n = recvfrom(pcliSrv->muticast_fd, buff, BUFF_SIZE, 0, (struct sockaddr*)&local_addr,&addr_len);
			if( n <= 0)
			{
				printf("client_mutilcast error: recvfrom failed!\n");
				break;
			}

			// Just for debug.
			//printf("recv mcast data: from %s.\n", inet_ntoa(local_addr.sin_addr));

			pthread_mutex_lock(&pcliSrv->m);
			
			strcpy(pcliSrv->server_ip, inet_ntoa(local_addr.sin_addr));
			set_server_ip(local_addr.sin_addr.s_addr);

			if (pcliSrv->link_on == 0)
			{
				printf("client_mutilcast: link is on!\n");
			}
			
			pcliSrv->link_on = 1;
			pcliSrv->time_alive_timeout = 0;
			
			pthread_mutex_unlock(&pcliSrv->m);
			//printf("client_multicast: recv msg, update server ip: %s. %d\n", pcliSrv->server_ip, get_server_ip() );
		}
		
		if(pcliSrv->muticast_fd > 0)
		{
			int fd = pcliSrv->muticast_fd;
			pcliSrv->muticast_fd = 0;
			err = setsockopt(fd, IPPROTO_IP, IP_DROP_MEMBERSHIP,&mreq, sizeof(mreq));
			close(fd);
		}
		
		printf("client_mutilcast: current loop end\n");
	}
	
	pthread_exit(0);
	return NULL;
}

static void* server_start(void *data)
{
    cliService_t *pcliSrv = (cliService_t*)data;
    int ret,len;
	
#if thread_pool
    struct threadpool *pool = threadpool_init(10, 20);
#endif

	unsigned char* arg = (unsigned char*)malloc(FIFO_MAX_LEN/2);

	while(pcliSrv->srv_working)
	{
		pcliSrv->listen_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
		if(pcliSrv->listen_socket == -1)
		{
			printf("server_start error: create socket failed!\n");
			usleep(500*1000);
			continue; 
		}

		/* Socket Reuse */
		ret = 1;
		setsockopt(pcliSrv->listen_socket, SOL_SOCKET, SO_REUSEADDR, (void*)&ret, sizeof(ret));

		/* Bind Socket */
		struct sockaddr_in addr;
		memset(&addr,0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = INADDR_ANY;
		addr.sin_port = htons( pcliSrv->local_port );
		ret = bind( pcliSrv->listen_socket, (struct sockaddr*)&addr, sizeof( addr ) );
		if(ret < 0)
		{
			printf("server_start error: bind socket failed!\n");
			close(pcliSrv->listen_socket);
			pcliSrv->listen_socket = -1;
			usleep(500*1000);
			continue;
		}

		//try to listen
		if( listen( pcliSrv->listen_socket, 1 ) <0 )
		{
			printf("server_start error: listen failed!\n");
			close(pcliSrv->listen_socket);
			pcliSrv->listen_socket = -1;
			usleep(500*1000);
			continue;
		}
		
		printf("server_start: listening on %d ...\n", pcliSrv->local_port );

		while(pcliSrv->srv_working)
		{
			len = sizeof(addr);
			pcliSrv->client_socket = accept(pcliSrv->listen_socket, (struct sockaddr *)&addr, &len);
			if(pcliSrv->client_socket < -1)
			{
				printf("server_start error: accept failed!\n");
				break;
			}

			printf("server_start: New Client: %s:%d.\n", inet_ntoa(addr.sin_addr), addr.sin_port);

			socket_cmd_class_t *pSock_cmd_class = &pcliSrv->socketCmdClass;
			socket_cmd_t *pSocketCmd = 0;

			while(pcliSrv->srv_working)
			{
				ret = recv(pcliSrv->client_socket, arg, FIFO_MAX_LEN/2, 0);
				if(ret <= 0)
				{
					printf("server_start: server_start->recv Error: %d.\n", ret);
					break;
				}
				else
				{
					pSock_cmd_class->recvSocketCmd(pSock_cmd_class, arg, ret);
					
					while(pcliSrv->srv_working)
					{
						//printf("pSock_cmd_class->parseSocketCmd: 1591. recv len = %d.\n", ret);
						pSocketCmd = pSock_cmd_class->parseSocketCmd(pSock_cmd_class);
						if(pSocketCmd)
						{
#if thread_pool
							if((pSocketCmd->cmd == SERVER2CLI_MSG_SHUTON)
								|| (pSocketCmd->cmd == SERVER2CLI_MSG_SYNCTIME))
							{
								threadpool_add_job(pool, msgProc, pSocketCmd);
							}
							else
							{
								ret = msgProc(pSocketCmd);
							}
#else
							ret = msgProc(pSocketCmd);
#endif
						}
						else
						{
							break;
						}
					}

				}
			}

			printf("server_start: Client is disconnected!\n");
			if(pcliSrv->client_socket != -1)
			{
				close(pcliSrv->client_socket);
				pcliSrv->client_socket = -1;
			}

		}

		if(pcliSrv->listen_socket != -1)
		{
			close(pcliSrv->listen_socket);
			pcliSrv->listen_socket = -1;
			printf("server_start: close org_server_fd socket.\n");
		}
	}
	
#if thread_pool
	threadpool_destroy(pool);
#endif

	free(arg);

	printf("server_start: thread end!\n");
	pthread_exit(0);
    return NULL;
}

static void load_config(cliService_t* cli)
{
    struct XML_DATA* xml = xml_load("config.xml");

    if(!xml)
	{
		printf("load_config error: xml_load failed!\n");
    	return;
	}
	
    int terminal_log = xml_readnum(xml, "client_terminal_log");
    int file_log = xml_readnum(xml, "client_file_log");
	
    debug_set_dir( xml_readstr(xml, "client_file_log:directory") );
    if( terminal_log )
        debug_term_on();
    else
        debug_term_off();
    if( file_log )
        debug_file_on();
    else
        debug_file_off();
    xml_free(xml);
}

static void heartBackTimerCallback(MS_U32 u32Timer)
{
    client_rebackHeart();
}

void client_init(int multicast_port)
{
    int result;
	
    memset(&cliservice,0,sizeof(cliservice));
    load_config(&cliservice);
    cliservice.server_port = SERVER_CLIENT_PORT; //send to server;
    cliservice.local_port  = CLIENT_SERVER_PORT; //local monitor;
    cliservice.srv_timeout = 3;
	
    result = pthread_mutex_init(&cliservice.m, 0);

    init_socket_cmd_class(&cliservice.socketCmdClass, PACKAGE_MAX_LEN,FIFO_MAX_LEN);
	
    cliservice.mutil_working = 1;
	cliservice.multicast_port = multicast_port;

	cliservice.guard_working = 1;
    result  = pthread_create(&cliservice.thread_guard, 0, (void*)server_guard, (void*)&cliservice );
	
    result |= pthread_create(&cliservice.thread_muticast, 0, (void*)client_mutilcast, (void*)&cliservice );

	cliservice.srv_working = 1;
	result |= pthread_create(&cliservice.thread_server, 0, (void*)server_start, (void*)&cliservice );
	result |= pthread_create(&cliservice.thread_connect, 0, (void*)client_connect, (void*)&cliservice );

    recv_file_list_init();

	int enable_timer = 1;
    heartBackTimerId = MsOS_CreateTimer(heartBackTimerCallback, 0, 1000, enable_timer, "heartBackTimer");
}

void client_destory()
{
    cliservice.mutil_working = 0;
    cliservice.guard_working = 0;
    cliservice.srv_working = 0;
	
	//把关闭套接字放在前面来
	if (cliservice.client_socket != -1)
		close(cliservice.client_socket);
	
	if (cliservice.muticast_fd != -1)
		close(cliservice.muticast_fd);

	if (cliservice.listen_socket != 1)
		close(cliservice.listen_socket);

	if (cliservice.send_fd != -1)
		close(cliservice.send_fd);
	
    pthread_join(cliservice.thread_guard, NULL);
    pthread_join(cliservice.thread_muticast, NULL);
    pthread_join(cliservice.thread_server, NULL);

	//原来关闭套接字在这个地方
	MsOS_StopTimer(heartBackTimerId);
	MsOS_DeleteTimer(heartBackTimerId);
	
	free(cliservice.socketCmdClass.cmd_fifo.cmd);
}


