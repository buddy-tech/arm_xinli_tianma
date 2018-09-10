#include "MsOS.h"
#include <time.h>

#define	TIMER_INTERVAL	1//ms

typedef struct tag_MSOS_TIMER
{
	MS_BOOL 	bStartTimer;		
	MS_U32 	FirstTimeMs;
    MS_U32	PeriodTimeMs;
	TimerCb 	pTimerCb;
	MS_U32		initTimeMs;		//timer create time
	MS_U32		times;			//timer repeate times
	struct tag_MSOS_TIMER *pNext;
}MSOS_timer;

static MS_U32 s_taskId;
static MS_U32 s_semaphore;	
static MSOS_timer* s_pTimerHead=0;
static MS_U32 s_taskStop = 0;


MS_U32 MsOS_GetSystemTime (void)
{
	struct timespec ts;
    
    if (0 == clock_gettime(CLOCK_MONOTONIC, &ts))
    {
		return (ts.tv_sec * 1000 + ts.tv_nsec/1000000);
    }
    else
    {
		return 0;
    }
}

static void *TimerProcess(void *pvParam)
{
	MSOS_timer    *pTimer = NULL;
	MS_U32		   timeNow=0;
    while(!s_taskStop)
	{
		MsOS_DelayTask(TIMER_INTERVAL);	
		MsOS_ObtainSemaphore(s_semaphore, -1);

		pTimer = s_pTimerHead;
		while(pTimer)
		{
			timeNow = MsOS_GetSystemTime();
			
            if(pTimer->bStartTimer)
			{
				if(pTimer->times == 0 )
				{
					// first.
					if( pTimer->initTimeMs+pTimer->FirstTimeMs <= timeNow )
					{
						if(pTimer->pTimerCb)
						{
                            pTimer->pTimerCb((MS_U32)pTimer);
						}
						pTimer->times = 1;
					}
					else
					{
						pTimer = pTimer->pNext;
						continue;
					}
				}
				else 
				{
					// repeat
					if(pTimer->PeriodTimeMs)
					{// timer need repeat generate callback
						if(pTimer->initTimeMs+pTimer->FirstTimeMs+pTimer->PeriodTimeMs*pTimer->times <= timeNow)
						{
							if(pTimer->pTimerCb)
							{
                                pTimer->pTimerCb((MS_U32)pTimer);
							}
							pTimer->times++;
						}
						else
						{
							pTimer = pTimer->pNext;
							continue;
						}
					}
					else
					{
						pTimer = pTimer->pNext;
						continue;
					}
				}
			}
			
			pTimer = pTimer->pNext;
		}
		MsOS_ReleaseSemaphore(s_semaphore);
	}
    return NULL;
}

MS_U32 MsOS_CreateTimer (TimerCb    pTimerCb,
                         MS_U32     u32FirstTimeMs,
                         MS_U32     u32PeriodTimeMs,
                         MS_BOOL    bStartTimer,
                         char       *pName)
{
    MSOS_timer  *pPrev,*pCur;
    MSOS_timer* pTimer=0;
    MsOS_ObtainSemaphore(s_semaphore, -1);
	
    pCur = pPrev = s_pTimerHead;
	
    while(pCur)
    {
        pPrev = pCur;
        pCur = pCur->pNext;
    }
	
    pTimer = MsOS_AllocateMemory(sizeof(MSOS_timer), 0);
    if(pTimer)
    {
            pTimer->bStartTimer = bStartTimer;
            pTimer->FirstTimeMs = u32FirstTimeMs;
            pTimer->PeriodTimeMs = u32PeriodTimeMs;
            pTimer->pTimerCb = pTimerCb;
			
            if(bStartTimer)
            {
                pTimer->initTimeMs = MsOS_GetSystemTime();
                pTimer->times = 0;
            }
			
            pTimer->times = 0;
            pTimer->pNext = 0;
			
            if(pPrev)
                pPrev->pNext = pTimer;
            else
                s_pTimerHead = pTimer;
    }
	
    MsOS_ReleaseSemaphore(s_semaphore);
    return (MS_U32)pTimer;
}

MS_BOOL MsOS_DeleteTimer (MS_U32 u32TimerId)
{
	MSOS_timer  *pPrev,*pCur;
    MsOS_ObtainSemaphore(s_semaphore, -1);
    pCur=pPrev=s_pTimerHead;
    while(pCur)
    {
        if(pCur == (MSOS_timer*)u32TimerId)
        {
            pPrev->pNext = pCur->pNext;
            if(pCur == s_pTimerHead)
            	    s_pTimerHead = pCur->pNext;
            MsOS_FreeMemory(pCur, 0);
            break;
        }
        pPrev = pCur;
        pCur = pCur->pNext;
    }
    MsOS_ReleaseSemaphore(s_semaphore);
}

MS_BOOL MsOS_StartTimer (MS_U32 u32TimerId)
{
    MSOS_timer  *pCur;
    MsOS_ObtainSemaphore(s_semaphore, -1);
    pCur=s_pTimerHead;
    while(pCur)
    {
    	if(pCur == (MSOS_timer*)u32TimerId)
    	{
    		pCur->bStartTimer = 1;
    		pCur->initTimeMs = MsOS_GetSystemTime();
    		pCur->times=0;
    		break;
    	}
        pCur = pCur->pNext;
    }
    MsOS_ReleaseSemaphore(s_semaphore);
}

MS_BOOL MsOS_StopTimer (MS_U32 u32TimerId)
{
        MSOS_timer  *pCur;
        MsOS_ObtainSemaphore(s_semaphore, -1);
        pCur=s_pTimerHead;
        while(pCur)
        {
        	if(pCur == (MSOS_timer*)u32TimerId)
        	{
        		pCur->bStartTimer = 0;
        		pCur->initTimeMs = MsOS_GetSystemTime();
        		pCur->times=0;
        		break;
        	}
            pCur = pCur->pNext;
        }
        MsOS_ReleaseSemaphore(s_semaphore);
}

MS_BOOL MsOS_ResetTimer (MS_U32     u32TimerId,
                         MS_U32     u32FirstTimeMs,
                         MS_U32     u32PeriodTimeMs,
                         MS_BOOL    bStartTimer)
{
        MSOS_timer  *pCur;
        MsOS_ObtainSemaphore(s_semaphore, -1);
        pCur=s_pTimerHead;
        while(pCur)
        {
                if(pCur == (MSOS_timer*)u32TimerId)
                {
                	pCur->bStartTimer = bStartTimer;
                	pCur->FirstTimeMs = u32FirstTimeMs;
                	pCur->PeriodTimeMs = u32PeriodTimeMs;
                	
                	pCur->initTimeMs=MsOS_GetSystemTime();
                	pCur->times=0;
                	break;
                }
                pCur = pCur->pNext;
        }
        MsOS_ReleaseSemaphore(s_semaphore);
}

void MsOS_TimerInit(void)
{
    s_semaphore = MsOS_CreateSemaphore(1);
    s_pTimerHead = 0;
    s_taskId=MsOS_CreateTask((TaskEntry)TimerProcess,0, E_TASK_PRI_MEDIUM, &s_taskStop,0,"TimerTask");
}
