/*
 * semSet.h
 *
 *  Created on: 2018-6-20
 *      Author: tys
 */

#ifndef SEMSET_H_
#define SEMSET_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct semCon
{
	pthread_mutex_t mutex;
	pthread_cond_t conSignal;
}semCon_t;

static inline void semCRelease(semCon_t* pSemC);

static inline semCon_t* semCInit()
{
	semCon_t* pSemC = (semCon_t *)calloc(1, sizeof(semCon_t));
	if (NULL == pSemC)
		return NULL;

	pthread_mutex_init(&pSemC->mutex, NULL);
	pthread_cond_init(&pSemC->conSignal, NULL);

	return pSemC;
}

static inline void semCDeinit(semCon_t* pSemC)
{
	if (pSemC)
	{
		semCRelease(pSemC);
		pthread_mutex_lock(&pSemC->mutex);
		free(pSemC);
	}
}

static inline void semCWait(semCon_t* pSemC)
{
	if (pSemC)
	{
		pthread_mutex_lock(&pSemC->mutex);
#if 0
		struct timeval now = {0};
		gettimeofday(&now, NULL);

		struct timespec timeout = {0};
		timeout.tv_sec = now.tv_sec + 2;
		timeout.tv_nsec = now.tv_usec * 1000;

		pthread_cond_timedwait(&pSemC->conSignal, &pSemC->mutex, &timeout);
#else
		pthread_cond_wait(&pSemC->conSignal, &pSemC->mutex);
#endif
		pthread_mutex_unlock(&pSemC->mutex);
	}
}

static inline void semCRelease(semCon_t* pSemC)
{
	if (pSemC)
		pthread_cond_signal(&pSemC->conSignal);
}

typedef struct semMutex
{
	pthread_mutex_t mutex;
}semMutex_t;

static inline void semMRelease(semMutex_t* pSemM);

static inline semMutex_t* semMInit()
{
	semMutex_t* pSemM = (semMutex_t *)calloc(1, sizeof(semMutex_t));
	if (NULL == pSemM)
		return NULL;

	pthread_mutex_init(&pSemM->mutex, NULL);

	return pSemM;
}

static inline void semMDeinit(semMutex_t* pSemM)
{
	if (pSemM)
	{
		semMRelease(pSemM);
		pthread_mutex_lock(&pSemM->mutex);
		free(pSemM);
	}
}

static inline void semMWait(semMutex_t* pSemM)
{
	if (pSemM)
		pthread_mutex_lock(&pSemM->mutex);
}

static inline void semMRelease(semMutex_t* pSemM)
{
	if (pSemM)
		pthread_mutex_unlock(&pSemM->mutex);
}

#ifdef __cplusplus
}
#endif

#endif /* SEMSET_H_ */
