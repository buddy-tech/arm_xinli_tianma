#include "packsocket.h"
#include "util/util.h"
#include "util/debug.h"

static void int2buffer(unsigned char* pBuffer, int data)
{
    char intdata[4];
    intdata[0] = data>>24;
    intdata[1] = (data>>16) & 0xFF;
    intdata[2] = (data>>8) & 0xFF;
    intdata[3] = data & 0xFF;
    memcpy(pBuffer,intdata,sizeof(intdata));
}

static void short2buffer(unsigned char* pBuffer, short data)
{
    char shortdata[2];
    shortdata[0] = (data>>8) & 0xFF;
    shortdata[1] = data & 0xFF;
    memcpy(pBuffer,shortdata,sizeof(shortdata));
}

int getPackageMaxLen()
{
    return PACKAGE_MAX_LEN;
}

int sendSocketCmd(int sendFd,unsigned int typeCmd,unsigned int ipnum,unsigned short curNo,unsigned short lastNo,unsigned char *pData,unsigned int len)
{
        if(sendFd == -1)
        {
        	//printf("sendSocketCmd error: Invalid socket fd = %d.\n", sendFd);
            return -1;
        }
		
        int sendFlag = 0;
        socket_cmd_info_t *pSocketCmdInfo = (socket_cmd_info_t*)malloc(sizeof(socket_cmd_info_t));
        pSocketCmdInfo->sync  = SYNC_HEAD;
        pSocketCmdInfo->cmd   =  typeCmd&0xFFFF;
        pSocketCmdInfo->curNo = curNo;
        pSocketCmdInfo->lastNo = lastNo;
        pSocketCmdInfo->ipnum = ipnum;
        pSocketCmdInfo->type    = typeCmd>>16;
		
        if(len == 0)
        {            
            pSocketCmdInfo->length = 0;
            pSocketCmdInfo->pValid = 0;
            pSocketCmdInfo->crc = 0;
        }
        else
        {
            pSocketCmdInfo->length = len;
            pSocketCmdInfo->pValid = (unsigned char*)malloc(len);
            memcpy(pSocketCmdInfo->pValid,pData,len);
            pSocketCmdInfo->crc = crc32_le(CRCPOLY_LE,pSocketCmdInfo->pValid,pSocketCmdInfo->length);
        }
		
         unsigned char *pOrgSocketCmd = (unsigned char*)malloc(sizeof(socket_cmd_head_info_t)+pSocketCmdInfo->length+4);
         unsigned char *pSocketCmd = pOrgSocketCmd;
         short2buffer(pSocketCmd,pSocketCmdInfo->sync);
         pSocketCmd+=2;
         short2buffer(pSocketCmd,pSocketCmdInfo->cmd);
         pSocketCmd+=2;
         int2buffer(pSocketCmd,pSocketCmdInfo->ipnum);
         pSocketCmd+=4;
         short2buffer(pSocketCmd,pSocketCmdInfo->curNo);
         pSocketCmd+=2;
         short2buffer(pSocketCmd,pSocketCmdInfo->lastNo);
         pSocketCmd+=2;
         short2buffer(pSocketCmd,pSocketCmdInfo->type);
         pSocketCmd+=2;
         short2buffer(pSocketCmd,pSocketCmdInfo->length);
         pSocketCmd+=2;
		 
         if(pSocketCmdInfo->length)
         {
            memcpy(pSocketCmd,pSocketCmdInfo->pValid,pSocketCmdInfo->length);
            pSocketCmd+=pSocketCmdInfo->length;
            free(pSocketCmdInfo->pValid);
         }
		 
         int2buffer(pSocketCmd,pSocketCmdInfo->crc);
         int ret = send(sendFd,pOrgSocketCmd,sizeof(socket_cmd_head_info_t)+pSocketCmdInfo->length+4,0);
         if(ret <= 0)
         {
             sendFlag = -1;
             printf("sendSocketCmd error: ret is %d, cmd:%d\n", ret, typeCmd);
         }
		 
         free(pOrgSocketCmd);
         free(pSocketCmdInfo);
		 
         return sendFlag;
}


static void _initRecvCmdList(void *pSock_cmd_class)
{
    socket_cmd_class_t *pSocket_cmd_class = pSock_cmd_class;

    pSocket_cmd_class->gstRecvSocketCmdList.psockCmdListHead = 0;
}

