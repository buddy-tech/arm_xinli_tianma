/*
 * ringBufApi.h
 *
 *  Created on: 2018-6-24
 *      Author: tys
 */

#ifndef RINGBUFAPI_H_
#define RINGBUFAPI_H_

#include "semApi.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ringbufM;
typedef unsigned int RINGBUF_ID;	//the descriptor of the ring buffer
typedef unsigned int (*availableBytesForRead)(struct ringbufM* pM);//the functions is used to check how many bytes is available in the ring buffer for read
typedef unsigned int (*availableBytesForWrite)(struct ringbufM* pM);//the functions is used to check how many bytes is available in the ring buffer for write
typedef unsigned int (*ringbufMWriteData)(struct ringbufM* pM, const void* pData, unsigned int length);
typedef unsigned int (*ringbufMReadData)(struct ringbufM* pM, void* pData, unsigned int length);
typedef unsigned int (*ringbufMCloneData)(struct ringbufM* pM, void* pData, unsigned int length);
typedef void (*ringbufMClearData)(struct ringbufM* pM);	//the functions is used to clear data of the ring buffer
typedef void (*ringbufMDestroy)(struct ringbufM* pM);//the functions is used to destroy the ring buffer

typedef struct ringbufM
{
	availableBytesForRead readAvailable;
	availableBytesForWrite writeAvailable;
	ringbufMWriteData write;
	ringbufMReadData read;
	ringbufMCloneData clone;
	ringbufMClearData clear;
	ringbufMDestroy free;
	RINGBUF_ID id;	//the descriptor of the ring buffer
	semM_t* mutex;
}ringbufM_t;

/* It is used to initialize the conditional semaphore object */
ringbufM* ringBufMMalloc(unsigned int len);

#ifdef __cplusplus
}
#endif

#endif /* RINGBUFAPI_H_ */
