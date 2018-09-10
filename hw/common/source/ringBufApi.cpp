/*
 * ringBufApi.cpp
 *
 *  Created on: 2018-6-24
 *      Author: tys
 */

#include "ringBufApi.h"
#include "ringbuf.h"
#include "mList.h"

static unsigned int readAvailable(struct ringbufM* pM)
{
	int ret = 0;
	if (pM)
	{
		pM->mutex->semTake(pM->mutex);
		ret = ringBufAvailableBytesForRead((ringbuf_t *)pM->id);
		pM->mutex->semGive(pM->mutex);
	}

	return ret;
}

static unsigned int writeAvailable(struct ringbufM* pM)
{
	int ret = 0;
	if (pM)
	{
		pM->mutex->semTake(pM->mutex);
		ret = ringBufAvailableBytesForWrite((ringbuf_t *)pM->id);
		pM->mutex->semGive(pM->mutex);
	}

	return ret;
}

static unsigned int write(struct ringbufM* pM, const void* pData, unsigned int length)
{
	int ret = 0;
	if (pM)
	{
		pM->mutex->semTake(pM->mutex);
		ret = ringBufWrite((ringbuf_t *)pM->id, pData, length);
		pM->mutex->semGive(pM->mutex);
	}

	return ret;
}

static unsigned int read(struct ringbufM* pM, void* pData, unsigned int length)
{
	int ret = 0;
	if (pM)
	{
		pM->mutex->semTake(pM->mutex);
		ret = ringBufRead((ringbuf_t *)pM->id, pData, length);
		pM->mutex->semGive(pM->mutex);
	}

	return ret;
}

static unsigned int clone(struct ringbufM* pM, void* pData, unsigned int length)
{
	int ret = 0;
	if (pM)
	{
		pM->mutex->semTake(pM->mutex);
		ret = ringBufClone((ringbuf_t *)pM->id, pData, length);
		pM->mutex->semGive(pM->mutex);
	}

	return ret;
}

static void clear(struct ringbufM* pM)
{
	if (pM)
	{
		pM->mutex->semTake(pM->mutex);
		ringBufClear((ringbuf_t *)pM->id);
		pM->mutex->semGive(pM->mutex);
	}
}

static void destroy(struct ringbufM* pM)
{
	if (pM)
	{
		pM->mutex->semTake(pM->mutex);
		ringBufFree((ringbuf_t *)pM->id);
		pM->mutex->semGive(pM->mutex);
		if (pM->mutex)
			pM->mutex->semFree(pM->mutex);

		free(pM);
	}
}

ringbufM* ringBufMMalloc(unsigned int len)
{
	ringbufM* pM = NULL;
	do
	{
		pM = (ringbufM *)calloc(1, sizeof(ringbufM));
		if (NULL == pM)
			break;

		pM->id = (RINGBUF_ID)ringBufMalloc();
		if  (pM->id == 0)
			break;

		int ret = ringBufInit((ringbuf_t *)pM->id, len);
		if (ret < 0)
			break;

		pM->mutex = semMMalloc();
		if (NULL == pM->mutex)
			break;

		pM->write = write;
		pM->read = read;
		pM->writeAvailable = writeAvailable;
		pM->readAvailable = readAvailable;
		pM->clear = clear;
		pM->clone = clone;
		pM->free = destroy;

		return pM;
	}while (0);

	if (pM)
	{
		if (pM->id)
			ringBufFree((ringbuf_t *)pM->id);

		free(pM);
	}

	return NULL;
}

