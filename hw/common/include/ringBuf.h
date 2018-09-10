/*
 * ringBuf.h
 *
 *  Created on: 2018-6-15
 *      Author: tys
 */

#ifndef RINGBUFMNG_H_
#define RINGBUFMNG_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/*_max: power(2,n); _w:next write position; _r:next read position */
#define RING_DATA_LEN(_max, _w, _r)	((_w) > (_r) ? (_w) - (_r) : (((_max) - (_r))^((_max) - (_w))))

typedef struct ringbuf
{
	unsigned int read;	//next read position
	unsigned int write;	//next write position
	unsigned int len;	//the length of the buffer
	void* pData;	//the buffer
}ringbuf_t;

/* It is used to allocate a ring buffer,
 * and return a pointer of the buffer
 */

static inline ringbuf_t* ringBufMalloc()
{
	ringbuf_t* pRingBuf = (ringbuf_t*)calloc(1, sizeof(ringbuf_t));
	return pRingBuf;
}

/* It is used to free a ring buffer */
static inline void ringBufFree(ringbuf_t* pRingBuf)
{
	if (NULL == pRingBuf)
		return;

	if (pRingBuf->pData)
		free(pRingBuf->pData);

	free(pRingBuf);
}

/**
 * fun: it is used to initialize the ring buffer
 *
 * @pRingBuf: the manager of the ring buffer.
 *
 * @length: the length will be used to added to ring buffer.
 *
 * */

static inline int ringBufInit(ringbuf_t* pRingBuf, unsigned int length)
{
	int ret = -1;

	do
	{
		if (length <= 2 || NULL == pRingBuf)
			break;

		pRingBuf->len = length;
		pRingBuf->read = 0;
		pRingBuf->write = 0;
		pRingBuf->pData = calloc(1, length);
		if (NULL == pRingBuf->pData)
			break;

		ret = 0;
	}while (0);

	return ret;
}

/*********
 * fun: It is used to clear all data in the ring buffer
 * */
static inline void ringBufClear(ringbuf_t* pRingBuf)
{
	if (pRingBuf)
	{
		pRingBuf->read = 0;
		pRingBuf->write = 0;
		memset(pRingBuf->pData, '\0', pRingBuf->len);
	}
}

/**
 * fun: It is used to obtain available bytes free for writing.
 *
 * */
static inline unsigned int ringBufAvailableBytesForWrite(ringbuf_t* pRingBuf)
{
	unsigned int availableBytes = 0;
	do
	{
		if (NULL == pRingBuf)
			break;

#if 1
		if (pRingBuf->write > pRingBuf->read)
			availableBytes = pRingBuf->len - pRingBuf->write + pRingBuf->read;
		else if (pRingBuf->write == pRingBuf->read)
			availableBytes = pRingBuf->len;
		else
			availableBytes = pRingBuf->read - pRingBuf->write;
#else
		availableBytes = pRingBuf->len - RING_DATA_LEN(pRingBuf->len, pRingBuf->write, pRingBuf->read);
#endif
	}while (0);

	return availableBytes;
}

static inline unsigned int ringBufAvailableBytesForRead(ringbuf_t* pRingBuf)
{
	unsigned int availableBytes = 0;
	do
	{
		if (NULL == pRingBuf)
			break;

#if 1
		if (pRingBuf->write > pRingBuf->read)
			availableBytes = pRingBuf->write - pRingBuf->read;
		else if (pRingBuf->write == pRingBuf->read)
			availableBytes = 0;
		else
			availableBytes = pRingBuf->len - pRingBuf->read + pRingBuf->write;
#else
		availableBytes = RING_DATA_LEN(pRingBuf->len, pRingBuf->write, pRingBuf->read);
#endif
	}while (0);

	return availableBytes;
}

/***
 * fun: It is used to write data into the ring buffer.
 *
 * @pRingBuf: the manager of the ring buffer.
 *
 * @pData: the pointer of the data which will be written into
 * the ring buffer.
 *
 * @length: the length of the data which will be written into
 * the ring buffer.
 *
 * return: the data length have been written into buffer.
 * */

static inline unsigned int ringBufWrite(ringbuf_t* pRingBuf, const void* pData, unsigned int length)
{
	if (ringBufAvailableBytesForWrite(pRingBuf) < length)	////Free buffer is not enough in ring buffer
		return 0;

	unsigned int nextWrite = pRingBuf->write + length;
	if (nextWrite >= pRingBuf->len)	//It will come to the head
	{
		unsigned int fstSegLen = pRingBuf->len - pRingBuf->write;
		unsigned int secSegLen = length - fstSegLen;
		memcpy((char *)pRingBuf->pData + pRingBuf->write, pData, fstSegLen);

		if (secSegLen > 0)
			memcpy((char *)pRingBuf->pData, ((char *)pData + fstSegLen), secSegLen);

		nextWrite = secSegLen;
	}
	else
		memcpy((char *)pRingBuf->pData + pRingBuf->write, pData, length);

	pRingBuf->write = nextWrite;	//update the write position

	return length;
}

/***
 * fun: It is used to read data from the ring buffer.
 *
 * @pRingBuf: the manager of the ring buffer.
 *
 * @pData: the pointer of the data which will be written data
 * which is read from ring buffer.
 *
 * @length: the length of the data which will be read from
 * the ring buffer.
 *
 * return: the data length have been read from ring buffer.
 * */

static inline unsigned int ringBufRead(ringbuf_t* pRingBuf, void* pData, unsigned int length)
{
	if (ringBufAvailableBytesForRead(pRingBuf) < length)	//Data is not enough in ring buffer
		return 0;

	unsigned int nextRead = pRingBuf->read + length;
	if (nextRead >= pRingBuf->len)
	{
		unsigned int fstSegLen = pRingBuf->len - pRingBuf->read;
		unsigned int secSegLen = length - fstSegLen;
		memcpy(pData, (char *)pRingBuf->pData + pRingBuf->read, fstSegLen);

		if (secSegLen > 0)
			memcpy((char *)pData + fstSegLen, (char *)pRingBuf->pData, secSegLen);

		nextRead = secSegLen;
	}
	else
		memcpy(pData, (char *)pRingBuf->pData + pRingBuf->read, length);

	pRingBuf->read = nextRead;	//update the read position

	return length;
}

/***
 * fun: It is used to read data from the ring buffer, which doesn't change
 * the attribute of the ring buffer.
 *
 * @pRingBuf: the manager of the ring buffer.
 *
 * @pData: the pointer of the data which will be written data
 * which is read from ring buffer.
 *
 * @length: the length of the data which will be read from
 * the ring buffer.
 *
 * return: the data length have been read from ring buffer.
 * */

static inline unsigned int ringBufClone(ringbuf_t* pRingBuf, void* pData, unsigned int length)
{
	if (ringBufAvailableBytesForRead(pRingBuf) < length)	//Data is not enough in ring buffer
		return 0;

	unsigned int nextRead = pRingBuf->read + length;
	if (nextRead >= pRingBuf->len)
	{
		unsigned int fstSegLen = pRingBuf->len - pRingBuf->read;
		unsigned int secSegLen = length - fstSegLen;
		memcpy(pData, (char *)pRingBuf->pData + pRingBuf->read, fstSegLen);

		if (secSegLen > 0)
			memcpy((char *)pData + fstSegLen, (char *)pRingBuf->pData, secSegLen);
	}
	else
		memcpy(pData, (char *)pRingBuf->pData + pRingBuf->read, length);

	return length;
}

#ifdef __cplusplus
}
#endif
#endif /* RINGBUFMNG_H_ */
