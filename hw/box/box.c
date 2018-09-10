#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <semaphore.h>
#include "pgos/MsOS.h"
#include "common.h"
#include "comStruct.h"
#include "box.h"
#include "comUart.h"
#include "pgDB/pgDB.h"
#include "vcom.h"
#include "common.h"
#include "icm/ic_manager.h"
#include "clipg.h"
#include "rwini.h"
#include "client.h"
#include "debug.h"

#define WHILE_TEST           0
#define ENABLE_MTP_ON        1
#define ENABLE_GPIO_RESET    1
#define ENABLE_SWRESET       0
#define ENABLE_POWER_ON_OFF  0
unsigned int test_count = 0;

extern int m_channel_1; 
extern int m_channel_2;
extern int m_channel_3;
extern int m_channel_4;

static pthread_mutex_t gBoxReadLock;
static pthread_mutex_t gBoxGuardLock;
static volatile int gBoxKeyReturn = 0;
static MS_APARTMENT *boxApartMent = 0;
static time_t boxtime_alive = 0;

static void boxRcvThread();
static void boxHandThread();

volatile int gBoxUpdateState = 0;
volatile int gPowerUpdateState = 0;

static int box_time_out_enable = 1;

void set_box_time_out_enable()
{
	box_time_out_enable = 1;
}

void set_box_time_out_disable()
{
	box_time_out_enable = 0;
}

static int s_box_power_on_status = 0;

void set_box_power_on_status(int error)
{
	s_box_power_on_status = error;
}

int get_box_power_on_status()
{
	return s_box_power_on_status;
}

unsigned int        boxSoftwareVersion = 0;
static volatile int boxSoftwareVersion_request = 1;
static sem_t        boxSoftwareVersion_sem;
static pthread_t    boxSoftwareVersion_pthread;

int box_gen_pass_failed_msg_data(char* p_msg, int is_pass, char* p_buf, int buf_len)
{
	if( (p_msg == NULL) || (p_buf == NULL) || (buf_len <= 0) )
	{
		printf("box_gen_pass_failed_msg_data: invalid param.\n");
		return -1;
	}

	int msg_len = strlen(p_msg);
	if (msg_len > buf_len)
		msg_len = buf_len -2;

	memset(p_buf, 0, buf_len);
	
	if (is_pass)
		p_buf[0] = 0x02; // Green
	else 
		p_buf[0] = 0x01; // Red.

	memcpy(p_buf + 1, p_msg, msg_len);
	
	return 0;
}

void box_notify_power_on_ready()
{
	const char reqStr[] = {
		0xbf ,0xaa ,0xb5 ,0xe7 ,
		0xa3 ,0xac ,0xc7 ,0xeb ,
		0xb5 ,0xc8 ,0xb4 ,0xfd ,
		0x2e ,0x2e ,0x2e ,0x00
	}; //开电，请等待...！

	boxFeedback(reqStr, NULL);
}

void box_notify_power_on_end()
{
	const char reqStr[] = {
		0xbf ,0xaa ,0xb5 ,0xe7 ,
		0xcd ,0xea ,0xb3 ,0xc9 ,
		0xa3 ,0xa1 ,0x00
	}; //开电完成！
	
	boxFeedback(reqStr, NULL);
}

void box_notify_power_down()
{
	char error[64] = "";
	box_gen_pass_failed_msg_data("Pass", 1, error, sizeof(error));

	const char reqStr[] = {
		0xc9 ,0xe8 ,0xb1 ,0xb8 ,
		0xb9 ,0xd8 ,0xb5 ,0xe7 ,
		0x21 ,0x00
	}; //设备关电！

	boxFeedback(reqStr, error);
}

void box_notify_power_error_down()
{
	char error[64] = "";
	box_gen_pass_failed_msg_data("Fail", 0, error, sizeof(error));

	const char reqStr[] = {
		0xb5 ,0xe7 ,0xd4 ,0xb4 ,
		0xbc ,0xec ,0xb2 ,0xe9 ,
		0xb4 ,0xed ,0xce ,0xf3 ,
		0xa3 ,0xac ,0xcd ,0xa3 ,
		0xd6 ,0xb9 ,0xbf ,0xaa ,
		0xb5 ,0xe7 ,0x21 ,0x00
	}; //电源检查错误，停止开电!

	boxFeedback(reqStr, error);
}

