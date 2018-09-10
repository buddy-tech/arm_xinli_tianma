#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "MsOS.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include "common.h"

MS_PRINT_T ms_print;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#define MAX_PROCESSES 60

static Task_Info   processes[MAX_PROCESSES];
static MS_BOOL     bInit = MS_FALSE;

static void  ms_printf(const char * Format_p, ...)
{
    if(ms_print.fp)
    {
        va_list list;
        va_start(list,Format_p);
        vfprintf(ms_print.fp,Format_p,list);
        va_end(list);
    }
    return;
}

MS_BOOL MsOS_Init (Printk pfun,void *fp)
{
    if(!bInit)
    {
        memset(processes,0,sizeof(processes));
        if(pfun)
        {
            ms_print.pfun = pfun;
        }
        else
        {
            ms_print.fp = fp;
            ms_print.pfun = ms_printf;
        }
        bInit = TRUE;
    }
	return TRUE;
}


void * MsOS_AllocateMemory (MS_U32 u32Size, MS_U32 u32PoolId)
{
	return malloc(u32Size);
}

void * MsOS_ReallocateMemory (void *pOrgAddress, MS_U32 u32NewSize, MS_U32 u32PoolId)
{
	return realloc(pOrgAddress,u32NewSize);
}

MS_BOOL MsOS_FreeMemory (void *pAddress, MS_U32 u32PoolId)
{
	if(!pAddress)
		return FALSE;
	else
	{
		free(pAddress);
		return TRUE;
	}
}



MS_U32 MsOS_CreateTask (TaskEntry pTaskEntry,
                        MS_U32 u32TaskEntryData,
                        MS_U32 eTaskPriority,
                        MS_U32 *pu32StopFlag,
                        MS_U32 u32StackSize,
                        char *pTaskName)
{
	pthread_t tid;
	pthread_attr_t attr;
	struct sched_param param;
	int i;
	int max_prio, min_prio;
	int ret = 0;
    pthread_mutex_lock(&mutex);
	if(pTaskEntry == NULL)
	{
		pthread_mutex_unlock(&mutex);
		return 0;
	}
	for(i = 0; i < MAX_PROCESSES; i++)
	{
        if(processes[i].iId == 0)
	    {
	        break;
	    }
	}
	if(i >= MAX_PROCESSES)
	{
		pthread_mutex_unlock(&mutex);
		return 0;
	}
	if(eTaskPriority < 0)
	{
		pthread_mutex_unlock(&mutex);
		return 0;
	}
	if(ret >= 0)
	{
	    ret = pthread_attr_init(&attr);
	}
	if(ret >= 0)
	{
		ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	}
    if(ret >= 0)
	{
        if(u32StackSize != NULL)
	    {
            ret = pthread_attr_setstacksize(&attr,(size_t)u32StackSize);
	    }
	}
	if(ret >= 0)
	{
        ret = pthread_create(&tid, &attr,(void *(*) (void *))pTaskEntry, (void *)u32TaskEntryData);
	}
	if(ret >= 0)
	{   
		ret = pthread_attr_destroy(&attr);
	}
	if(ret >= 0)
	{
        processes[i].iId = tid;
        processes[i].ePriority = eTaskPriority;
        processes[i].u32StackSize = u32StackSize;
        processes[i].pu32StopFlag = pu32StopFlag;

        if(pTaskName)
        {
            strcpy(processes[i].szName,pTaskName);
        }
	}
	pthread_mutex_unlock(&mutex);
	return (ret >= 0) ? (int)&processes[i] : 0;
}

MS_BOOL MsOS_DeleteTask (MS_U32 u32TaskId)
{
    Task_Info *pTask = NULL;
	pthread_mutex_lock(&mutex);
    pTask = (Task_Info *)u32TaskId;
	if(pTask == NULL)
	{
	    pthread_mutex_unlock(&mutex);
	    return false;
	}

    if(pTask->iId)
	{
        if(pTask->pu32StopFlag)
        {
            *pTask->pu32StopFlag = 1;
        }
        pthread_join((pthread_t)pTask->iId,0);
        pTask->iId = 0;
        if(pTask->pu32StopFlag)
        {
            *pTask->pu32StopFlag = 0;
        }
	    pthread_mutex_unlock(&mutex);
	    return true;
	}
	else
	{
	    pthread_mutex_unlock(&mutex);
	    return false;
	}
}

