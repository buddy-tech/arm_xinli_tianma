#ifndef PACKSOCKET_H
#define PACKSOCKET_H

#define SYNC_HEAD  0X4747
#define PACKAGE_MAX_LEN   1600  //don't more 2048,because the conn->tempbuf len is 2048
#define FIFO_MAX_LEN   40960

#pragma pack(push)
#pragma pack(1)

typedef struct tag_socket_cmd_head_info_s
{
    unsigned short sync;
    unsigned short cmd;
    unsigned int   ipnum;
    unsigned short curNo;
    unsigned short lastNo;
    unsigned short type; //json ,bin,
    unsigned short length;
}socket_cmd_head_info_t;

typedef struct tag_socket_cmd_info_s
{
    unsigned short sync;
    unsigned short cmd;
    unsigned int   ipnum;
    unsigned short curNo;
    unsigned short lastNo;
    unsigned short type; //json ,bin,
    unsigned short length;
    unsigned char  *pValid;
    unsigned int   crc;
}socket_cmd_info_t;

#pragma pack(pop)

typedef struct tag_socket_cmd_list_s
{
    socket_cmd_info_t info;
    struct tag_socket_cmd_list_s *next;
}socket_cmd_list_t;

typedef struct tag_socket_cmd_list_set_s
{
    socket_cmd_list_t *psockCmdListHead;
}socket_cmd_list_set_t;

typedef struct tag_socket_cmd_s
{
    unsigned short cmd;
    unsigned short type;
    unsigned int   ipnum;
    unsigned int   len;
    unsigned char  *pcmd;
}socket_cmd_t;


typedef struct tag_cmd_fifo_s
{
    unsigned char *cmd;
    unsigned int  read;
    unsigned int  write;
    unsigned int  len;
    unsigned int  maxLen;
    unsigned int  bInit;
}cmd_fifo_t;

typedef struct tag_socket_cmd_class_s
{
    socket_cmd_list_set_t  gstRecvSocketCmdList;
    cmd_fifo_t             cmd_fifo;
    int (*getPackageMaxLen)(void);
    int (*sendSocketCmd)(int sendFd,unsigned int typeCmd,unsigned int ipnum,unsigned short curNo,unsigned short lastNo,unsigned char *pData,unsigned int len);
    void (*recvSocketCmd)(void *pSock_cmd_class,unsigned char *pData,int len);
    socket_cmd_t *(*parseSocketCmd)(void *pSock_cmd_class);
    int   sendPackageLen;
    int   recvFifoLen;
    //pthread_mutex_t	mutex;
}socket_cmd_class_t;

void init_socket_cmd_class(socket_cmd_class_t *pSocket_cmd_class,int sendpackageLen,int recvFifoLen);

#endif // PACKSOCKET_H