void box_notify_openshort_error_down()
{
	char error[64] = "";
	box_gen_pass_failed_msg_data("Fail", 0, error, sizeof(error));

	const char reqStr[] = {
		0xbf ,0xaa ,0xb6 ,0xcc ,
		0xc2 ,0xb7 ,0xbc ,0xec ,
		0xb2 ,0xe9 ,0xb4 ,0xed ,
		0xce ,0xf3 ,0xa3 ,0xac ,
		0xcd ,0xa3 ,0xd6 ,0xb9 ,
		0xbf ,0xaa ,0xb5 ,0xe7 ,
		0x21 ,0x00
	}; //开短路检查错误，停止开电!

	boxFeedback(reqStr, error);
}

void box_notify_NTC_error_down()
{
	char error[64] = "";
	box_gen_pass_failed_msg_data("Fail", 0, error, sizeof(error));

	const char reqStr[] = {
		0x4e ,0x54 ,0x43 ,0xbc ,
		0xec ,0xb2 ,0xe2 ,0xb3 ,
		0xac ,0xcf ,0xde ,0xa3 ,
		0xac ,0xcd ,0xa3 ,0xd6 ,
		0xb9 ,0xbf ,0xaa ,0xb5 ,
		0xe7 ,0xa3 ,0xa1 ,0x00
	}; //NTC检测超限，停止开电！

	boxFeedback(reqStr, error);
}

void box_show_being_power_on_msg()
{
	const char reqStr[] = {
		0xc9 ,0xe8 ,0xb1 ,0xb8 ,
		0xbf ,0xaa ,0xb5 ,0xe7 ,
		0xd6 ,0xd0 ,0x2e ,0x2e ,
		0x2e ,0x00
	}; //设备开电中...

	boxFeedback(reqStr, NULL);
}

unsigned int box_get_version()
{
	return boxSoftwareVersion;
}

static void boxThreadCreate(void)
{
    INT ret = 0;
    pthread_t boxthread1;
    pthread_t boxthread2;
    ret = pthread_create(&boxthread1, NULL, (void *) boxHandThread, NULL);
    ret = pthread_create(&boxthread2, NULL, (void *) boxRcvThread, NULL);
    if (ret != 0)
    {
        printf("can't create thread: %s.\r\n", strerror(ret));
        return ;
    }
	
    pthread_detach(boxthread1);
    pthread_detach(boxthread2);
}

static box_cur_state_t  box_cur_state;

void clear_box_state()
{
	memset(&box_cur_state, 0, sizeof(box_cur_state));
}

#define BOX_TYPE 0xF0

static INT boxCmdTransfer(BYTE cmd, void *pBuf, WORD16 nLen,void *pNode)
{
    int ret  = 0;
    int sendLen = sizeof(SMsgHead)+nLen;
    char *pSendBuf = (char*)malloc(sendLen);
    if(nLen>0)
    {
        memcpy(&pSendBuf[sizeof(SMsgHead)],pBuf,nLen);
    }
	
    SMsgHead *pSmgHead = (SMsgHead *)pSendBuf;
    pSmgHead->type = BOX_TYPE;
    pSmgHead->cmd  = cmd;
    pSmgHead->len  = nLen;
    pthread_mutex_lock(&gBoxReadLock);

	usleep(20 * 1000);
    ret = uartSendMsgToBoard(getBoxUartNo(), pSendBuf, sendLen);
			
    pthread_mutex_unlock(&gBoxReadLock);
	free(pSendBuf);
    return ret;
}

void boxSavePwrData(sByPtnPwrInfo *pPwrData, short vcom, short ivcom)
{
    memcpy(&box_cur_state.pwrInfo,pPwrData,sizeof(sByPtnPwrInfo));
    boxpwr_info_t boxpwr_info;
    boxpwr_info.vdd = pPwrData->VDD;
    boxpwr_info.idd = pPwrData->iVDD/10; //pPwrData.ratio
    boxpwr_info.vbl = pPwrData->VBL;
    boxpwr_info.ibl = pPwrData->iVBL/10;

	boxpwr_info.vint = pPwrData->VDDIO;
	boxpwr_info.ivint = pPwrData->iVDDIO/10;

	boxpwr_info.avsp = pPwrData->VSP;
	boxpwr_info.ivsp = pPwrData->iVSP/10;

	boxpwr_info.avsn = pPwrData->VSN;
	boxpwr_info.ivsn = pPwrData->iVSN/10;

	boxpwr_info.vgh = pPwrData->ELVDD;
	boxpwr_info.ivgh = pPwrData->iELVDD/10;

	boxpwr_info.vgl = pPwrData->ELVSS;
	boxpwr_info.ivgl = pPwrData->iELVSS/10;

	boxpwr_info.vcom = vcom;
	boxpwr_info.ivcom = ivcom/10;
	
    boxCmdTransfer(CHECK_PWR_KEY,&boxpwr_info, sizeof(boxpwr_info), 0);
}

