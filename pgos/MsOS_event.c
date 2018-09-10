#include "MsOS.h"
#include "assert.h"

#define EVENT_NODE_NUM  32

typedef enum 
{
	MESSAGE_CLEAR_EVNET,
	MESSAGE_SET_EVENT,
	MESSAGE_WAIT_EVENT,
	MESSAGE_RELEASE_EVENT,
}EVENT_MESSAGE_ID;

typedef struct tag_EVENT_MESSAGE
{
	MS_U32			MessageID;
	MS_U32			Param1;
	MS_U32			Param2;
	MS_U32			SendFLag;
	MS_U32			Result;
	MS_U32			ReturnSemaphore;
}EVENT_MESSAGE;

typedef struct tag_Event_node_s
{
	MS_U32 eventSemaphore;
	MS_U32 requestEventFlag;
	MS_U32 requestEventMode;
	MS_U32 returnEventAddr;
}Event_node_t;

typedef struct _tag_MSOS_EVENT
{
    MS_U32  	eventSemaphore;
    char		eventName[128];
	MS_U32		eventFlag;
	MS_U32		eventDeleted;
	MS_U32		u32QueueId;
	MS_U32		u32SiTaskId;
	Event_node_t *pEvent_Container;
	MS_U32		iEvent_Container_Size;
}MSOS_event;

typedef struct _tag_MSOS_EVENT_Group_list
{
	MSOS_event *pNode;
	struct _tag_MSOS_EVENT_Group_list *pnext;
}MSOS_event_Group_list;

MSOS_event_Group_list *pGroup_list = 0;

void _AddEventGroupList(MSOS_event *pNode)
{
	MSOS_event_Group_list *pNewListnode,*pList = pGroup_list;
	pNewListnode = malloc(sizeof(MSOS_event_Group_list));	
	pNewListnode->pNode = pNode;
	pNewListnode->pnext  = 0;
	while(pList)
	{
		if(!pList->pnext)
			break; 
	}
	if(pList == pGroup_list)
	{
		pGroup_list = pNewListnode;
	}
	else
	{
		pList->pnext = pNewListnode;
	}
}

MS_S32  _ResumEventGroupList(char *pName)
{
	MSOS_event_Group_list *pList = pGroup_list;
	while(pList)
	{
		if(!strcmp(pName,pList->pNode->eventName))
		{
			if(pList->pNode->eventDeleted)
			{
				pList->pNode->eventDeleted = 0;
				return pList->pNode;
			}
		}
		pList = pList->pnext;
	}
	return 0;
}

int  _SuspendEventGroupList(char *pName)
{
	MSOS_event_Group_list *pList = pGroup_list;
	while(pList)
	{
		if(!strcmp(pName,pList->pNode->eventName))
		{
				pList->pNode->eventDeleted = 1;
				return 1;
		}
		pList = pList->pnext;
	}
	return 0;
}




static void _InitEventList(MSOS_event *pEvent)
{
	pEvent->iEvent_Container_Size = EVENT_NODE_NUM;
	pEvent->pEvent_Container = MsOS_AllocateMemory(sizeof(Event_node_t)*pEvent->iEvent_Container_Size, 0);
	memset(pEvent->pEvent_Container,0,sizeof(Event_node_t)*pEvent->iEvent_Container_Size);
}


static void _AddEventList(MSOS_event *pEvent,MS_U32 EventSemphore,MS_U32 ReqEventFlag,MS_U32 ReqEventMod,MS_U32 RetEventAddr)
{
	int i;
	for(i=0;i<pEvent->iEvent_Container_Size;i++)
	{
		if(pEvent->pEvent_Container[i].eventSemaphore == 0)
		{
			break;
		}
	}
	if(i>=pEvent->iEvent_Container_Size)
	{
		assert(0);
		//event list is full
	}
	pEvent->pEvent_Container[i].eventSemaphore  = EventSemphore;
	pEvent->pEvent_Container[i].requestEventFlag	  = ReqEventFlag;
	pEvent->pEvent_Container[i].requestEventMode= ReqEventMod;
	pEvent->pEvent_Container[i].returnEventAddr   = RetEventAddr;
}

static void _RemoveEventList(MSOS_event *pEvent,MS_U32 EventSemphore)
{
	int i;
	for(i=0;i<pEvent->iEvent_Container_Size;i++)
	{
		if(pEvent->pEvent_Container[i].eventSemaphore == EventSemphore)
		{
		        free((void *)pEvent->pEvent_Container[i].returnEventAddr);
		        MsOS_DeleteSemaphore(pEvent->pEvent_Container[i].eventSemaphore);
			pEvent->pEvent_Container[i].eventSemaphore  = 0;
			break;
		}
	}
}

