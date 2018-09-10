/*
 * semApi.h
 *
 *  Created on: 2018-6-24
 *      Author: tys
 */

#ifndef SEMAPI_H_
#define SEMAPI_H_

#ifdef __cplusplus
extern "C" {
#endif

struct semSet;
typedef unsigned int SEM_ID;	//the descriptor of the semaphore
typedef void (*semWait)(struct semSet* pSem);	//the functions is used to waiting the semaphore
typedef void (*semRelease)(struct semSet* pSem);	//the functions is used to waiting the semaphore
typedef void (*semDestroy)(struct semSet* pSem);	//the functions is used to destroy the semaphore

typedef struct semSet
{
	semWait semTake;	//It is used to take the semaphore
	semRelease semGive;	//It is used to release the semaphore
	semDestroy semFree;	//It is used to free the semaphore object
	SEM_ID semId;	//It is used to descript the semaphore
}semSet_t;

typedef semSet_t semM_t;
typedef semSet_t semC_t;

/* It is used to initialize the conditional semaphore object */
semC_t* semCMalloc();

/* It is used to initialize the mutex semaphore object */
semM_t* semMMalloc();

#ifdef __cplusplus
}
#endif

#endif /* SEMAPI_H_ */
