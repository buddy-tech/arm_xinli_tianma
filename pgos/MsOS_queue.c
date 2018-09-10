#include "MsOS.h"

#define MAX_NUM_OF_MSGS	0x100
#define MAX_SIZE_OF_MSG	0x100


typedef struct _tag_MsOS_MsgNode
{
	struct _tag_MsOS_MsgNode*	prev;
	struct _tag_MsOS_MsgNode*	next;
	MS_U32		msgRealSize;
	MS_U32		msg[1];
}MsOS_MsgNode;

typedef struct MsOS_queues {
	MS_U32 lockSemaphore;
	MS_U32 sendSemaphore;
	MS_U32 recvSemaphore;
	MS_U32 maxNumOfMsgs;
	MS_U32 maxSizeOfMsg;
	MsOS_MsgNode*		pMsgList;
	MsOS_MsgNode* 		pFreeHead;
	MsOS_MsgNode*		pUsedHead;
	MS_S32			fullTimes;

}MsOS_queue;


#define MsOS_MSG_NODE_HEAD_SIZE 	(sizeof(MsOS_MsgNode)-4)
#define	MsOS_MSG_SIZE(q)	     	(((q->maxSizeOfMsg +3) >> 2) << 2) 
#define MsOS_MSG_NODE_SIZE(q) 	((MS_U32)(MsOS_MSG_NODE_HEAD_SIZE+MsOS_MSG_SIZE(q)))
#define MsOS_MSG_QUEUE_SIZE(q)	(MsOS_MSG_NODE_SIZE(q)*q->maxNumOfMsgs)

static MsOS_MsgNode* _getNodeByIndex(MsOS_queue* pMsgq, MS_U32 curIndex)
{
	if(pMsgq && curIndex<pMsgq->maxNumOfMsgs)
	{
		return (MsOS_MsgNode*)((MS_U32)pMsgq->pMsgList+MsOS_MSG_NODE_SIZE(pMsgq)*curIndex);
	}
	return 0;
}
static void _AddToListTail(MsOS_queue* pMsgq, MS_BOOL bFree, MsOS_MsgNode* pNode)
{
	MsOS_MsgNode* pCur = 0;
	if(pMsgq && pNode)
	{
		if(bFree)
		{
			if(!pMsgq->pFreeHead)
			{
				pMsgq->pFreeHead = pNode;
				pNode->prev = pNode;
				pNode->next = 0;
			}
			else
			{
				pCur = pMsgq->pFreeHead;
			}
		}
		else
		{
			if(!pMsgq->pUsedHead)
			{
				pMsgq->pUsedHead = pNode;
				pNode->prev = pNode;
				pNode->next = 0;
			}
			else
			{
				pCur = pMsgq->pUsedHead;
			}
		}
		if(pCur)
		{
			pNode->prev = pCur->prev;
			pNode->next = 0;
			pCur->prev->next = pNode;
			pCur->prev = pNode;
		}
	}
}

static MsOS_MsgNode*  _RemoveFromListHead(MsOS_queue* pMsgq, MS_BOOL bFree)
{
	MsOS_MsgNode* p = 0;
	if(pMsgq)
	{
		MsOS_MsgNode* pCur = bFree ? pMsgq->pFreeHead : pMsgq->pUsedHead;
		p = pCur;
		if(pCur)
		{
			if(pCur->next)
			{
				pCur->next->prev = pCur->prev;
			}
			pCur = pCur->next;
		}
		if(bFree)
			pMsgq->pFreeHead = pCur;
		else
			pMsgq->pUsedHead = pCur;
	}
	return p;
}


static MsOS_MsgNode*  _GetFromListHead(MsOS_queue* pMsgq)
{
	MsOS_MsgNode* p = 0;
	if(pMsgq)
	{
		MsOS_MsgNode* pCur =  pMsgq->pUsedHead;
		p = pCur;
	}
	return p;
}


static MsOS_MsgNode* _getNodeByMsg(MsOS_queue* pMsgq, MS_VOID* pMsg)
{
	if(!pMsg || !pMsgq)
	{
		return 0;
	}
	if((MS_U32)pMsg < (MS_U32)pMsgq->pMsgList+MsOS_MSG_NODE_HEAD_SIZE)
	{
		return 0;
	}
	if((MS_U32)pMsg + MsOS_MSG_SIZE(pMsgq) > (MS_U32)pMsgq->pMsgList + MsOS_MSG_QUEUE_SIZE(pMsgq))
	{
		return 0;
	}
	if((((MS_U32)pMsg - MsOS_MSG_NODE_HEAD_SIZE - (MS_U32)pMsgq->pMsgList) % (MsOS_MSG_NODE_SIZE(pMsgq))) != 0)
	{
		return 0;
	}

	return (MsOS_MsgNode*)((MS_U32)pMsg - MsOS_MSG_NODE_HEAD_SIZE);
}