void boxFeedback(const char* pReqDescript, const char* pError)
{
	int i;
	box_otp_info_t info;
	unsigned char* ptr;

	memset(&info, 0x20, sizeof(info));

	if(pReqDescript != NULL)
	{
		int cpyLen = strlen(pReqDescript);
		cpyLen = (cpyLen > MAX_ROW_CHAR) ? MAX_ROW_CHAR:cpyLen;
		memcpy(info.ch1, pReqDescript, cpyLen);
	}

	if(pError != NULL)
	{
		int cpyLen = strlen(pError);
		cpyLen = (cpyLen > MAX_ROW_CHAR) ? MAX_ROW_CHAR:cpyLen;
		memcpy(info.ch2, pError, cpyLen);
	}

	boxCmdTransfer(OPT_KEY, &info, sizeof(info), 0);
}

static void boxHandThread()
{
    while(1)
    {
		if(gBoxUpdateState == 1 || gPowerUpdateState == 1 || gBoxKeyReturn == 1)
		{
			usleep(10 * 1000);
			continue;
		}

		if (box_time_out_enable == 0)
		{
			usleep(1000 * 1000);
			continue;
		}
		
        pthread_mutex_lock(&gBoxGuardLock);
        time_t t = time(NULL);
        
        if(t - boxtime_alive > 10)
        {
            boxtime_alive = t;
            if (getBoxUartNo() >= 0)
            {
				box_go_do_home_proc();
            }
        }
        pthread_mutex_unlock(&gBoxGuardLock);
		usleep(10 * 1000);
    }
}

static int use_SendCode = 0;

static int m_box_vcom_info_first = 1;
static vcom_info_t m_box_vcom_info;
#define USE_POWER_ON_OFF   1
#define USE_GPIO_RESET_2828_RESET   0

void box_shutdown_clear_power_info()
{
	sByPtnPwrInfo pwrData = { 0 };
	usleep(100 * 1000);
	boxSavePwrData(&pwrData, 0, 0);
}

void box_go_do_home_proc()
{
	clear_box_state();
	boxCmdTransfer(HAND_KEY,0,0,0);

	box_shutdown_clear_power_info();
}