static void _addRecvList(void *pSock_cmd_class,socket_cmd_list_t *pSocketList)
{
    socket_cmd_class_t *pSocket_cmd_class = pSock_cmd_class;
    socket_cmd_list_t *psockCmdListRecvHead = pSocket_cmd_class->gstRecvSocketCmdList.psockCmdListHead;
    if(!psockCmdListRecvHead)
    {
        pSocket_cmd_class->gstRecvSocketCmdList.psockCmdListHead = pSocketList;
    }
    else
    {
        socket_cmd_list_t *temp = psockCmdListRecvHead;
        socket_cmd_list_t *prev;
        while(temp)
        {
            prev = temp;
            temp = temp->next;
        }
        prev->next = pSocketList;
    }
}


static int _checkRecvList2(void *pSock_cmd_class,int cmd)
{
    socket_cmd_class_t *pSocket_cmd_class = pSock_cmd_class;
    socket_cmd_list_t  *pSocketList = pSocket_cmd_class->gstRecvSocketCmdList.psockCmdListHead;
    int totalnum = -1;
    int num = 0;
    while(pSocketList)
    {
        if(pSocketList->info.cmd == cmd)
        { 
           // DBG("Cmd %x %x %x",pSocketList->info.cmd, pSocketList->info.curNo,pSocketList->info.lastNo);
            num++;
            if(totalnum == -1)
            {
                totalnum = pSocketList->info.lastNo+1;
            }
        }
        else
        {
            //DBG("Cmd %x %x",pSocketList->info.cmd, cmd);
        }
		
        if(num == totalnum)
        {
            return 1;
        }
		
        pSocketList = pSocketList->next;
    }
	
    return 0;
}

static void _destoryRecvList(void *pSock_cmd_class)
{
    socket_cmd_class_t *pSocket_cmd_class = pSock_cmd_class;
    socket_cmd_list_set_t *psocket_cmd_list_set = &pSocket_cmd_class->gstRecvSocketCmdList;

    socket_cmd_list_t *pTmpSocketList = psocket_cmd_list_set->psockCmdListHead;
    socket_cmd_list_t *pPrvSocketList;
    while(pTmpSocketList)
    {
        pPrvSocketList = pTmpSocketList;
        pTmpSocketList = pTmpSocketList->next;
        if(pPrvSocketList->info.pValid)
        {
            free(pPrvSocketList->info.pValid);
        }
        free(pPrvSocketList);
    }
    psocket_cmd_list_set->psockCmdListHead = 0;
}

static void _destoryRecvList2(void *pSock_cmd_class,int cmd)
{
    socket_cmd_class_t    *pSocket_cmd_class = pSock_cmd_class;
    socket_cmd_list_set_t *psocket_cmd_list_set = &pSocket_cmd_class->gstRecvSocketCmdList;

    socket_cmd_list_t *pTmpSocketList = psocket_cmd_list_set->psockCmdListHead;
    socket_cmd_list_t *pPrvSocketList = NULL;
    socket_cmd_list_t *pFreeSocketList= NULL;
    while(pTmpSocketList)
    {
        if(pTmpSocketList->info.cmd == cmd)
        {
            //如果是头节点
            if(!pPrvSocketList)
            {
                psocket_cmd_list_set->psockCmdListHead = pTmpSocketList->next;
            }
            //如果是尾节点
            if(pTmpSocketList->next == NULL)
            {
                if(pPrvSocketList)
                {
                    pPrvSocketList->next = NULL;
                }
            }
            pFreeSocketList = pTmpSocketList;
        }
        else
        {
            pPrvSocketList = pTmpSocketList;
        }
        pTmpSocketList = pTmpSocketList->next;
        if(pFreeSocketList)
        {
            if(pFreeSocketList->info.pValid)
            {
                free(pFreeSocketList->info.pValid);
            }
            free(pFreeSocketList);
            pFreeSocketList = NULL;
        }
    }
}

static void _addcmd(socket_cmd_t  *psocket_cmd,unsigned char *pcmd,int len)
{
    int tmpLen = psocket_cmd->len + len;
    unsigned char *pCmdTmp = malloc(tmpLen);
    memcpy(pCmdTmp,psocket_cmd->pcmd,psocket_cmd->len);
    memcpy(&pCmdTmp[psocket_cmd->len],pcmd,len);
    free(psocket_cmd->pcmd);
    psocket_cmd->pcmd = pCmdTmp;
    psocket_cmd->len  = tmpLen;
}