static void _DisptchEventList(MSOS_event *pEvent,MS_U32 *pu32EventGroupFlag)
{
	int i;
	MS_U32 u32EventFlag = *pu32EventGroupFlag;
	for(i=0;i<pEvent->iEvent_Container_Size;i++)
	{
		if(pEvent->pEvent_Container[i].eventSemaphore)
		{
			switch(pEvent->pEvent_Container[i].requestEventMode)
			{
				case E_AND:
				if(pEvent->pEvent_Container[i].requestEventFlag == u32EventFlag)
				{
					*(MS_U32 *)pEvent->pEvent_Container[i].returnEventAddr = u32EventFlag;
					MsOS_ReleaseSemaphore(pEvent->pEvent_Container[i].eventSemaphore);
					pEvent->pEvent_Container[i].eventSemaphore = 0;
				}
				break;
				case E_OR:
				if(pEvent->pEvent_Container[i].requestEventFlag & u32EventFlag)
				{
					*(MS_U32 *)pEvent->pEvent_Container[i].returnEventAddr = u32EventFlag;
                    //ms_print.pfun("pEvent E_OR %s-%x\n",pEvent->eventName,pEvent->pEvent_Container[i].eventSemaphore);
					MsOS_ReleaseSemaphore(pEvent->pEvent_Container[i].eventSemaphore);
					pEvent->pEvent_Container[i].eventSemaphore = 0;
				}
				break;
				case E_AND_CLEAR:
				if(pEvent->pEvent_Container[i].requestEventFlag == u32EventFlag)
				{
					*(MS_U32 *)pEvent->pEvent_Container[i].returnEventAddr = u32EventFlag;
					MsOS_ReleaseSemaphore(pEvent->pEvent_Container[i].eventSemaphore);
					pEvent->pEvent_Container[i].eventSemaphore = 0;
					(*pu32EventGroupFlag)&=~u32EventFlag;
				}
				break;
				case E_OR_CLEAR:
				if(pEvent->pEvent_Container[i].requestEventFlag & u32EventFlag)
				{
					*(MS_U32 *)pEvent->pEvent_Container[i].returnEventAddr = u32EventFlag;
                    //ms_print.pfun("pEventGroup E_OR_CLEAR %s-%x\n",pEvent->eventName,pEvent->pEvent_Container[i].eventSemaphore);
					MsOS_ReleaseSemaphore(pEvent->pEvent_Container[i].eventSemaphore);
					pEvent->pEvent_Container[i].eventSemaphore = 0;
                    //ms_print.pfun("1111 %x\n",*pu32EventGroupFlag);
					(*pu32EventGroupFlag)&=~u32EventFlag;
                    //ms_print.pfun("2222 %x\n",*pu32EventGroupFlag);
				}
				break;
			}
		}
	}
}


static void   *MsOS_EventTask(void *param)
{
	MSOS_event *pEventGroup = param;
	MS_S32 u32QueueId = pEventGroup->u32QueueId;
	EVENT_MESSAGE  Event_Message;
	MS_S32 s32ActualSize;
	while(1)
	{
		MsOS_RecvFromQueue (u32QueueId, &Event_Message, sizeof(EVENT_MESSAGE), &s32ActualSize, -1);
		MsOS_ObtainSemaphore(pEventGroup->eventSemaphore,-1);

        //ms_print.pfun("MsOS_EventTask msgId %d\n",Event_Message.MessageID);
		switch(Event_Message.MessageID)
		{
			case MESSAGE_CLEAR_EVNET:
				pEventGroup->eventFlag&=~Event_Message.Param1;
			break;

			case MESSAGE_SET_EVENT:
				pEventGroup->eventFlag|= Event_Message.Param1;
			break;

			case MESSAGE_WAIT_EVENT:
			{
				_AddEventList(pEventGroup,\
							  Event_Message.ReturnSemaphore,\
							  Event_Message.Param1,\
							  Event_Message.Param2,\
							  Event_Message.Result);
			}
			break;

			case MESSAGE_RELEASE_EVENT:
			{
				_RemoveEventList(pEventGroup,Event_Message.ReturnSemaphore);
			}
			break;
		}
		_DisptchEventList(pEventGroup,&pEventGroup->eventFlag);
		MsOS_ReleaseSemaphore(pEventGroup->eventSemaphore);
	}
    return NULL;
}

MS_U32 MsOS_CreateEventGroup (char *pName)
{
	MSOS_event* pEvent;
	if(pEvent = _ResumEventGroupList(pName))
	{
        ms_print.pfun("!!!notice there is event group %s have been create!!!\n",pName);
		return pEvent;
	}
	else
	{
		pEvent=MsOS_AllocateMemory(sizeof(MSOS_event), 0);
		if(pEvent == 0)
			return 0;
        pEvent->eventSemaphore = MsOS_CreateSemaphore(1);
		if(pEvent->eventSemaphore < 0)
		{
			MsOS_FreeMemory(pEvent,0);
			return 0;
		}
		strcpy(pEvent->eventName,pName);
		pEvent->eventFlag=0;
		pEvent->eventDeleted=0;
    		// Create a queue to receive all events and interrupts.
        pEvent->u32QueueId = MsOS_CreateQueue(EVENT_NODE_NUM,
                                           sizeof(EVENT_MESSAGE));
		 _InitEventList(pEvent);
    		// Create A event Task
         pEvent->u32SiTaskId =MsOS_CreateTask((TaskEntry)MsOS_EventTask,(MS_U32)pEvent,E_TASK_PRI_MEDIUM,0,0,pName);

		_AddEventGroupList(pEvent);
		return (MS_U32)pEvent;
	}
}

