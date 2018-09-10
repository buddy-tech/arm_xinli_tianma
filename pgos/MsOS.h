////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2007 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (¨MStar Confidential Information〃) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
///
/// @file   MsOS.h
/// @brief  MStar OS Wrapper
/// @author MStar Semiconductor Inc.
///////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef _MS_OS_H_
#define _MS_OS_H_

#include "MsTypes.h" 
#include <string.h>


#ifdef __cplusplus
extern "C"
{
#endif


//-------------------------------------------------------------------------------------------------
// Defines
//-------------------------------------------------------------------------------------------------
#define MSOS_TASK_MAX           (32+120)
#define MSOS_MEMPOOL_MAX        (8+64)
#define MSOS_FIXSIZE_MEMPOOL_MAX  (8)
#define MSOS_SEMAPHORE_MAX      (32+150)
#define MSOS_MUTEX_MAX          (64+240)
#define MSOS_EVENTGROUP_MAX     (32)
#define MSOS_TIMER_MAX          (16)
//#define MSOS_SIGNAL_MAX         (8)
#define MSOS_QUEUE_MAX          (16+60)

//-------------------------------------------------------------------------------------------------
// Macros
//-------------------------------------------------------------------------------------------------
//time and clock macros
#define TICK_PER_ONE_MS     (1) //Note: confirm Kernel fisrt
#define MSOS_WAIT_FOREVER   0xffffffff


//-------------------------------------------------------------------------------------------------
// Type and Structure Declaration
//-------------------------------------------------------------------------------------------------
#if 1

//compatible with Nucleus's task_entry
typedef void*( *TaskEntry ) (void *argv); ///< Task entry function  argc: pass additional data to task entry; argv: not used by eCos
typedef void ( *InterruptCb ) (void);     ///< Interrupt callback function
typedef void ( *SignalCb ) (MS_U32 u32Signals);        ///< Signal callback function
typedef void ( *TimerCb ) (MS_U32 u32StTimer);  ///< Timer callback function  u32StTimer:  Timer ID
typedef void ( *Printk )(const char * Format_p, ...);
/// Task priority
typedef enum
{
    E_TASK_PRI_SYS      = 99,    ///< System priority task   ( interrupt level driver, e.g. TSP, SMART )
    E_TASK_PRI_HIGHEST  = 90,    ///< Highest priority task  ( background monitor driver, e.g. DVBC, HDMI )
    E_TASK_PRI_HIGH     = 80,    ///< High priority task     ( service task )
    E_TASK_PRI_MEDIUM   = 70,   ///< Medium priority task   ( application task )
    E_TASK_PRI_LOW      = 60,   ///< Low priority task      ( nonbusy application task )
    E_TASK_PRI_LOWEST   = 24,   ///< Lowest priority task   ( idle application task )
} TaskPriority;

/// Suspend type
typedef enum
{
    E_MSOS_PRIORITY,            ///< Priority-order suspension
    E_MSOS_FIFO,                ///< FIFO-order suspension
} MsOSAttribute;

/// Message size type
typedef enum
{
    E_MSG_FIXED_SIZE,           ///< Fixed size message
    E_MSG_VAR_SIZE,             ///< Variable size message
} MessageType;

/// Event mode
typedef enum
{
    E_AND,                      ///< Specify all of the requested events are require.
    E_OR,                       ///< Specify any of the requested events are require.
    E_AND_CLEAR,                ///< Specify all of the requested events are require. If the request are successful, clear the event.
    E_OR_CLEAR,                 ///< Specify any of the requested events are require. If the request are successful, clear the event.
} EventWaitMode;

typedef struct
{
    MS_U32                          iId;
    TaskPriority                    ePriority;
    MS_U32                          u32StackSize;
    MS_U32                          *pu32StopFlag;
    char szName[16];
} Task_Info, *PTask_Info;


typedef struct tag_MS_EVENT
{
    MS_U32			EventID;
    MS_U32			Parameter1;
    MS_U32			Parameter2;
}MS_EVENT;

typedef MS_BOOL (*MS_EVENT_HANDLER)(MS_EVENT Event);

typedef struct tag_MS_EVENT_HANDLER_LIST_NODE
{
    MS_EVENT_HANDLER EventHandler;
    MS_BOOL Enabled;
    MS_S32	Reserved;
    struct tag_MS_EVENT_HANDLER_LIST_NODE *pPre;
    struct tag_MS_EVENT_HANDLER_LIST_NODE *pNext;
}MS_EVENT_HANDLER_LIST_NODE;

typedef struct tag_MS_EVENT_HANDLER_LIST
{
    MS_S32 NumberOfEventHandlers;
    MS_EVENT_HANDLER_LIST_NODE *pHead;
    MS_EVENT_HANDLER_LIST_NODE *pTail;
} MS_EVENT_HANDLER_LIST;


typedef struct tag_MSOS_MESSAGE
{
    MS_U32              MessageID;
    MS_U32				Parameter1;
    MS_U32				Parameter2;
    MS_BOOL				Blocking;
    MS_U32				*pResult;
    MS_U32              ReturnSemaphore;
}MSOS_MESSAGE;

typedef struct tag_MS_PRINT_S
{
    Printk pfun;
    void   *fp;
}MS_PRINT_T;

extern MS_PRINT_T ms_print;

#elif defined (MSOS_TYPE_XXX)

#error "The OS is not supported now ..."

#endif

//-------------------------------------------------------------------------------------------------
// Extern Functions
//-------------------------------------------------------------------------------------------------
//
// Init
//
MS_BOOL MsOS_Init (Printk pfun,void *fp);


//
// Memory management
//
void * MsOS_AllocateMemory (MS_U32 u32Size, MS_U32 u32PoolId);

void * MsOS_ReallocateMemory (void *pOrgAddress, MS_U32 u32NewSize, MS_U32 u32PoolId);

MS_BOOL MsOS_FreeMemory (void *pAddress, MS_U32 u32PoolId);


//
// Task
//
MS_U32 MsOS_CreateTask (TaskEntry pTaskEntry,
                        MS_U32 u32TaskEntryData,
                        MS_U32 eTaskPriority,
                        MS_U32 *pu32StopFlag,
                        MS_U32 u32StackSize,
                        char *pTaskName);

MS_BOOL MsOS_DeleteTask (MS_U32 u32TaskId);

MS_U32 MsOS_InfoTaskID (void);

void MsOS_DelayTask (MS_U32 u32Ms);



//
// Mutex
//
MS_U32 MsOS_CreateMutex ();

MS_BOOL MsOS_DeleteMutex (MS_U32 u32MutexId);

MS_BOOL MsOS_ObtainMutex (MS_U32 u32MutexId, MS_U32 u32WaitMs);

MS_BOOL MsOS_ReleaseMutex (MS_U32 s32MutexId);


//
// Semaphore
//
MS_U32 MsOS_CreateSemaphore (MS_U32 u32InitCnt);

MS_BOOL MsOS_DeleteSemaphore (MS_U32 u32SemaphoreId);

MS_BOOL MsOS_ObtainSemaphore (MS_U32 u32SemaphoreId, MS_U32 u32WaitMs);

MS_BOOL MsOS_ReleaseSemaphore (MS_U32 u32SemaphoreId);


//
// Event management
//
MS_U32 MsOS_CreateEventGroup (char *pName);

MS_BOOL MsOS_DeleteEventGroup (MS_U32 u32EventGroupId);

MS_BOOL MsOS_SetEvent (MS_U32 s32EventGroupId, MS_U32 u32EventFlag);

MS_BOOL MsOS_ClearEvent (MS_U32 s32EventGroupId, MS_U32 u32EventFlag);

MS_BOOL MsOS_WaitEvent (MS_U32  s32EventGroupId,
                        MS_U32  u32WaitEventFlag,
                        MS_U32  *pu32RetrievedEventFlag,
                        EventWaitMode eWaitMode,
                        MS_U32  u32WaitMs);

/*
//
// Signal management
//
MS_BOOL MsOS_CreateSignal (SignalCb pSignalCb);

MS_BOOL MsOS_ControlSignals (MS_U32 u32SignalMask);

MS_BOOL MsOS_SendSignals (MS_S32 s32TaskId, MS_U32 u32Signal);

MS_U32 MsOS_ReceiveSignals (void);
*/

//
// Timer management
//
MS_VOID   MsOS_TimerInit(void);
MS_U32    MsOS_CreateTimer (TimerCb    pTimerCb,
                         MS_U32     u32FirstTimeMs,
                         MS_U32     u32PeriodTimeMs,
                         MS_BOOL    bStartTimer,
                         char       *pName);

MS_BOOL MsOS_DeleteTimer (MS_U32 u32TimerId);

MS_BOOL MsOS_StartTimer (MS_U32 u32TimerId);

MS_BOOL MsOS_StopTimer (MS_U32 u32TimerId);

MS_BOOL MsOS_ResetTimer (MS_U32  u32TimerId,
                         MS_U32     u32FirstTimeMs,
                         MS_U32     u32PeriodTimeMs,
                         MS_BOOL    bStartTimer);

//
// System time
//
//MS_U32 MsOS_GetSystemTick (void);

MS_U32 MsOS_GetSystemTime (void);

//MS_BOOL MsOS_SetSystemTime (MS_U32 u32SystemTime);


//
// Queue
//
MS_U32 MsOS_CreateQueue (MS_U32         u32QueueSize,
                         MS_U32         u32MessageSize);

MS_BOOL MsOS_DeleteQueue (MS_U32 u32QueueId);

MS_BOOL MsOS_SendToQueue (MS_U32 u32QueueId, MS_U8 *pu8Message, MS_U32 u32Size, MS_U32 u32WaitMs);

MS_BOOL MsOS_RecvFromQueue (MS_U32 u32QueueId, MS_U8 *pu8Message, MS_U32 u32IntendedSize, MS_U32 *pu32ActualSize, MS_U32 u32WaitMs);

MS_BOOL MsOS_PeekFromQueue (MS_U32 u32QueueId, MS_U8 *pu8Message, MS_U32 u32IntendedSize, MS_U32 *pu32ActualSize);

MS_U32 MsOS_SendMessage(MS_U32 u32MsgQueueId,MS_U32 MessageID,MS_U32 Parameter1,MS_U32 Parameter2);

MS_U32 MsOS_PostMessage(MS_U32 u32MsgQueueId,MS_U32 MessageID,MS_U32 Parameter1,MS_U32 Parameter2);

//Event Handler
MS_BOOL MsOS_DispatchEventHanders(MS_EVENT_HANDLER_LIST *pEventHandlerList, MS_EVENT Event);

MS_BOOL MsOS_CreateEventHanderList(MS_EVENT_HANDLER_LIST **ppEventHanderList);

MS_BOOL MsOS_AddEventHanderToList(
    MS_EVENT_HANDLER_LIST *pEventHanderList,
    MS_EVENT_HANDLER EventHander,
    MS_BOOL Enabled);

MS_BOOL MsOS_SetEventHanderEnable(
    MS_EVENT_HANDLER_LIST *pEventHanderList,
    MS_EVENT_HANDLER EventHander,
    MS_BOOL Enabled);

MS_BOOL MsOS_DestoryEventHanderList(MS_EVENT_HANDLER_LIST *pEventHanderList);

void testmain(void);

void registerHandler(MS_EVENT_HANDLER pHandler);


typedef MS_U32 ( *MS_MESSAGE_HANDLER ) (MS_VOID *pApartMent,MSOS_MESSAGE msg);

typedef struct tag_MS_APARTMENT_s
{
    MS_U32 MessageQueue;
    MS_U32 Thread;
    MS_U32 Semaphore;
    MS_MESSAGE_HANDLER     MessageHandler;
    MS_EVENT_HANDLER_LIST *EventHandlerList;
    MS_CHAR ApartmentName[32];
}MS_APARTMENT;

MS_APARTMENT *MsOS_CreateApartment(MS_CHAR *pApartmentName,
                                   MS_MESSAGE_HANDLER MessageHandler,
                                   MS_EVENT_HANDLER_LIST *pEventHandlerList);

MS_U32 MsOS_DestroyApartment(MS_APARTMENT *pApartment);

#ifdef __cplusplus
}
#endif


#endif // _MS_OS_H_