static void boxProcCmd(int cmd)
{
    switch(cmd)
    {
        case  UP_KEY:
        {
            if(!box_cur_state.isConnected)
        	{
        		printf("boxProcCmd: UP_KEY: disconnted!\n");
            	break;
        	}
			
            if(box_cur_state.boxLayer == MODULE_LAYER)
            {
                module_self_t *pModule_Self  = dbModuleSelfGetByIndex(box_cur_state.curModuleIndex - 1);
                if(pModule_Self)
                { 
					strncpy(box_cur_state.curModuleName, pModule_Self->moduleselfName, 31);
                    strncpy(box_cur_state.curModulePwrName, "", 31);
					box_cur_state.curModuleIndex = pModule_Self->index;
					box_cur_state.curTotalPtnNums = pModule_Self->totalPattern;
					
                    change_info_t change_info;
                    change_info.isModuleFlag = 1;
                    strncpy(change_info.moduleName, pModule_Self->moduleselfName, 31);
					strncpy(change_info.pwrName, "", 31);
                    strncpy(change_info.pictureName, pModule_Self->patternFirstName, 31);
                    change_info.curindex = box_cur_state.curModuleIndex;                    
                    change_info.totalNum = box_cur_state.curTotalPtnNums;
					
                    printf("UP_KEY totalNum %d\n", change_info.totalNum);
                    boxCmdTransfer(UP_KEY, &change_info, sizeof(change_info), 0);
                }
            }
            else if(PICTURE_LAYER == box_cur_state.boxLayer)
            {
                int ptnId = box_cur_state.curPtnId - 1;
                module_info_t *pModule_info = dbModuleGetPatternById(ptnId);
                if(pModule_info)
                {
                    box_cur_state.curPtnId = ptnId;
                    client_pg_showPtn(ptnId);
					
                    change_info_t change_info = { 0 };
                    change_info.isModuleFlag = 2;
                    strncpy(change_info.moduleName, box_cur_state.curModuleName, 31);
					strncpy(change_info.pwrName, "", 31);
                    strncpy(change_info.pictureName, pModule_info->ptdpatternname, 31);
                    change_info.curindex = box_cur_state.curPtnId;
                    change_info.totalNum = box_cur_state.curTotalPtnNums;
                    change_info.lockSec  = pModule_info->ptddisptime;
					
                    printf("UP_KEY NAME %s CURIndex %d curLock %d\n",change_info.pictureName,change_info.curindex,change_info.lockSec);
                    boxCmdTransfer(UP_KEY, &change_info, sizeof(change_info), 0);
                }
            }
        }
        break;

        case  DOWN_KEY:
        {
            if(!box_cur_state.isConnected)
			{
        		printf("boxProcCmd: DOWN_KEY: disconnted!\n");
                break;
            }
			
            if(box_cur_state.boxLayer == MODULE_LAYER)
            {
                module_self_t *pModule_Self  = dbModuleSelfGetByIndex(box_cur_state.curModuleIndex + 1);
                if(pModule_Self)
                {
                    box_cur_state.curModuleIndex = pModule_Self->index;
					box_cur_state.curTotalPtnNums = pModule_Self->totalPattern;
					
                    change_info_t change_info;
                    change_info.isModuleFlag = 1;
                    strncpy(change_info.moduleName, pModule_Self->moduleselfName, 31);
                    strncpy(change_info.pwrName, "", 31);
                    strncpy(change_info.pictureName, pModule_Self->patternFirstName, 31);
                    strncpy(box_cur_state.curModuleName, pModule_Self->moduleselfName, 31);
					strncpy(box_cur_state.curModulePwrName, "", 31);
                    change_info.curindex = box_cur_state.curModuleIndex;                    
                    change_info.totalNum = box_cur_state.curTotalPtnNums;
					
                    printf("DW_KEY totalNum %d\n", change_info.totalNum);
                    boxCmdTransfer(DOWN_KEY, &change_info, sizeof(change_info), 0);
                }
				else
				{
					printf("DOWN_KEY: module layer error!\n");
				}
        }
            else if(box_cur_state.boxLayer == PICTURE_LAYER)
            {
            	int ptnId = 0;
				printf("box_cur_state.curPtnId = %d, total = %d.\n", box_cur_state.curPtnId, box_cur_state.curTotalPtnNums);
				if (box_cur_state.curPtnId < box_cur_state.curTotalPtnNums - 1)
				{					
					ptnId = box_cur_state.curPtnId + 1;
				}
				else
				{
					ptnId = box_cur_state.curPtnId;
					printf("down key:  ========== zzzzzzzzzz  =======  picture end: ===========\n");
				}
				
				if(box_cur_state.pgPwrStatus == 0) 
				{
        			printf("boxProcCmd: DOWN_KEY: box_cur_state.pgPwrStatus == 0!\n");
					ptnId = box_cur_state.curPtnId;
					printf("ptnid = %d.\n", ptnId);
				}
				
                module_info_t *pModule_info = dbModuleGetPatternById(ptnId);
                if(pModule_info)
                {
                    box_cur_state.curPtnId = ptnId;

					box_time_out_enable = 0;
                    client_pg_showPtn(ptnId);
					box_time_out_enable = 1;
					
                    change_info_t change_info;
                    change_info.isModuleFlag = 2;
                    strncpy(change_info.moduleName, box_cur_state.curModuleName, 31);
					strncpy(change_info.pwrName, "", 31);
                    strncpy(change_info.pictureName, pModule_info->ptdpatternname, 31);
                    change_info.curindex = box_cur_state.curPtnId;
                    change_info.totalNum = box_cur_state.curTotalPtnNums;
                    change_info.lockSec = pModule_info->ptddisptime;
					
                    printf("DWN_KEY NAME %s CURIndex %d curLock %d ptnNum %d\n", change_info.pictureName, change_info.curindex, change_info.lockSec, box_cur_state.curTotalPtnNums);
                    boxCmdTransfer(DOWN_KEY, &change_info, sizeof(change_info), 0);
                }
				else
				{
					printf("Down Key: picture layer: index = %d. \n", ptnId);
				}
            }
        }
        break;

        case  POWER_KEY:
        {
        	int channel = 0x0F;
			int signal_type = 0;
				
			if(box_cur_state.boxLayer == MODULE_LAYER)
			{
    			printf("boxProcCmd: POWER_KEY: MODULE_LAYER!\n");
				break;
			}
			
            if(!box_cur_state.isConnected)
			{
    			printf("boxProcCmd: POWER_KEY: !box_cur_state.isConnected!\n");
                break;
            }
			
			printf("\n\n");
			gBoxKeyReturn = 1;
			
            if(box_cur_state.pgPwrStatus)
            {
            	printf("== box power off ...\n");
				client_pg_shutON(0, 0, channel, signal_type);
				
                box_shut_info_t box_shut_info;
                box_shut_info.vcom = 0;
                box_shut_info.vcom_otp  = 0;
                box_shut_info.pgPwrStatus = client_pg_pwr_status_get();

				box_cur_state.pgPwrStatus = box_shut_info.pgPwrStatus;
				box_cur_state.curPtnId = box_shut_info.ptnIndex = 0;
                
                module_info_t *pModule_info = dbModuleGetPatternById(box_shut_info.ptnIndex);
                if(pModule_info)
                {
                    strncpy(box_shut_info.pictureName, pModule_info->ptdpatternname, 31);
                    box_shut_info.lockSec = 0;
                }
				
				gBoxKeyReturn = 0;
                boxCmdTransfer(POWER_KEY, &box_shut_info, sizeof(box_shut_info), 0);

				// clear power inf.
				box_shutdown_clear_power_info();

				printf("== box power off OK!\n");
            }
            else
            {
            	printf("== box power on ...\n");
            	// power on
				struct timeval tpstart, tpend; 
				float timeuse; 
				gettimeofday(&tpstart,NULL);

				set_box_power_on_status(0);
				set_box_time_out_disable();
				
				int power_on_result = client_pg_shutON(1, box_cur_state.curModuleName, channel, signal_type);

				set_box_time_out_enable();
				set_box_power_on_status(power_on_result);
				
				{							
					box_cur_state.pgPwrStatus = client_pg_pwr_status_get();
					
					box_shut_info_t box_shut_info;
					memset(&box_shut_info, 0, sizeof(box_shut_info));
					
					box_shut_info.pgPwrStatus = box_cur_state.pgPwrStatus;
					box_cur_state.curPtnId = box_shut_info.ptnIndex = 0;

					#if 1
					module_info_t *pModule_info = dbModuleGetPatternById(box_shut_info.ptnIndex);
					if(pModule_info)
					{
						box_shut_info.lockSec = pModule_info->ptddisptime;
						strncpy(box_shut_info.pictureName,pModule_info->ptdpatternname,31);
					}
					#endif

					gBoxKeyReturn = 0;
					boxCmdTransfer(POWER_KEY,&box_shut_info,sizeof(box_shut_info),0);
				}
				
				gBoxKeyReturn = 0;						
				printf("====== Box Power On End =======\n");

				printf("== box power on OK.\n");
            }

        }
        break;

        case  OK_KEY:
        {
            if(!box_cur_state.isConnected)
			{
    			printf("boxProcCmd: OK_KEY: !box_cur_state.isConnected!\n");
                break;
            }
			
            if(box_cur_state.boxLayer == MODULE_LAYER)
            {
				gBoxKeyReturn = 1;
				
               set_cur_module_name(box_cur_state.curModuleName);
               write_cur_module_name();
			   
               load_cur_module();
			   gBoxKeyReturn = 0;
			   //
               box_cur_state.boxLayer = PICTURE_LAYER;
               box_cur_state.curPtnId = 0;
			   //
               box_pwr_status_t box_pwr_status;
               box_pwr_status.pgPwrStatus = client_pg_pwr_status_get();
               boxCmdTransfer(OK_KEY,&box_pwr_status, sizeof(box_pwr_status), 0);

               printf("boxProcCmd: OK_KEY: ptn num %d\n",box_cur_state.curTotalPtnNums);
            }
        }
        break;

        case  HOME_KEY:
        {
			printf("box debug: home key\n");
			boxFeedback(NULL, NULL);
			usleep(100 * 1000);
			
            if(!box_cur_state.isConnected)
            {
            	printf("========   boxProcCmd: home key: connected!\n");
				
                //1.connected
                box_cur_state.isConnected = 1;
				
                char modelName[256];
                int ret = read_cur_module_name(modelName);
				
                if(ret == 0)
                {
                    int moduelNums = dbInitModuleSelf();
                    box_cur_state.totalModuleNum = moduelNums;
                    strcpy(box_cur_state.curModuleName,modelName);
                    module_self_t *pModule_Self  = dbModuleSelfGetByName(modelName);
                    if(pModule_Self)
                    {
                        box_cur_state.curModuleIndex = pModule_Self->index;
                        int pgpwrStatus = client_pg_pwr_status_get();;

                        hand_info_t  hand_info;
                        memset(&hand_info,0,sizeof(hand_info_t));
                        strncpy(hand_info.moduleName,modelName,31);
                        strncpy(box_cur_state.curModuleName,modelName,31);
						strncpy(hand_info.pwrName, "", 31);
						strncpy(box_cur_state.curModulePwrName, "", 31);


                        hand_info.pgPwrStatus = pgpwrStatus;
                        hand_info.moduleTotalNum  = moduelNums;
                        hand_info.curModuleIndex  = pModule_Self->index;
                        hand_info.pictureTotalNum = pModule_Self->totalPattern;
						
                        if(pgpwrStatus == 0)
                        {
                            box_cur_state.boxLayer = MODULE_LAYER;
							
                            hand_info.curPictureIndex = 0;
                            strncpy(hand_info.pictureName,pModule_Self->patternFirstName,31);
                        }
                        else
                        {
                            box_cur_state.curPtnId = client_pg_ptnId_get();
                            box_cur_state.boxLayer = PICTURE_LAYER;
                            hand_info.curPictureIndex = box_cur_state.curPtnId;
                            module_info_t *pModule_info = dbModuleGetPatternById(box_cur_state.curPtnId);
                            if(pModule_info)
                            {
                                strncpy(hand_info.pictureName,pModule_info->ptdpatternname,31);
                            }
                        }

                        box_cur_state.curTotalPtnNums = pModule_Self->totalPattern;
                        boxCmdTransfer(HOME_KEY,&hand_info,sizeof(hand_info),0);
                        printf("boxProcCmd: HOME_KEY: model name %s, picture total is %d\n",modelName,pModule_Self->totalPattern);
						
                    }
                }
				else
				{
					printf("=====  boxProcCmd: HOME_KEY: Invalid Module.\n");
				}
				
                break;
            }
			
            if(PICTURE_LAYER == box_cur_state.boxLayer)
            {
            	printf("=======  boxProcCmd: HOME_KEY: box layer!\n");
				
                box_cur_state.boxLayer = MODULE_LAYER;

                int ptnId = box_cur_state.curPtnId;
                module_info_t *pModule_info = dbModuleGetPatternById(ptnId);
                if(pModule_info)
                {
                    hand_info_t  hand_info;
                    memset(&hand_info, 0, sizeof(hand_info_t));
                    strncpy(hand_info.moduleName, box_cur_state.curModuleName,31);
					strncpy(hand_info.pwrName, "",31);
                    strncpy(hand_info.pictureName,pModule_info->ptdpatternname,31);
					
                    hand_info.pgPwrStatus = client_pg_pwr_status_get();
                    hand_info.moduleTotalNum = box_cur_state.totalModuleNum;
                    hand_info.curModuleIndex = box_cur_state.curModuleIndex;
                    hand_info.pictureTotalNum = box_cur_state.curTotalPtnNums;
                    hand_info.curPictureIndex = 0;
					
                    printf("2model name %s picture total is %d xx:%s\n",box_cur_state.curModuleName,box_cur_state.curTotalPtnNums,hand_info.pwrName);
                    boxCmdTransfer(HOME_KEY,&hand_info,sizeof(hand_info),0);

					printf("box debug: home key pic\n");
                }
            }
			else
			{
    			printf("====  boxProcCmd: HOME_KEY: !PICTURE_LAYER!\n");
			}
        }
        break;

        case  OPT_KEY: 
        {
        }
        break;

        case CHECK_PWR_KEY:
        {
        }
        break;

        default:
			printf("boxProcCmd: Invalid Box Key: %d\n", cmd);
        break;
    }
}


