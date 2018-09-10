#ifndef COMSTRUCT_H_
#define COMSTRUCT_H_

#if 1
#include "common.h"

#pragma pack(push)
#pragma pack(1)

typedef INT (*RFUNC)(INT, struct timeval);
typedef INT (*SFUNC)(CHAR *, INT, void *);
typedef INT (*FUN)(BYTE , void * , WORD16 wlen, LPVOID);

typedef struct _process_cmd
{
    BYTE cmd;
    FUN cmdFun;
} T_CMD_PROCFUN;

typedef struct _XML_UART_
{
    INT  xmlTtyStatus;  // 0:disable 1:enable
    BYTE xmlUartName[16];
    BYTE xmlTtyName[16];
    BYTE xmlRoleName[16];
}T_XML_UART;

typedef struct sIntComNodeInf
{
    BYTE bResponseFlag;//0:not response 1:response
    BYTE res[2];
    INT fd;
    struct sockaddr_in  cliaddr;
}ComNodeIf;

typedef struct sIntComDevicdInf
{
    ComNodeIf nodeInf;
    SFUNC sndFunc;
    RFUNC rcvFunc;
}ComDeviceIf;

enum 
{
	E_SYS_CMD_SENDER_NET = 1,
	E_SYS_CMD_SENDER_BOX,
	
};

enum 
{
	E_BL_MODE_CONST_VOLT = 0,
	E_BL_MODE_CONST_CURRENT
};
	
typedef struct PWR_MSG_HEAD
{
    BYTE 	cmd;        //[command word]
    BYTE 	type;       //[command type]
    WORD16 	len;        //[payload size,defalut is 0]
    WORD16	dest_addr;  //[link,level]
    WORD16  signl_type; //[1:ethernet,2:serial,3:arm]
    WORD16  signl_fd;   //offline setup: serial fd or socket fd
    unsigned long   ipaddr;     //pc ip address
    WORD16  ways;       //option ways
    WORD32  result;     //option result: [0:success,1:failure]
}tPwrMsgHead;

#define PWR_MSG_HEAD_LEN	(20)

typedef struct pwrMsgHead
{
    unsigned char 	cmd;        //[command word]
    unsigned char 	type;       //[command type]
    unsigned short 	len;        //[payload size,defalut is 0]
    unsigned short	destAddr;  //[link,level]
    unsigned short  signalType; //[1:ethernet,2:serial,3:arm]
    unsigned short  signalFd;   //offline setup: serial fd or socket fd
    unsigned long   ipaddr;     //pc ip address
    unsigned short  ways;       //option ways, 1 for all channel, 2 for left channel, 3 for right channel
    unsigned int  result;     //option result: [0:success,1:failure]
    void* payload;
}pwrMsgHead_t;

typedef struct  smsghead
{
    BYTE cmd;
    BYTE type;
    WORD16 len;
} SMsgHead;

typedef struct __FPGA_REG__
{
	WORD16 ofset;
	WORD16 value;
#define MAX_OFFSET 0x40
} fpgaRegStr;

typedef struct _BY_PTN_PWR_INFO_
{
    unsigned char ratio;	//It is used to the electricity,the truly electricity = ratio*feedback
    short VDD;
    short VDDh;
    short VDDl;
    int iVDD;
    int iVDDh;
    int iVDDl;

    short VDDIO;
    short VDDIOh;
    short VDDIOl;
    int iVDDIO;
    int iVDDIOh;
    int iVDDIOl;

    short ELVDD;
    short ELVDDh;
    short ELVDDl;
    int iELVDD;
    int iELVDDh;
    int iELVDDl;

    short ELVSS;
    short ELVSSh;
    short ELVSSl;
    int iELVSS;
    int iELVSSh;
    int iELVSSl;

	unsigned short VBL;
	short iVBL;
	unsigned char vbl_led_p_open_short;
	unsigned char vbl_led_n_open_short[6];
	unsigned char vbl_reserved[7];
	
    short VSP;
    short VSPh;
    short VSPl;
    int iVSP;
    int iVSPh;
    int iVSPl;

    short VSN;
    short VSNh;
    short VSNl;
    int iVSN;
    int iVSNh;
    int iVSNl;

    short MTP;
    short MTPh;
    short MTPl;
    int iMTP;
    int iMTPh;
    int iMTPl;
}sByPtnPwrInfo;