MS_U32 MsOS_CreateQueue (MS_U32         u32QueueSize,
                         MS_U32         u32MessageSize)

{
	MsOS_queue  *msgq;
	MS_U32 i;

	if( (!u32QueueSize) || (u32MessageSize > MAX_NUM_OF_MSGS) )
		return 0;
	if ( (!u32MessageSize) || (u32MessageSize > MAX_SIZE_OF_MSG) )
		return 0;
        msgq = (MsOS_queue *)MsOS_AllocateMemory(sizeof(MsOS_queue), 0);
	if( !msgq )
		return 0;

	memset(msgq,0,sizeof(MsOS_queue));
	msgq->maxNumOfMsgs = u32QueueSize;
	msgq->maxSizeOfMsg = u32MessageSize;

	msgq->pMsgList = (MsOS_MsgNode *)MsOS_AllocateMemory(MsOS_MSG_QUEUE_SIZE(msgq),0);
	memset(msgq->pMsgList , 0, MsOS_MSG_QUEUE_SIZE(msgq));
	for(i=0;i<u32QueueSize;i++)
	{
		_AddToListTail(msgq,1,_getNodeByIndex(msgq,i));
	}
    msgq->lockSemaphore = MsOS_CreateSemaphore (1);
    msgq->sendSemaphore = MsOS_CreateSemaphore(msgq->maxNumOfMsgs);
    msgq->recvSemaphore = MsOS_CreateSemaphore(0);
	return msgq;
}


static MsOS_MsgNode * QueueClaim(MsOS_queue * msgq,MS_U32 u32WaitMs)
{

	MsOS_MsgNode* pNode = 0;
	if( (!msgq))
	{
		return 0;
	}
	if(MsOS_ObtainSemaphore(msgq->sendSemaphore,u32WaitMs)==0)
	{
		return 0;
	}	
	MsOS_ObtainSemaphore(msgq->lockSemaphore,0xffffffff);
	pNode = _RemoveFromListHead(msgq,1);
	MsOS_ReleaseSemaphore(msgq->lockSemaphore);

	return pNode;
}

static MS_BOOL QueueSend(MsOS_queue * msgq, MsOS_MsgNode* pNode)
{
	if( (!msgq) || (!pNode)  )
	{
		return 0;
	}
	//pNode = _getNodeByMsg(msgq,msg);
	//if(!pNode)
	//	return 0;
	    MsOS_ObtainSemaphore(msgq->lockSemaphore,0xffffffff);
		_AddToListTail(msgq,0,pNode);
		MsOS_ReleaseSemaphore(msgq->lockSemaphore);
		MsOS_ReleaseSemaphore(msgq->recvSemaphore);

	return 1;
}

MS_BOOL MsOS_SendToQueue(MS_U32 u32QueueId, MS_U8 *pu8Message, MS_U32 u32Size, MS_U32 u32WaitMs)
{ 
	MsOS_MsgNode* pNode = 0;
	MsOS_queue* msgq=(MsOS_queue * )u32QueueId;
	pNode=QueueClaim(msgq,u32WaitMs);
	if(!pNode)
		return 0;

	if( msgq->maxSizeOfMsg >= u32Size)
	{               
		memcpy(pNode->msg,pu8Message,u32Size);
		pNode->msgRealSize=u32Size;
        //ms_print.pfun("pNode->msg:%s\n",pNode->msg);
	}
    else
	{
        ms_print.pfun("MsOS_SendToQueue error\r\n");
		//copy(pNode->msg,pu8Message,pQueue->maxSizeOfMsg);
		MsOS_ObtainSemaphore(msgq->lockSemaphore,0xffffffff);
		_AddToListTail(msgq,1,pNode);
        MsOS_ReleaseSemaphore(msgq->lockSemaphore);
		return 0;
    }
	if( QueueSend(msgq,pNode) == 0)
	{
        MsOS_ObtainSemaphore(msgq->lockSemaphore,0xffffffff);
		_AddToListTail(msgq,1,pNode);
        MsOS_ReleaseSemaphore(msgq->lockSemaphore);
		return 0;
	}
	return 1;
}



static MsOS_MsgNode* QueueRecv(MsOS_queue * msgq , MS_U32  ms)
{
	MsOS_MsgNode* pMsg = 0;
	if( (!msgq) )
	{
		return 0;
	}

	if(MsOS_ObtainSemaphore(msgq->recvSemaphore,ms)==0)
		return 0;	

	MsOS_ObtainSemaphore(msgq->lockSemaphore,0xffffffff);
	pMsg = _RemoveFromListHead(msgq,0);
	MsOS_ReleaseSemaphore(msgq->lockSemaphore);
	return pMsg;
}