static void boxSendCmd(int cmd)
{
    unsigned int msgFlag = 0xa0000000;
    switch(cmd)
    {
        case  UP_KEY:
		//printf("--------------UP_KEY----------\n");
		if(gBoxKeyReturn == 0)
        {
            MsOS_PostMessage(boxApartMent->MessageQueue,msgFlag|cmd, 0,0);
        }
        break;

        case  DOWN_KEY:
		printf("--------------DOWN_KEY----------\n");
		if(gBoxKeyReturn == 0)
        {
            MsOS_PostMessage(boxApartMent->MessageQueue,msgFlag|cmd, 0,0);
        }
        break;

        case  POWER_KEY:
		printf("--------------POWER_KEY----------\n");
		if(gBoxKeyReturn == 0)
        {
            MsOS_PostMessage(boxApartMent->MessageQueue,msgFlag|cmd, 0,0);
        }
        break;

        case  OK_KEY:
		printf("--------------OK_KEY----------\n");
		if(gBoxKeyReturn == 0)
        {
            MsOS_PostMessage(boxApartMent->MessageQueue,msgFlag|cmd, 0,0);
        }
        break;

        case  HOME_KEY:
		printf("--------------HOME_KEY----------\n");
		if(gBoxKeyReturn == 0)
        {
            MsOS_PostMessage(boxApartMent->MessageQueue, msgFlag|cmd, 0,0);
        }
        break;

        case  OPT_KEY:
		printf("--------------OPT_KEY----------\n");
		if(gBoxKeyReturn == 0)
        {
            MsOS_PostMessage(boxApartMent->MessageQueue,msgFlag|cmd, 0,0);
        }
        break;

        case CHECK_PWR_KEY:
		//printf("--------------GET_POWER_KEY----------\n");
		if(box_cur_state.pgPwrStatus && gBoxKeyReturn == 0)
        {
        	if (client_pg_pwr_status_get() == 1)
    		{
        		getPwrInfo(PWR_CHANNEL_RIGHT);
    		}
        }
        break;

        default:
		printf("--------------KEY[%d]----------\n", cmd);
        break;
    }
}