typedef struct tag_main_power_info
{
	int sender;		// client; box;
	int channel;
	sByPtnPwrInfo main_power;
	short vcom;
	unsigned short ivcom;
}main_power_info_t;

typedef struct tag_s1103PwrCfgDb_s
{
    short VDD;
    short VDDFlyTime;
    short VDDOverLimit;
    short VDDUnderLimit;
    int VDDCurrentOverLimit;
    int VDDCurrentUnderLimit;
    short VDDOpenDelay;
    short VDDCloseDelay;

    short VDDIO;
    short VDDIOFlyTime;
    short VDDIOOverLimit;
    short VDDIOUnderLimit;
    int VDDIOCurrentOverLimit;
    int VDDIOCurrentUnderLimit;
    short VDDIOOpenDelay;
    short VDDIOCloseDelay;

    short ELVDD;
    short ELVDDFlyTime;
    short ELVDDOverLimit;
    short ELVDDUnderLimit;
    int ELVDDCurrentOverLimit;
    int ELVDDCurrentUnderLimit;
    short ELVDDOpenDelay;
    short ELVDDCloseDelay;

    short ELVSS;
    short ELVSSFlyTime;
    short ELVSSOverLimit;
    short ELVSSUnderLimit;
    int ELVSSCurrentOverLimit;
    int ELVSSCurrentUnderLimit;
    short ELVSSOpenDelay;
    short ELVSSCloseDelay;

    short VBL;
    short VBLFlyTime;
    short VBLOverLimit;
    short VBLUnderLimit;
    int VBLCurrentOverLimit;
    int VBLCurrentUnderLimit;
    short VBLOpenDelay;
    short VBLCloseDelay;

    short VSP;
    short VSPFlyTime;
    short VSPOverLimit;
    short VSPUnderLimit;
    int VSPCurrentOverLimit;
    int VSPCurrentUnderLimit;
    short VSPOpenDelay;
    short VSPCloseDelay;

    short VSN;
    short VSNFlyTime;
    short VSNOverLimit;
    short VSNUnderLimit;
    int VSNCurrentOverLimit;
    int VSNCurrentUnderLimit;
    short VSNOpenDelay;
    short VSNCloseDelay;

    short MTP;
    short MTPFlyTime;
    short MTPOverLimit;
    short MTPUnderLimit;
    int MTPCurrentOverLimit;
    int MTPCurrentUnderLimit;
    short MTPOpenDelay;
    short MTPCloseDelay;

    short LEDEnable;//signalType;		//// ttl lvds类型，有的需要在电源板配置
    short LEDChannel;
    short LEDNumber;
    short LEDCurrent;
    short LEDOVP;
    short LEDUVP;

    short signalOpenDelay;
    short signalCloseDelay;

    short pwmOpenDelay;
    short pwmCloseDelay;
    short pwm;
    short pwmDuty;
    short pwmFreq;

    short vdimOpenDelay;
    short vdimCloseDelay;
    short vdim;

    short invertOpenDelay;
    short invertCloseDelay;
    short invert;

	// gpio seq
    short udOpenDelay;
    short udCloseDelay;

    short lrOpenDelay;
    short lrCloseDelay;

    short modeOpenDelay;
    short modeCloseDelay;

    short stbyOpenDelay;
    short stbyCloseDelay;

    short rstOpenDelay;
    short rstCloseDelay;
}s1103PwrCfgDb;

typedef struct tag_power_cfg_info
{
	s1103PwrCfgDb power_module;
	// VCOM
	short vcom_set_volt;
    short vcom_max_volt;
    short vcom_min_volt;
    short vcom_max_current;
	// Back light power mode;
	unsigned char bl_power_mode;	// 0: const volt; 1: const current.
}power_cfg_info_t;

typedef struct _PWR_VDD_VBL_SET
{
    unsigned char  flag;
    unsigned char  set;
    unsigned short value[2];
}PwrVddVblSet;
#endif

#pragma pack(pop)


#endif