MS_BOOL MsOS_DeleteEventGroup (MS_U32 u32EventGroupId)
{
	MSOS_event  *pEvent = (MSOS_event  *)u32EventGroupId;
	if(pEvent)
	{
		_SuspendEventGroupList(pEvent->eventName);
	}
    ms_print.pfun("!!!!!!!!!notice there is event group %s should be delete!!!\n",pEvent->eventName);
	return pEvent?TRUE:FALSE;
}

MS_BOOL MsOS_SetEvent (MS_U32 u32EventGroupId, MS_U32 u32EventFlag)
{
	MSOS_event  *pEvent = (MSOS_event  *)u32EventGroupId;
	if(!pEvent || pEvent->eventDeleted)
		return 0;
	EVENT_MESSAGE  Event_Message;
	Event_Message.MessageID = MESSAGE_SET_EVENT;
	Event_Message.Param1    = u32EventFlag;
	Event_Message.SendFLag = 0;
	return MsOS_SendToQueue(pEvent->u32QueueId, &Event_Message, sizeof(EVENT_MESSAGE), -1);
}

MS_BOOL MsOS_ClearEvent (MS_U32 u32EventGroupId, MS_U32 u32EventFlag)
{
	MSOS_event  *pEvent = (MSOS_event  *)u32EventGroupId;
	if(!pEvent || pEvent->eventDeleted)
		return 0;
	EVENT_MESSAGE  Event_Message;
	Event_Message.MessageID = MESSAGE_CLEAR_EVNET;
	Event_Message.Param1    = u32EventFlag;
	Event_Message.SendFLag = 0;
	return MsOS_SendToQueue(pEvent->u32QueueId, &Event_Message, sizeof(EVENT_MESSAGE), -1);
}

MS_BOOL MsOS_WaitEvent (MS_U32  u32EventGroupId,
                        MS_U32  u32WaitEventFlag,
                        MS_U32  *pu32RetrievedEventFlag,
                        EventWaitMode eWaitMode,
                        MS_U32  u32WaitMs)
{
        //if(s32EventGroupId ==-1)
        //	return 0;
        MSOS_event  *pEvent = (MSOS_event  *)u32EventGroupId;
        if(!pEvent || pEvent->eventDeleted)
            return 0;
        if(u32WaitMs==0)
        {
            MsOS_ObtainSemaphore(pEvent->eventSemaphore,-1);
            *pu32RetrievedEventFlag = (u32WaitEventFlag&pEvent->eventFlag);
            switch(eWaitMode)
            {
                case E_OR:
                if(*pu32RetrievedEventFlag)
                {
                    MsOS_ReleaseSemaphore(pEvent->eventSemaphore);    
                    return 1;
                }
                case E_OR_CLEAR:
                if(*pu32RetrievedEventFlag)
                {
                    pEvent->eventFlag &= ~(*pu32RetrievedEventFlag);
                    MsOS_ReleaseSemaphore(pEvent->eventSemaphore);    
                    return 1;
                }
                break;
            }
            MsOS_ReleaseSemaphore(pEvent->eventSemaphore);        
            return 0;
        }	
        else
        {
            EVENT_MESSAGE  Event_Message;
            Event_Message.MessageID = MESSAGE_WAIT_EVENT;
            Event_Message.Param1      = u32WaitEventFlag;
            Event_Message.Param2	  = eWaitMode;
            Event_Message.Result      = malloc(sizeof(MS_U32));
            Event_Message.SendFLag    = 1;
            Event_Message.ReturnSemaphore = MsOS_CreateSemaphore(0);
            MsOS_SendToQueue(pEvent->u32QueueId, &Event_Message, sizeof(EVENT_MESSAGE), -1);
            if(!MsOS_ObtainSemaphore(Event_Message.ReturnSemaphore,u32WaitMs))
            {
                //ms_print.pfun("MsOS_WaitEvent 0 %s wait sem is 0x%x\n",pEvent->eventName, Event_Message.ReturnSemaphore);
                *pu32RetrievedEventFlag = 0;
                Event_Message.MessageID = MESSAGE_RELEASE_EVENT;
                MsOS_SendToQueue(pEvent->u32QueueId, &Event_Message, sizeof(EVENT_MESSAGE), -1);
                return 0;
            }
            //ms_print.pfun("MsOS_WaitEvent 1 wait sem is 0x%x\n",Event_Message.ReturnSemaphore);
            *pu32RetrievedEventFlag =  *(MS_U32 *)Event_Message.Result;
            free((void *)Event_Message.Result);
            MsOS_DeleteSemaphore(Event_Message.ReturnSemaphore);
        }
        return 1;
}

