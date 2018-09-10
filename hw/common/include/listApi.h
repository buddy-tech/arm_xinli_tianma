/*
 * listApi.h
 *
 *  Created on: 2018-6-24
 *      Author: tys
 */

#ifndef LISTAPI_H_
#define LISTAPI_H_

#include "semApi.h"
#include "mList.h"

#ifdef __cplusplus
extern "C" {
#endif

struct dataAreaM;

typedef unsigned int DATA_ID;
typedef void* (*dataAreaMData)(struct dataAreaM* pM);	//read the data pointer
typedef unsigned int (*dataAreaMSize)(struct dataAreaM* pM);	//read the data size of the pointer
typedef void (*dataAreaMDestroy)(struct dataAreaM* pM);	//

/* It is used to manage the data area */
typedef struct dataAreaM
{
	dataAreaMData data;
	dataAreaMSize size;
	dataAreaMDestroy free;
	DATA_ID id;
}dataAreaM_t;

/* It is used to allocate a data area with data length */
dataAreaM_t* dataAreaMMalloc(unsigned int len);

struct nodeM;

typedef dataAreaM_t* (*nodeMDataArea)(struct nodeM* pM);	//It is used to obtain the data area
typedef void (*nodeMDestroy)(struct nodeM* pM);	//It is used to destroy the nodeM object.

/* It is used to manage the node of the list */
typedef struct nodeM
{
	node_t index;
	dataAreaM_t* pData;
	nodeMDataArea data;
	nodeMDestroy free;
}nodeM_t;

/* It is used to allocate a node which will be added into node-list */
nodeM_t* nodeMMalloc(unsigned int len);

struct listM;
typedef nodeM_t* (*_takeFirst)(struct listM* pM);	//It is used to take nodeM from list
typedef int (*_add2Tail)(struct listM* pM, nodeM_t* pData);	//It is used to add nodeM into list
typedef void (*_waitAvailableNode)(struct listM* pM);	//It is used to wait for available node added into list.
typedef void (*_endWaitAvailableNode)(struct listM* pM);
typedef void (*_listMDestroy)(struct listM* pM);	//It is used to destroy the listM object.

typedef struct listM
{
	semM_t* mutex;	//the lock of the list, which is used to protect list
	semC_t* signal;	//the conditional signal between producer and consumer
	nodeM_t* head;	//the head of the list.
	_takeFirst takeFirst;
	_add2Tail add2Tail;
	_waitAvailableNode wait;
	_endWaitAvailableNode end;
	_listMDestroy free;
}listM_t;

/* It is used to initialize the list manage */
listM_t* listMMalloc();

#ifdef __cplusplus
}
#endif
#endif /* LISTAPI_H_ */
