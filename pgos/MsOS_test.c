#include <stdio.h>
#include "MsOS.h"

#if 0
#define TEST_TIME 1

static MS_U32 u32StopFlag1 = 0;
static MS_U32 u32StopFlag2 = 0;

typedef struct tst_msg_s_info
{
    MS_U32 tst;
}msg_info_t;

static MS_U32 tstqueueId;

static MS_U32 tstEventGroupId;

static MS_U32 u32WaitEventFlag = 0x00000001;

static MS_EVENT_HANDLER curEvHandler;
static MS_EVENT_HANDLER_LIST *pEventHanderList;

void registerHandler(MS_EVENT_HANDLER pHandler)
{
    curEvHandler = pHandler;
}

void *helloThread1(void *param)
{
    msg_info_t msg_info;
    MS_S32 s32ActualSize;
    MS_BOOL ret;
    while(!u32StopFlag1)
    {
        ret = MsOS_RecvFromQueue (tstqueueId, &msg_info, sizeof(msg_info_t), &s32ActualSize, -1);
        if(ret)
        {
            ms_print.pfun("hello1!\n");
            MS_EVENT Event;
            Event.EventID = 1234;
            MsOS_DispatchEventHanders(pEventHanderList, Event);
        }
    }
    ms_print.pfun("leave the thread 1\n");
    return NULL;
}

void *helloThread2(void *param)
{
    MS_U32  u32RetrievedEventFlag;
    MS_BOOL ret;
    while(!u32StopFlag2)
    {
        ret = MsOS_WaitEvent (tstEventGroupId,
                                u32WaitEventFlag,
                                &u32RetrievedEventFlag,
                                E_OR_CLEAR,
                                -1);
        if(ret)
        {
            ms_print.pfun("x");
        }
        else
        {
            MsOS_DelayTask(1);
            ms_print.pfun("hello2!!!!!!!!!!!!!\n");
        }

    }
    ms_print.pfun("leave the thread 2\n");
    return NULL;
}

void tstTimer(MS_U32 u32Timer)
{
    ms_print.pfun("hello timer\n");
}

MS_BOOL tstEvHandler(MS_EVENT Event)
{
    ms_print.pfun("event id is %d\n",Event.EventID);
}



void testmain()
{
    MS_U32 taskId1,taskId2,timerId;
    msg_info_t msg_info;

    MsOS_Init(printf,0);

    registerHandler(tstEvHandler);

    ms_print.pfun("Hello World start!\n");

#if 1
    MsOS_CreateEventHanderList(&pEventHanderList);

    MsOS_AddEventHanderToList(pEventHanderList,curEvHandler,
        MS_FALSE);

    MsOS_SetEventHanderEnable(pEventHanderList,curEvHandler,
        MS_TRUE);

    //MS_EVENT Event;
    //Event.EventID = 1234;
    //MsOS_DispatchEventHanders(pEventHanderList, Event);

    tstEventGroupId = MsOS_CreateEventGroup("tstEvent");

    tstqueueId = MsOS_CreateQueue(100,sizeof(msg_info_t));

    taskId1 = MsOS_CreateTask(helloThread1,0,0,&u32StopFlag1,0,0);
    taskId2 = MsOS_CreateTask(helloThread2,0,0,&u32StopFlag2,0,0);

    MsOS_SendToQueue (tstqueueId, &msg_info, sizeof(msg_info), -1);

    MsOS_SetEvent(tstEventGroupId,u32WaitEventFlag);

    MsOS_DelayTask(1000);

    MsOS_DeleteQueue(tstqueueId);

    MsOS_DeleteEventGroup(tstEventGroupId);

    MsOS_DelayTask(1000);

    MsOS_CreateEventGroup("tstEvent");

    MsOS_DeleteTask(taskId1);

    MsOS_DelayTask(1000);

    MsOS_DeleteTask(taskId2);
#endif

#if TEST_TIME
    MsOS_TimerInit();

    timerId = MsOS_CreateTimer (tstTimer,
                            0,
                            1000,
                            0,
                            "tstTimer");

    MsOS_StartTimer(timerId);

    MsOS_DelayTask(10000);

    MsOS_StopTimer(timerId);

    //MsOS_ResetTimer(timerId,0,1000,1);

    //MsOS_DelayTask(1000);

    //MsOS_DeleteTimer(timerId);
#endif
    //MsOS_DestoryEventHanderList(pEventHanderList);

    ms_print.pfun("Hello World over!\n");
}

#endif