static MS_BOOL QueueFree(MsOS_queue * msgq , MsOS_MsgNode* pNode)
{
	//MsOS_MsgNode* pNode = 0;
	if( (!msgq) || !pNode )
	{
		return 0;
	}
	//pNode = _getNodeByMsg(msgq,msg);
	//if(!pNode)
	//	return 0;
	MsOS_ObtainSemaphore(msgq->lockSemaphore,0xffffffff);
	_AddToListTail(msgq,1,pNode);
	MsOS_ReleaseSemaphore(msgq->lockSemaphore);
	MsOS_ReleaseSemaphore(msgq->sendSemaphore);

	return 1;
}

MS_BOOL MsOS_RecvFromQueue (MS_U32 u32QueueId, MS_U8 *pu8Message, MS_U32 u32IntendedSize, MS_U32 *pu32ActualSize, MS_U32 u32WaitMs)
{
	MsOS_MsgNode* pNode = 0;
	pNode=QueueRecv((MsOS_queue *)u32QueueId,u32WaitMs);

	if(!pNode)
		return 0;
	if(pNode->msgRealSize <= u32IntendedSize)
	{
		memcpy(pu8Message,pNode->msg,pNode->msgRealSize);
		*pu32ActualSize=pNode->msgRealSize;
        //ms_print.pfun("pu8Message:%s\n",pu8Message);
	}
	else
	{
		memcpy(pu8Message,pNode->msg,u32IntendedSize);
		*pu32ActualSize=u32IntendedSize;
	}
	
	QueueFree((MsOS_queue *)u32QueueId,pNode); 
	return 1;
}

MS_BOOL MsOS_PeekFromQueue (MS_U32 u32QueueId, MS_U8 *pu8Message, MS_U32 u32IntendedSize, MS_U32 *pu32ActualSize)
{
	MsOS_MsgNode* pNode = 0;
	pNode = _GetFromListHead((MsOS_queue *)u32QueueId);
	if(!pNode)
		return 0;
	if(pNode->msgRealSize <= u32IntendedSize)
	{
		memcpy(pu8Message,pNode->msg,pNode->msgRealSize);
		*pu32ActualSize=pNode->msgRealSize;
        //ms_print.pfun("pu8Message:%s\n",pu8Message);
	}
	else
	{
		memcpy(pu8Message,pNode->msg,u32IntendedSize);
		*pu32ActualSize=u32IntendedSize;
	}
	return 1;
}	
	
MS_BOOL MsOS_DeleteQueue (MS_U32 u32QueueId)
{
	MsOS_queue* msgq=(MsOS_queue * )u32QueueId;
	if( !msgq )
	{
		return 0;
	}
	
	if(msgq->pMsgList)
		MsOS_FreeMemory(msgq->pMsgList, 0);
  
	MsOS_ReleaseSemaphore(msgq->lockSemaphore);

	MsOS_ReleaseSemaphore(msgq->sendSemaphore);

	MsOS_ReleaseSemaphore(msgq->recvSemaphore);


	MsOS_FreeMemory(msgq,0);
	return 1;
}

MS_U32 MsOS_SendMessage(
    MS_U32 u32MsgQueueId,
    MS_U32 MessageID,
    MS_U32 Parameter1,
    MS_U32 Parameter2)
{
    MSOS_MESSAGE	Message;

    MS_U32 	Result = (MS_U32)-1;
    Message.MessageID	= MessageID;
    Message.Parameter1	= Parameter1;
    Message.Parameter2	= Parameter2;
    Message.Blocking 	= MS_TRUE;
    Message.pResult 	= &Result;
    Message.ReturnSemaphore = MsOS_CreateSemaphore(0);

    MsOS_SendToQueue(u32MsgQueueId, &Message, sizeof(MSOS_MESSAGE), -1);
    MsOS_ObtainSemaphore(Message.ReturnSemaphore,-1);
    MsOS_DeleteSemaphore(Message.ReturnSemaphore);

    return Result;
}

MS_U32 MsOS_PostMessage(
    MS_U32 u32MsgQueueId,
    MS_U32 MessageID,
    MS_U32 Parameter1,
    MS_U32 Parameter2)
{
    MSOS_MESSAGE	Message;

    Message.MessageID	= MessageID;
    Message.Parameter1	= Parameter1;
    Message.Parameter2	= Parameter2;
    Message.Blocking	= MS_FALSE;
    Message.pResult		= NULL;

    return MsOS_SendToQueue(u32MsgQueueId, &Message, sizeof(MSOS_MESSAGE), -1);
}

/* end message */
