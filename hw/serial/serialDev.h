/*
 * serialDev.h
 *
 *  Created on: 2018-7-6
 *      Author: tys
 */

#ifndef SERIALDEV_H_
#define SERIALDEV_H_

#include <pthread.h>

#include "ringBufApi.h"
#include "listApi.h"

#ifndef __cplusplus
extern "C" {
#endif

#define RING_BUF_SIZE	(512*1024)	//512Kbit

struct serialDev;

typedef int (*_serialPktParse)(void* pData, unsigned int length);	//It is used to parse the data area contain a valid packet
typedef void (*_serialProcess)(void* pData, unsigned int length);	//It is used to process Serial data
typedef void (*_serialDevFree)(struct serialDev* pM);
typedef int (*_serialDevSend)(struct serialDev* pM, void* pData, unsigned int length);
typedef void (*_serialDevSetBaud)(struct serialDev* pM, unsigned int rate);
typedef void (*_serialDevSetStopBits)(struct serialDev* pM, unsigned int stopBits);
typedef void (*_serialDevSetParity)(struct serialDev* pM, unsigned int parity);
typedef void (*_serialDevSetDataBits)(struct serialDev* pM, unsigned int dataBits);

typedef struct serialDev
{
	int fd;
	bool bRcvStarted;
	pthread_t rcvTaskId;	// the received data task id
	bool bProcStarted;
	pthread_t proTaskId;	// the data process task id
	bool bSndStarted;
	pthread_t sndTaskId;		// the task id of send data out.
	ringbufM_t* pRcvBuf;	// the received data buffer
	listM_t* pListM;		// the data which need be process.
	listM_t* pSndListM;		// the data list for sending out.
	_serialDevSetBaud setBaud;
	_serialDevSetStopBits setStopBits;
	_serialDevSetParity setParity;
	_serialDevSetDataBits setDataBits;
	_serialDevFree free;
	_serialDevSend send;
	_serialPktParse onePktParse;
	_serialProcess serialProcess;
}serialDev_t;

unsigned int serialDevMalloc(const char* devName, _serialProcess process, _serialPktParse parse);

#ifndef __cplusplus
}
#endif

#endif /* SERIALDEV_H_ */