MS_U32 MsOS_InfoTaskID (void)
{
    pthread_t tid = 0;
    int task_id = 0;
    int i;

    tid = pthread_self();
    for(i = 0; i < MAX_PROCESSES; i++)
    {
        if(tid == processes[i].iId)
        {
            task_id = (int )&processes[i];
            break;
        }
    }
    return task_id;
}


void MsOS_DelayTask (MS_U32 u32Ms)
{
#if WIN32
    Sleep(u32Ms);
#else
	usleep(1000*u32Ms);
#endif
}


MS_U32 MsOS_CreateMutex ()
{
    return MsOS_CreateSemaphore(1);
}

MS_BOOL MsOS_DeleteMutex (MS_U32 u32MutexId)
{
	return MsOS_DeleteSemaphore(u32MutexId);
}

MS_BOOL MsOS_ObtainMutex (MS_U32 u32MutexId, MS_U32 u32WaitMs)
{
	return MsOS_ObtainSemaphore(u32MutexId,u32WaitMs);
}

MS_BOOL MsOS_ReleaseMutex (MS_U32 u32MutexId)
{
	return MsOS_ReleaseSemaphore(u32MutexId);
}

MS_U32 MsOS_CreateSemaphore (MS_U32 u32InitCnt)
{
	sem_t *sem;
	sem = (sem_t*)malloc(sizeof(sem_t));
	sem_init(sem,0,u32InitCnt);
	return (MS_U32)sem;
}

MS_BOOL MsOS_DeleteSemaphore (MS_U32 u32SemaphoreId)
{
	sem_t *sem = (sem_t*)u32SemaphoreId;
	if(sem == NULL)
	return FALSE;
	sem_destroy(sem);
	free(sem);
	return TRUE;
}

MS_BOOL MsOS_ObtainSemaphore (MS_U32 u32SemaphoreId, MS_U32 u32WaitMs)
{
    int sem_ret;
    sem_t *sem = (sem_t*)u32SemaphoreId;
    struct timespec tspec;

    if(sem == NULL)
        return FALSE;
    
    if(u32WaitMs == MSOS_WAIT_FOREVER)
    {
AGAIN1:
        sem_ret = sem_wait(sem);
        if(sem_ret < 0)
        {
            if(errno == EAGAIN)
                goto AGAIN1;
            else
                return FALSE;
        }
    }
    else
    {
        clock_gettime(CLOCK_REALTIME, &tspec);
        tspec.tv_sec += (tspec.tv_nsec / 1000000 + u32WaitMs) / 1000;
        tspec.tv_nsec = ((u32WaitMs + tspec.tv_nsec / 1000000) % 1000) * 1000000;
AGAIN2:
        sem_ret = sem_timedwait(sem,&tspec);
        if(sem_ret < 0)
        {
            if(errno == EAGAIN)
                goto AGAIN2;
            else if(errno == ETIMEDOUT)
                return FALSE;
            else
                return FALSE;
        }
    }
    return TRUE;
}


MS_BOOL MsOS_ReleaseSemaphore (MS_U32 u32SemaphoreId)
{
	sem_t *sem = (sem_t*)u32SemaphoreId;
	if(sem == NULL)
		return FALSE;
	if(sem_post(sem))
		return FALSE;
	return TRUE;
}


MS_U64 MSOS_TimeNow()
{
	MS_U64  TimeNowMs;
	struct timeval stTiemNow;
	gettimeofday(&stTiemNow, NULL);
	TimeNowMs =  stTiemNow.tv_sec*1000 + stTiemNow.tv_usec/1000;
	return TimeNowMs;
}

MS_U64 MSOS_TimeNowUs()
{
    MS_U64  TimeNowUs;
    struct timeval stTiemNow;
    gettimeofday(&stTiemNow, NULL);
    TimeNowUs =  stTiemNow.tv_sec*1000*1000 + stTiemNow.tv_usec;
    return TimeNowUs;
}


