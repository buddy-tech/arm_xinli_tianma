#ifndef _TM_MES_H_
#define _TM_MES_H_

typedef struct tag_tm_mes_packet_header
{
    unsigned short sync;
    unsigned short cmd;
    unsigned int   ipnum;
    unsigned short curNo;
    unsigned short lastNo;
    unsigned short packet_type; 	//json ,bin,
    unsigned short data_len;
}tm_mes_packet_header_t;

#define MAX_MES_MSG_BUF_LEN			(1024 * 4)


enum
{
	E_TM_MES_MSG_POWER_ON_REQ = 0x0001,
	E_TM_MES_MSG_POWER_ON_ACK,

	E_TM_MES_MSG_POWER_OFF_REQ,
	E_TM_MES_MSG_POWER_OFF_ACK,

	E_TM_MES_MSG_KEY_UP_REQ,
	E_TM_MES_MSG_KEY_UP_ACK,

	E_TM_MES_MSG_KEY_DOWN_REQ,
	E_TM_MES_MSG_KEY_DOWN_ACK,
};

#endif


