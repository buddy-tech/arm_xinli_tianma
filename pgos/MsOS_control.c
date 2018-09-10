#include "MsOS.h"

/* Apartment Processing */
static MS_VOID ApartmentThread(MS_VOID *pParameter)
{
    MS_APARTMENT	*pApartment = pParameter;
    MSOS_MESSAGE 	Message;
    MS_U32  s32ActualSize;
    MS_U32 	Result;

    while(1)
    {
        Result = MsOS_RecvFromQueue (pApartment->MessageQueue, &Message, sizeof(MSOS_MESSAGE), &s32ActualSize, -1);

        Result = pApartment->MessageHandler (pApartment,Message);

        if(Message.Blocking && Message.pResult!=NULL)
        {
            *Message.pResult= Result;
        }
        if(Message.Blocking)
        {
            MsOS_ReleaseSemaphore(Message.ReturnSemaphore);
        }
    }
}

MS_APARTMENT *MsOS_CreateApartment(MS_CHAR *pApartmentName,
                                   MS_MESSAGE_HANDLER MessageHandler,
                                   MS_EVENT_HANDLER_LIST *pEventHandlerList)
{
    MS_APARTMENT *pApartment=(MS_APARTMENT*)MsOS_AllocateMemory(sizeof(MS_APARTMENT),0);

    pApartment->MessageQueue = MsOS_CreateQueue(100,sizeof(MSOS_MESSAGE));

    if(strlen(pApartmentName) < 32)
    {
        strcpy(pApartment->ApartmentName,pApartmentName);
    }
    else
    {
        memcpy(pApartment->ApartmentName,pApartmentName,31);
        pApartment->ApartmentName[31] = 0;
    }

    pApartment->Semaphore = MsOS_CreateSemaphore(1);

    pApartment->MessageHandler   = MessageHandler;

    pApartment->EventHandlerList = pEventHandlerList;

    pApartment->Thread = MsOS_CreateTask (ApartmentThread,
                            pApartment,
                            0,
                            0,
                            0,pApartmentName);

    return pApartment;
}

MS_U32 MsOS_DestroyApartment(MS_APARTMENT *pApartment)
{
    MsOS_DeleteTask(pApartment->Thread);
    MsOS_DeleteSemaphore(pApartment->Semaphore);
    MsOS_DeleteQueue(pApartment->MessageQueue);
    MsOS_FreeMemory(pApartment,0);
    return 0;
}


