#include "MsOS.h"

MS_BOOL MsOS_CreateEventHanderList(MS_EVENT_HANDLER_LIST **ppEventHanderList)
{
    (*ppEventHanderList) = (MS_EVENT_HANDLER_LIST *)MsOS_AllocateMemory(sizeof(MS_EVENT_HANDLER_LIST),0);
    (*ppEventHanderList)->NumberOfEventHandlers = 0;
    (*ppEventHanderList)->pHead = NULL;
    (*ppEventHanderList)->pTail = NULL;

    return MS_TRUE;
}

MS_BOOL MsOS_DestoryEventHanderList(MS_EVENT_HANDLER_LIST *pEventHanderList)
{
    MS_EVENT_HANDLER_LIST_NODE *pTempEventHanderListHead,*pEventHanderListHead = 0;
    if(!pEventHanderList)
        return MS_TRUE;
    pTempEventHanderListHead = pEventHanderList->pHead;
    while(pEventHanderListHead)
    {
        pTempEventHanderListHead = pEventHanderListHead->pNext;
        MsOS_FreeMemory(pEventHanderListHead,0);
        pEventHanderListHead = pTempEventHanderListHead;
    }
    MsOS_FreeMemory(pEventHanderList,0);
    return MS_TRUE;
}

MS_BOOL MsOS_AddEventHanderToList(
    MS_EVENT_HANDLER_LIST *pEventHanderList,
    MS_EVENT_HANDLER EventHander,
    MS_BOOL Enabled)
{
    MS_EVENT_HANDLER_LIST_NODE *pNewListNode;
    MS_EVENT_HANDLER_LIST_NODE *pTmpListNode;

    pNewListNode = (MS_EVENT_HANDLER_LIST_NODE*)MsOS_AllocateMemory(sizeof(MS_EVENT_HANDLER_LIST_NODE),0);
    pNewListNode->EventHandler = EventHander;
    pNewListNode->Reserved	 = -1;
    pNewListNode->Enabled = Enabled;
    pNewListNode->pNext = NULL;

    pEventHanderList->pTail = pNewListNode;
    pEventHanderList->NumberOfEventHandlers += 1;

    /* no ListNode yet */
    if(pEventHanderList->pHead == NULL)
    {
        pEventHanderList->pHead = pNewListNode;
        pNewListNode->pPre = NULL;

        return MS_TRUE;
    }

    /* search the last ListNode */
    pTmpListNode = pEventHanderList->pHead;
    while(pTmpListNode->pNext != NULL)
    {
        pTmpListNode = pTmpListNode->pNext;
    }
    pTmpListNode->pNext = pNewListNode;
    pNewListNode->pPre = pTmpListNode;

    return MS_TRUE;
}

MS_BOOL MsOS_SetEventHanderEnable(
    MS_EVENT_HANDLER_LIST *pEventHanderList,
    MS_EVENT_HANDLER EventHander,
    MS_BOOL Enabled)
{
    MS_EVENT_HANDLER_LIST_NODE *pTmpListNode;

    /* search the ListNode */
    pTmpListNode = pEventHanderList->pHead;
    while(pTmpListNode != NULL)
    {
        if(pTmpListNode->EventHandler == EventHander)
        {
            pTmpListNode->Enabled = Enabled;
            return MS_TRUE;
        }

        pTmpListNode = pTmpListNode->pNext;
    }

    return MS_FALSE;
}

MS_BOOL MsOS_DispatchEventHanders(MS_EVENT_HANDLER_LIST *pEventHandlerList, MS_EVENT Event)
{
    MS_EVENT_HANDLER_LIST_NODE	*pEventHanderListNode;

    if(pEventHandlerList == NULL)
        return MS_TRUE;

    pEventHanderListNode = pEventHandlerList->pHead;
    while(pEventHanderListNode != NULL)
    {
        if(pEventHanderListNode->Enabled != MS_FALSE)
            pEventHanderListNode->EventHandler(Event);

        pEventHanderListNode = pEventHanderListNode->pNext;
    }


    return MS_TRUE;
}