static void boxRcvThread()
{
    struct timeval delaytime;
    BYTE buf[1024] = {0};
    INT readlen = 0;
    delaytime.tv_sec = 0;
    delaytime.tv_usec = 50000;
    BYTE cmd;
    SMsgHead *pSmsg;

    memset(&box_cur_state,0,sizeof(box_cur_state));

    while (1)
    {
        if (getBoxUartNo() >= 0)
        {
            readlen = uartReadData(getBoxUartNo(), delaytime, buf);
        }

        if(readlen<=0)
        {
			usleep(10 * 1000);
            continue;
        }

        pthread_mutex_lock(&gBoxGuardLock);
        boxtime_alive = time(NULL);
        pthread_mutex_unlock(&gBoxGuardLock);

		if(buf[0] == 0x08)
		{
			boxSoftwareVersion_request = 0;
			//
			unsigned char* ver = &(buf[21]);
			printf("box.c  box version: %d.%d.%d\n", ver[0], ver[1], ver[2]);

			unsigned int ver1 = ver[0] & 0xff;
			unsigned int ver2 = ver[1] & 0xff;
			unsigned int ver3 = ver[2] & 0xff;
			unsigned int version = 0;
			version = (ver1<<16) | (ver2<<8) | (ver3);
			boxSoftwareVersion = version;
			client_sendBoxVersion(1, version);
			continue;
		}

        pSmsg = buf;
        cmd = pSmsg->cmd;

        boxSendCmd(cmd);
    }
}