static cmd_fifo_t *init_cmd_fifo(void *pSock_cmd_class)
{
    socket_cmd_class_t *pSocket_cmd_class = pSock_cmd_class;
    cmd_fifo_t *pCmd_fifo = &pSocket_cmd_class->cmd_fifo;
    if(pCmd_fifo->bInit!=1)
    {
        pCmd_fifo->len = 0;
        pCmd_fifo->maxLen = FIFO_MAX_LEN;
        pCmd_fifo->read  = pCmd_fifo->write = 0;
        pCmd_fifo->bInit = 1;
        _initRecvCmdList(pSock_cmd_class);
    }
    return pCmd_fifo;
}


static int write_cmd_fifo(cmd_fifo_t *pCmd_fifo,unsigned char *pData,int len)
{
    if(pCmd_fifo->len + len > pCmd_fifo->maxLen)
        return -1;
    if(pCmd_fifo->write>=pCmd_fifo->read)
    {
        if((pCmd_fifo->write + len) < pCmd_fifo->maxLen)
        {
            memcpy(&pCmd_fifo->cmd[pCmd_fifo->write],pData,len);
            pCmd_fifo->write += len;
        }
        else
        {
            int len1 = pCmd_fifo->maxLen-pCmd_fifo->write;
            memcpy(&pCmd_fifo->cmd[pCmd_fifo->write],pData,len1);
            memcpy(pCmd_fifo->cmd,&pData[len1],len-len1);
            pCmd_fifo->write = len - len1;
        }
    }
    else
    {
        memcpy(&pCmd_fifo->cmd[pCmd_fifo->write],pData,len);
        pCmd_fifo->write += len;
    }
    pCmd_fifo->len += len;
    pCmd_fifo->write %= pCmd_fifo->maxLen;
    return 0;
}

static int read_cmd_try_read(cmd_fifo_t *pCmd_fifo,int *len)
{
    unsigned char cmd[32];
    unsigned short validLen;
    if(pCmd_fifo->len>=(sizeof(socket_cmd_head_info_t)+4))
    {
       int len1;
       len1 = pCmd_fifo->maxLen-pCmd_fifo->read;
       if(len1>sizeof(socket_cmd_head_info_t))
       {
           memcpy(cmd,&pCmd_fifo->cmd[pCmd_fifo->read],sizeof(socket_cmd_head_info_t));
       }
       else
       {
           memcpy(cmd,&pCmd_fifo->cmd[pCmd_fifo->read],len1);
           memcpy(&cmd[len1],pCmd_fifo->cmd,sizeof(socket_cmd_head_info_t)-len1);
       }
       if(((cmd[0]<<8)|cmd[1]) == SYNC_HEAD)
       {
           validLen = cmd[14]<<8 |cmd[15];
           if(pCmd_fifo->len>=sizeof(socket_cmd_head_info_t)+validLen+4)
           {
               *len = sizeof(socket_cmd_head_info_t)+validLen+4;
               return 0;
           }
       }
    }
    return -1;
}

static void read_cmd_fifo(cmd_fifo_t *pCmd_fifo,unsigned char *pBuffer,int len)
{
    if(pCmd_fifo->read+len > pCmd_fifo->maxLen)
    {
        int len1 = pCmd_fifo->maxLen - pCmd_fifo->read;
        memcpy(pBuffer,&pCmd_fifo->cmd[pCmd_fifo->read],len1);
        memcpy(&pBuffer[len1],pCmd_fifo->cmd,len-len1);
    }
    else
    {
        memcpy(pBuffer,&pCmd_fifo->cmd[pCmd_fifo->read],len);
    }
    pCmd_fifo->read += len;
    pCmd_fifo->read %= pCmd_fifo->maxLen;
    pCmd_fifo->len  -= len;
}

void recvSocketCmd(void *pSock_cmd_class,unsigned char *pData,int len)
{
    socket_cmd_class_t *pSocket_cmd_class = pSock_cmd_class;
    cmd_fifo_t *pCmd_fifo = init_cmd_fifo(pSocket_cmd_class);
    if(write_cmd_fifo(pCmd_fifo,pData,len) < 0)
    {
        printf("recvSocketCmd: buffer too small!:%d %d\n", pCmd_fifo->len, len);
        return;
    }
}

