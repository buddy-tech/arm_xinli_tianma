/*
 * listApi.cpp
 *
 *  Created on: 2018-6-26
 *      Author: tys
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "listApi.h"
#include "dataArea.h"

static void* data(dataAreaM_t* pM)
{
	if (NULL == pM)
		return NULL;

	return dataAreaData((dataArea_t *)pM->id);
}

static unsigned int size(dataAreaM_t* pM)
{
	if (NULL == pM)
		return 0;

	return dataAreaSize((dataArea_t *)pM->id);
}

static void destroy(dataAreaM_t* pM)
{
	if (pM)
	{
		dataAreaFree((dataArea_t *)pM->id);
		free(pM);
	}
}

/* It is used to allocate a data area with data length */
dataAreaM_t* dataAreaMMalloc(unsigned int len)
{
	dataAreaM_t* pM = NULL;
	do
	{
		if (len == 0)
			break;

		pM = (dataAreaM_t *)calloc(1,sizeof(dataAreaM_t));
		if (NULL == pM)
			break;

		pM->id = (DATA_ID)dataAreaMalloc(len);
		if (pM->id == 0)
			break;

		pM->data = data;
		pM->size = size;
		pM->free = destroy;

		return pM;
	}while (0);

	if (pM)
	{
		free(pM);
	}

	return NULL;
}

static dataAreaM_t* nodeData(struct nodeM* pM)
{
	if (NULL == pM)
		return NULL;

	return pM->pData;
}

static void nodeDestroy(struct nodeM* pM)
{
	if (pM)
	{
		if (pM->pData)
			pM->pData->free(pM->pData);

		free(pM);
	}
}

/* It is used to allocate a node which will be added into node-list */
nodeM_t* nodeMMalloc(unsigned int len)
{
	nodeM_t* pM = NULL;
	do
	{
		if (len == 0)
			break;

		pM = (nodeM_t *)calloc(1, sizeof(nodeM_t));
		if (NULL == pM)
			break;

		pM->pData = dataAreaMMalloc(len);
		if (NULL == pM->pData)
			break;

		init_list_head(&pM->index);

		pM->data = nodeData;
		pM->free = nodeDestroy;
		return pM;
	}while (0);

	if (pM)
	{
		if (pM->pData)
			pM->pData->free(pM->pData);

		free(pM);
	}

	return NULL;
}

static nodeM_t* takeFirst(struct listM* pM)
{
	nodeM_t* pNodeM = NULL;
	do
	{
		if (NULL == pM || NULL == pM->head)
			break;

		pM->mutex->semTake(pM->mutex);
		node_t* pNode = list_take_first(&pM->head->index);
		if (pNode)
			pNodeM = containter_of(pNode, nodeM_t, index);

		pM->mutex->semGive(pM->mutex);
	}while (0);

	return pNodeM;
}

static int add2Tail(struct listM* pM, nodeM_t* pData)
{
	if (NULL == pM || NULL == pData || NULL == pM->head)
		return -1;

	pM->mutex->semTake(pM->mutex);
	list_add_tail(&pData->index, &pM->head->index);
	pM->mutex->semGive(pM->mutex);
	pM->signal->semGive(pM->signal);
	return 0;
}

static void waitAvailableNode(struct listM* pM)
{
	if (NULL == pM || NULL == pM->head)
		return;

	pM->signal->semTake(pM->signal);
}

static void endWaitAvailableNode(struct listM* pM)
{
	if (NULL == pM || NULL == pM->head)
		return;

	pM->signal->semGive(pM->signal);
}

static void listMFree(struct listM* pM)
{
	if (pM)
	{
		pM->mutex->semTake(pM->mutex);
		pM->head->free(pM->head);
		pM->mutex->semGive(pM->mutex);

		pM->mutex->semFree(pM->mutex);

		pM->signal->semFree(pM->signal);

		free(pM);
	}
}

/* It is used to initialize the list manage */
listM_t* listMMalloc()
{
	listM_t* pM = NULL;
	do
	{
		pM = (listM_t *)calloc(1, sizeof(listM_t));
		if (NULL == pM)
			break;

		pM->mutex = semMMalloc();
		if (NULL == pM->mutex)
			break;

		pM->signal = semCMalloc();
		if (NULL == pM->signal)
			break;

		pM->head = nodeMMalloc(4);
		if (NULL == pM->head)
			break;

		pM->takeFirst = takeFirst;
		pM->add2Tail = add2Tail;
		pM->free = listMFree;
		pM->wait = waitAvailableNode;
		pM->end = endWaitAvailableNode;
		return pM;
	}while (0);

	if (pM)
	{
		if (pM->mutex)
			pM->mutex->semFree(pM->mutex);

		if (pM->signal)
			pM->signal->semFree(pM->signal);

		if (pM->head)
			pM->head->free(pM->head);

		free(pM);
	}

	return NULL;
}
