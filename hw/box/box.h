#ifndef  __BOX_H__
#define  __BOX_H__

#include "common.h"
#include "comStruct.h"

#define FLICKER_OK     0
#define FLICKER_NG_2   2

#pragma pack(push)
#pragma pack(1)

typedef enum
{
    HAND_KEY = 0X80,
    UP_KEY = 0x81,
    DOWN_KEY = 0x82,
    POWER_KEY = 0x83,
    OK_KEY = 0x84,
    HOME_KEY = 0x85,
    OPT_KEY = 0x86,
    CHECK_PWR_KEY = 0x87,
}EBOX_KEY;

typedef struct
{
    int  isConnected;
    int  pgPwrStatus;
    int  boxLayer; //module,picture
    int  isAutoFlick;
    int  totalModuleNum;
    int  curModuleIndex;
    char curModuleName[32];
    char curModulePwrName[32];
    int  curTotalPtnNums;
    int  curPtnId;
    sByPtnPwrInfo pwrInfo;
}box_cur_state_t;


#define MODULE_LAYER  1
#define PICTURE_LAYER 2

typedef struct tag_hand_info_s
{
    char moduleName[32];
    char pictureName[32];
    char pwrName[32];
    short  pgPwrStatus; //0 off 1:on
    short  moduleTotalNum;
    short  curModuleIndex;
    short  pictureTotalNum;
    short  curPictureIndex;
}hand_info_t;

typedef struct tag_change_info_s
{
    short  isModuleFlag; // 1:module 2:picture
    short  curindex;
    short  totalNum;
    short  lockSec;
    char moduleName[32];
    char pictureName[32];
    char pwrName[32];
}change_info_t;

// power: VDD, VBL, VINT(vddio), AVSP, AVSN, VGH, VGL, VCOM.
typedef struct tag_box_pwr_info_s
{
    short vdd;
    short idd;
    unsigned short vbl;
    short ibl;
	
	short vint;	// vddio
	short ivint;

	short avsp;
	short ivsp;

	short avsn;
	short ivsn;

	short vgh;
	short ivgh;

	short vgl;
	short ivgl;

	short vcom;
	short ivcom;
	
}boxpwr_info_t;

typedef struct tag_box_shut_info_s
{
    short vcom;
    short vcom_otp;
    short alloptStatus;
    short pgPwrStatus;
    short ptnIndex;
    short lockSec;
    char  pictureName[32];
}box_shut_info_t;

typedef struct tag_box_pwr_status_s
{
    short   pgPwrStatus;
}box_pwr_status_t;

#define MAX_ROW_CHAR  (32+6)
typedef struct tag_box_otp_info_s
{
	char ch1[MAX_ROW_CHAR];
	char ch2[MAX_ROW_CHAR];
}box_otp_info_t;

#pragma pack(pop)

void boxSavePwrData(sByPtnPwrInfo *pPwrData, short vcom, short ivcom);

//
#define BOX_CMD_GET_VER    0x44000001

//Flicker OK ��¼ʱ�� 8.23S


//Flicker NG �������� 1,   ��¼��������
#define FLICKER_NG_1   1

//Flicker NG �������� 2,   FLICK�豸����, ������,  �����ı������ ----.--, ���߶�ȡ����ֵΪ0
#define FLICKER_NG_2   2

//Flicker NG �������� 3,   FLICK�豸��Ӧ��ֵҪô�ܴ�, Ҫô��С, ��ƽ��ʱ, ˵���豸������
#define FLICKER_NG_3   3

//Flicker NG �������� 4,   FLICK�豸��Ӧ��С��ֵ��PG�趨��radio range��, ��������¼.
#define FLICKER_NG_4   4

//Flicker NG �������� 5,   ��¼ʧ��, ������û������, ���VCOMֵ����¼�Ĳ�һ��
#define FLICKER_NG_5   5

//Flicker NG �������� 6,
#define FLICKER_NG_6   6

//Flicker NG ������¼, ����û����¼��ȥ
#define FLICKER_NG_OTP_COUNT_ERR   7

void boxFeedback(const char* pReqDescript, const char* pError);

void box_go_do_home_proc();

INT boxCmdTransfer_version();

#endif
