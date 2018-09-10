#ifndef COMUART_H_
#define COMUART_H_

#include "common.h"
#include "comStruct.h"


#pragma   pack(push)
#pragma   pack(1)

#define UART_BUF_SIZE 1100
#define MAX_BUFF_SIZE 2048

#define MAX_UART_COUNT 2
#define MAX_ROLE_NAME 10
#define NAME_LEN 16

#define LCD_UART    2
#define PWR_UART	1
#define BOX_UART    0

typedef enum _UART_STATE_{
	UART_IDLE, 
	UART_RECV_BEGIN, 
	UART_RECV_LENGTH,
	UART_RECV_HEAD,
	UART_RECV_DATA,
	UART_RECV_END,
	UART_RECV_BUSY
}EUART_STATA;

typedef struct __UART_BUF__
{
	unsigned long  uartrecvcnt ;
    unsigned long  uartleftdatalen ;
	BYTE  uartcatch[UART_BUF_SIZE]; // 串口缓存
	BYTE  uartbuff[MAX_BUFF_SIZE] ; // 串口剩余数据buff
	BYTE  *curuartptr ;
	BYTE  uartstate ;
	BYTE  uartdo ;
	BYTE  uartbytecnt ;
}T_UART_BUF;

typedef struct _S_UART_INFO__
{
    INT fd;					   // 设备描述符
    BYTE uartName[NAME_LEN];   // 串口tty名
    INT serial ;			   // 串口序列号，0<= serial <=MAX_UART_COUNT
	T_UART_BUF UartBuf;	// 串口缓冲
}S_UART_INFO;


#pragma pack(pop)

void uartInit();
INT uartClearData (INT serialNo);  // 根据串口号，清空串口
INT uartWriteData (INT serialNo, void *pBuf, INT writeLen); // 串口写函数
INT uartWriteDataNoCrc(INT serialno , CHAR *pBuf , INT writeLen);
INT uartReadData (INT serialNo, struct timeval timeout, BYTE *pOutBuf); // 串口写函数
INT uartSendInterface(char *pBuf ,INT len ,void *pNode); // 串口回复上层注册函数
INT uartSendMsgToBoard(INT serialNo, void *pBuf, WORD16 nLen);


INT uartSendMsgToBoard(INT serialNo, void *pBuf, WORD16 nLen);
INT uartWaitRecvData(INT nSerialNo, BYTE ucCmd, struct timeval delaytime, BYTE aucreadbuf[], INT *nBufLen);
INT uartProcNormalCmd(INT nSerialNo, struct timeval delaytime, BYTE cmd, void *pBuf, WORD16 nLen, void *pNode);
INT uartProcNormalCmdNoWait(INT nSerialNo,  BYTE cmd, void *pBuf, WORD16 nLen);


int FindSeiralInUartInfo(int nSerialNo); // 串口号，找到对应的索引号

int getPwrUartNo();   // 获取PWR的串口号
int getBoxUartNo();

int uartGetSerialfd(int nsserialNo); //获取串口句柄

int  openUart(int nIndex);
void closeUart(int nIndex);

INT uartOpenAndSetData(BYTE *devName,int baud,int databit,int stopbit,char ecc);
#endif

