#ifndef _COM_INTERFACE_H_
#define _COM_INTERFACE_H_

#include "comStruct.h"

#undef MAX
#define MAX(x,y) ((x) > (y) ? (x) : (y))


#define DEV_TYPE_OFFLINE 0x01
#define DEV_TYPE_TAICHE  0x02

#define MAX_MSG_LENGTH   1024    /*接收socket的缓存长度*/

#define MAX_SOCKET_NUM      0x02    /*支持最大socket的个数*/

#define UDP_DIR_RECV        0x01
#define UDP_DIR_SEND        0x02

#pragma   pack(push)

#pragma   pack(1)

typedef struct sMsgResult
{
    WORD16 nLen;
    INT result;
}SMsgResult;

typedef struct ID_Info
{
    WORD16   rockid;
    WORD16   pgid;
    BYTE   pgnum;
    BYTE   reserv[15];
    BYTE   timepack[16];
} tIdInfo;


#pragma   pack(pop)

INT ifnetInit(void);
INT ifnetDataSend(BYTE *pBuf, INT len, void *pNode);
INT ifnetUdpSendData(BYTE *pBuf, INT len, void *pNode);
INT ifnetDataSendResult32(BYTE ucCmd, BYTE ucType, INT nResult, void *pNode);
INT ifnetDataSendCmdBuf(BYTE ucCmd, BYTE ucType, WORD16 wLen, BYTE *pBuf, void *pNode);
void ifnetSetComDevNode(ComDeviceIf *pNode);
ComDeviceIf * ifnetGetComDevNode();

INT ifnetSendData(CHAR *pBuf, INT len, void *pNode);
INT ifnetSend(CHAR *buf, INT size, INT sock);


INT listen_socket(INT listen_port);
INT ifnetTcpRev(void);

INT ifnetWriteData(INT *pfd,char *buf,int len);

#endif