void box_upgrade(unsigned char* file_buffer, unsigned int file_size)
{
	int UartNo;
	unsigned char type;
	pthread_mutex_t* lock;
	if(1)
	{
		UartNo = getBoxUartNo();
		lock = &gBoxReadLock;
		type = 0x50;
	}
	else
	{
		extern pthread_mutex_t gPwrReadLock;
		UartNo = getPwrUartNo();
		type = 0x05;
		lock = &gPwrReadLock;
	}

	pthread_mutex_lock(lock);
	
	int ret;
	unsigned int num;
	unsigned char txt[1500];
	num = 0;
	txt[num++] = 0x72;
	txt[num++] = type;
	//
	txt[num++] = 0x4;
	txt[num++] = 0x0;
	//
	txt[num++] = 0x0;
	txt[num++] = 0x0;
	txt[num++] = 0x0;
	txt[num++] = 0x0;
	ret = uartSendMsgToBoard(UartNo, txt, num);
	if(ret != 0) 
		printf("72 0B error\n");
	
	printf("box upgrade mode ...\n");
	sleep(4);

	//???????
	unsigned int offset = 0;
	unsigned char* buf = file_buffer;
	unsigned int len = file_size;
	
	while(len > 0)
	{
		unsigned int datalen = 0;
		if(len >= 1024)
		{
			datalen = 1024;
		}
		else
		{
			datalen = len;
		}

		num = 0;
		txt[num++] = 0x73;
		txt[num++] = type;
		//2
		unsigned short packdatalen = /*???????????????????*/4 + /*????????????A??*/4 + /*????????????*/4 + /*????????????A??*/datalen;
		txt[num++] = (packdatalen) & 0xff;
		txt[num++] = (packdatalen>>8) & 0xff;
		//4
		txt[num++] = (offset) & 0xff;
		txt[num++] = (offset>>8) & 0xff;
		txt[num++] = (offset>>16) & 0xff;
		txt[num++] = (offset>>24) & 0xff;
		//4
		txt[num++] = (datalen) & 0xff;
		txt[num++] = (datalen>>8) & 0xff;
		txt[num++] = (datalen>>16) & 0xff;
		txt[num++] = (datalen>>24) & 0xff;
		//4
		txt[num++] = (file_size) & 0xff;
		txt[num++] = (file_size>>8) & 0xff;
		txt[num++] = (file_size>>16) & 0xff;
		txt[num++] = (file_size>>24) & 0xff;
		//datalen
		memcpy((&(txt[num])), buf, datalen);
		num += datalen;
		ret = uartSendMsgToBoard(UartNo, txt, num);
		
		if(ret != 0) 
			printf("73 0B error, offset=%d\n", offset);
		
		printf("box upgrade data: offset: %d, len: %d\n", offset, datalen);
		usleep(800*1000);

		//
		offset += datalen;
		buf += datalen;
		len -= datalen;
	}

	printf("box upgrade end. total size: %d.\n", offset);

	pthread_mutex_unlock(lock/*&gBoxReadLock*/);
	usleep(1000*500);

	boxSoftwareVersion = 0;
	boxSoftwareVersion_request = 1;
	sem_post(&boxSoftwareVersion_sem);
}