socket_cmd_t *parseSocketCmd(void *pSock_cmd_class)
{
    socket_cmd_class_t *pSocket_cmd_class = pSock_cmd_class;
    int readLen = 0;
	int ret = 0;
    cmd_fifo_t *pCmd_fifo = init_cmd_fifo(pSocket_cmd_class);
	
    while(1)
    {
        ret = read_cmd_try_read(pCmd_fifo,&readLen);
        if(ret < 0)
        {
        	//printf("parseSocketCmd error: read_cmd_try_read error!\n");
            return NULL;
        }
		
        unsigned char *pBuffer = (unsigned char*)malloc(readLen);
        read_cmd_fifo(pCmd_fifo, pBuffer, readLen);

        unsigned short cmd    = pBuffer[2]<<8|pBuffer[3];
        unsigned int   ipnum  = pBuffer[4]<<24 | pBuffer[5]<<16 | pBuffer[6]<<8 |pBuffer[7];
        unsigned short curNo  = pBuffer[8]<<8  | pBuffer[9];
        unsigned short lastNo = pBuffer[10]<<8 | pBuffer[11];
        unsigned short type = pBuffer[12]<<8 |pBuffer[13];
        unsigned short validLen = pBuffer[14]<<8 |pBuffer[15];
        unsigned int crc = (pBuffer[validLen+16] <<24) | (pBuffer[validLen+17]<<16)
							| (pBuffer[validLen+18]<<8)| (pBuffer[validLen+19] );
        if( (crc == 0)
			|| (crc == crc32_le(CRCPOLY_LE, &pBuffer[sizeof(socket_cmd_head_info_t)], validLen) ) )
        {
            socket_cmd_list_t *pSocketList = malloc(sizeof(socket_cmd_list_t));
            pSocketList->next = 0;
			
            socket_cmd_info_t *pSocket_cmd_info = &pSocketList->info;
            memset(pSocket_cmd_info,0,sizeof(socket_cmd_info_t));
            pSocket_cmd_info->sync  = SYNC_HEAD;
            pSocket_cmd_info->cmd   = cmd;
            pSocket_cmd_info->ipnum = ipnum;
            pSocket_cmd_info->curNo = curNo;
            pSocket_cmd_info->lastNo = lastNo;
            pSocket_cmd_info->length = validLen;
            pSocket_cmd_info->type = type;
			
            if(validLen > 0)
            {
                pSocket_cmd_info->pValid = (unsigned char*)malloc(validLen);
                memcpy(pSocket_cmd_info->pValid,&pBuffer[sizeof(socket_cmd_head_info_t)], validLen);
            }
			else
			{
				printf("parseSocketCmd: valid len = %ud.\n", validLen);
			}
			
            pSocket_cmd_info->crc = crc;
            _addRecvList(pSocket_cmd_class, pSocketList);
        }
        else
        {
            //crc error;
            printf("parseSocketCmd error: crc is error %d.\n", validLen);
        }
		
        free(pBuffer);
		
        //if(_checkRecvList(pSocket_cmd_class))
        if(_checkRecvList2(pSocket_cmd_class,cmd))
        {
            socket_cmd_t  *psocket_cmd = (socket_cmd_t*)malloc(sizeof(socket_cmd_t));
            psocket_cmd->cmd = cmd;
            psocket_cmd->ipnum = ipnum;
            psocket_cmd->len  = 0;
            psocket_cmd->pcmd = 0;
            psocket_cmd->type = type;
			
            socket_cmd_list_t *pTmpSocketList = pSocket_cmd_class->gstRecvSocketCmdList.psockCmdListHead;
            while(pTmpSocketList)
            {
                //if(pTmpSocketList->info.length>0)
                if((pTmpSocketList->info.length>0)&&(pTmpSocketList->info.cmd == cmd))
                {
                    _addcmd(psocket_cmd,pTmpSocketList->info.pValid,pTmpSocketList->info.length);
                }
				
                pTmpSocketList = pTmpSocketList->next;
            }
			
            _destoryRecvList2(pSocket_cmd_class,cmd);
            return psocket_cmd;
        }
    }
	
    return NULL;
}

void init_socket_cmd_class(socket_cmd_class_t *pSocket_cmd_class, int sendpackageLen, int recvFifoLen)
{
    pSocket_cmd_class->getPackageMaxLen = getPackageMaxLen;
    pSocket_cmd_class->sendSocketCmd  =  sendSocketCmd;
    pSocket_cmd_class->recvSocketCmd  =  recvSocketCmd;
    pSocket_cmd_class->parseSocketCmd = parseSocketCmd;
	
    pSocket_cmd_class->sendPackageLen = sendpackageLen;
    pSocket_cmd_class->recvFifoLen    = recvFifoLen;
	
    pSocket_cmd_class->cmd_fifo.cmd = (unsigned char*)malloc(recvFifoLen);
}
