/*
 * semApi.cpp
 *
 *  Created on: 2018-6-24
 *      Author: tys
 */

#include "semApi.h"
#include "semSet.h"
#include "mList.h"


static void semCTake(struct semSet* pSem)
{
	if (pSem)
		semCWait((semCon_t*)pSem->semId);
}

static void semCGive(struct semSet* pSem)
{
	if (pSem)
		semCRelease((semCon_t*)pSem->semId);
}

static void semCFree(struct semSet* pSem)
{
	if (pSem)
	{
		semCDeinit((semCon_t*)pSem->semId);
		free(pSem);
	}
}

semC_t* semCMalloc()
{
	semC_t* pSem = NULL;

	do
	{
		pSem = (semC_t *)calloc(1, sizeof(semC_t));
		if (NULL == pSem)
			break;

		pSem->semId = (SEM_ID)semCInit();
		if (pSem->semId == 0)
			break;

		pSem->semTake = semCTake;
		pSem->semGive = semCGive;
		pSem->semFree = semCFree;
		return pSem;
	}while (0);

	if (pSem)
		free(pSem);

	return NULL;
}

static void semMTake(struct semSet* pSem)
{
	if (pSem)
		semMWait((semMutex_t*)pSem->semId);
}

static void semMGive(struct semSet* pSem)
{
	if (pSem)
		semMRelease((semMutex_t*)pSem->semId);
}

static void semMFree(struct semSet* pSem)
{
	if (pSem)
	{
		semMDeinit((semMutex_t*)pSem->semId);
		free(pSem);
	}
}

semM_t* semMMalloc()
{
	semM_t* pSem = NULL;

	do
	{
		pSem = (semM_t *)calloc(1, sizeof(semM_t));
		if (NULL == pSem)
			break;

		pSem->semId = (SEM_ID)semMInit();
		if (pSem->semId == 0)
			break;

		pSem->semTake = semMTake;
		pSem->semGive = semMGive;
		pSem->semFree = semMFree;
		return pSem;
	}while (0);

	if (pSem)
		free(pSem);

	return NULL;
}