static MS_U32 box_message_proc(MS_APARTMENT *pPartMent,MSOS_MESSAGE Message)
{
	// box upgrade.
	if(Message.MessageID == 0x12345678)
	{
		unsigned char* file_buffer = (unsigned char*)Message.Parameter1;
		unsigned int file_size = (unsigned int)Message.Parameter2;
		gBoxUpdateState = 1;
		gBoxKeyReturn = 1;
		usleep(10 * 1000);
		box_upgrade(file_buffer, file_size);
		gBoxKeyReturn = 0;
		gBoxUpdateState = 0;

		printf("box_message_proc: *******  box upgrade.\n");
		return 0;
	}

	// power upgrade.
	if(Message.MessageID == PWR_CMD_UPGRADE) //???????
	{
		printf("=== box power upgrade\n");
		unsigned char* file_buffer = (unsigned char*)Message.Parameter1;
		unsigned int file_size = (unsigned int)Message.Parameter2;
		gBoxUpdateState = 1;
		gBoxKeyReturn = 1;
		usleep(10 * 1000);
		pwrUpgrade(file_buffer, file_size);
		gBoxKeyReturn = 0;
		gBoxUpdateState = 0;
		return 0;
	}
	
	if(Message.MessageID == BOX_CMD_GET_VER)
	{
		if(boxSoftwareVersion_request == 1)
		{
			boxCmdTransfer_version();
		}

		return 0;
	}
	

    unsigned char cmd  = Message.MessageID&0xffff;
    boxProcCmd(cmd);
	
    return 0;
}

unsigned int box_message_queue_get()
{
    return boxApartMent->MessageQueue;
}

INT boxCmdTransfer_version()
{
	//BYTE cmd ;
	void *pBuf = NULL;
	WORD16 nLen = 0;
	void *pNode = NULL;

	int ret  = 0;
	int sendLen = sizeof(SMsgHead)+nLen;
	char *pSendBuf = (char*)malloc(sendLen);
	if(nLen>0)
	{
		memcpy(&pSendBuf[sizeof(SMsgHead)],pBuf,nLen);
	}
	SMsgHead *pSmgHead = (SMsgHead *)pSendBuf;
	pSmgHead->type = 0x50;//BOX_TYPE;
	pSmgHead->cmd  = 0x08;//cmd;
	pSmgHead->len  = nLen;
	pthread_mutex_lock(&gBoxReadLock);

	ret = uartSendMsgToBoard(getBoxUartNo(), pSendBuf, sendLen);
	pthread_mutex_unlock(&gBoxReadLock);
	free(pSendBuf);
	return ret;
}

static void* boxSoftwareVersion_Task(void* arg)
{
	sleep(4);
	
	while(1)
	{
		sem_wait(&boxSoftwareVersion_sem);
		if(boxSoftwareVersion_request > 0)
		{
			while(boxSoftwareVersion_request > 0)
			{
				sleep(2);
				
				if(gBoxKeyReturn == 1) 
					continue;
				
				MsOS_PostMessage(boxApartMent->MessageQueue, BOX_CMD_GET_VER, 0, 0);
			}
		}
	}
	return NULL;
}

INT initBox(void)
{
    MS_EVENT_HANDLER_LIST *pEventHandlerList = 0;

    traceMsg("func %s\n", __FUNCTION__);

    pthread_mutex_init(&gBoxReadLock, NULL);

    pthread_mutex_init(&gBoxGuardLock, NULL);

    boxApartMent = MsOS_CreateApartment("lcdTask", box_message_proc, pEventHandlerList);

    boxCmdTransfer(HAND_KEY,0,0,0);

    boxtime_alive = time(NULL);

    pthread_t boxthread1;
    pthread_t boxthread2;
    int ret = pthread_create(&boxthread1, NULL, (void *) boxHandThread, NULL);
    ret = pthread_create(&boxthread2, NULL, (void *) boxRcvThread, NULL);
    if (ret != 0)
    {
        printf("can't create thread: %s.\r\n", strerror(ret));
        return -1;
    }
	
    pthread_detach(boxthread1);
    pthread_detach(boxthread2);

	sem_init(&boxSoftwareVersion_sem, 0, 0);
	pthread_create(&boxSoftwareVersion_pthread, NULL, boxSoftwareVersion_Task, NULL);
	if(boxSoftwareVersion_request == 1)
	{
		sem_post(&boxSoftwareVersion_sem);
	}
	
    return SUCCESS;
}
