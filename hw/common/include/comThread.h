
#if 0
#ifndef __COM_THREAD__H
#define __COM_THREAD__H
#ifndef __WIN32__
#include <sys/msg.h>
#endif
/***************************************************************************************/
/*                             应用进程类型起始值                              */
/***************************************************************************************/

typedef INT (*threadFun)(BYTE *pucBuf, WORD16 wLen, void *pNode);


#define PROCTYPE_DIVAPP_BEGIN           (WORD16)0x2000


#define MSG_RECV_FROM_UART    0x01    /*从串口接收到的消息*/
#define MSG_RECV_FROM_UDP     0x02    /*从UDP接收到的消息*/
#define MSG_RECV_FROM_TCP     0x03    /*从TCP接收到的消息*/


#define THREAD_NAME_LENGTH    32      /* 线程名字符串长度*/

#define MAX_QUEUE_NUM         64      /*最大队列数目*/

#define NOUSED                 0x00
#define USED                   0x01

#pragma   pack(1)

/***********************************************************
 *             线程属性表项结构                  *
***********************************************************/
/*消息处理的结构*/
typedef struct msgtype
{
	INT mtype;					/*线程接收消息类型*/
	BYTE *buffer;				 /*消息内容*/
	WORD16 wLen;				 /*消息长度*/
	BYTE ucIfNetType;			 /*ifnet类型，包括UDP.TCP,UARTS*/
	INT fd; 					 /*UDP socket ID*/
	struct sockaddr_in	cliaddr; /*UDP client 地址*/
} MSGTYPE;

/*消息处理的结构*/
typedef struct innermsgtype
{
	long mtype; 		   /*线程接收消息类型*/
	WORD16 wLen;		   /*消息长度*/
	CHAR *buffer;		   /*消息内容*/
} InnerMsgType;


typedef struct tagT_MsgQueueId
{
	BYTE ucIsUse;                       /*使用标志位*/
	WORD16 wCmdType;                     /*thread处理的命令类型*/
	INT MsgQueueId;                       /*消息队列ID*/
}T_MsgQueueId;

typedef struct tagT_CmdEntry
{
	WORD16 wCmdType;                     /*命令类型*/
	threadFun pEntry;                    /*命令类型对应的处理函数*/
}T_CMDENTRY;

typedef struct tatT_CmdTypeShareQueue
{
	WORD16 wShareCmdType;       /*共享队列的命令类型*/
	WORD16 wCmdType;            /*需要共享队列的命令类型*/
	threadFun pEntry;           /*命令类型对应的处理函数*/
}T_CmdTypeShareQueue;

#pragma   pack()


/***********************************************************
 *            定义函数指针                  *
***********************************************************/

/***********************************************************
 *             函数接口定义                  *
***********************************************************/


/*初始化函数，创建线程*/
INT createThreadQueue(WORD16 wCmdType, threadFun pEntry);
/*对接收到的消息进行调度，发到相应的处理模块*/
BOOLEAN isThreadProcMsgType(WORD16 ucCmdType);
/*对接收到的消息进行调度，发到相应的处理模块*/
INT dispatchMsg(MSGTYPE *pMsg);
/*线程内部消息发送接口，不涉及对PC的应答*/
INT sendInnerMsgToThread(BYTE ucCmdType, BYTE *pucBuf, WORD16 wLen);
INT threadInit();
INT simSendMsgToThread(BYTE ucCmdType, BYTE *pucBuf, WORD16 wLen, void *pNode);

#endif

#endif
