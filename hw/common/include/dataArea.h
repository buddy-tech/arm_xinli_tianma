/*
 * dataArea.h
 *
 *  Created on: 2018-6-26
 *      Author: tys
 */

#ifndef DATAAREA_H_
#define DATAAREA_H_

#include <string.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dataArea
{
	unsigned int size;
	void* payload;
}dataArea_t;

static inline unsigned int dataAreaSize(dataArea_t* pM)
{
	if (NULL == pM)
		return 0;

	return pM->size;
}

static inline void* dataAreaData(dataArea_t* pM)
{
	if (NULL == pM)
		return NULL;

	return pM->payload;
}

static inline dataArea_t* dataAreaMalloc(unsigned int length)
{
	dataArea_t* pM = NULL;

	do
	{
		pM = (dataArea_t *)calloc(1, sizeof(dataArea_t));
		if (NULL == pM)
			break;

		pM->payload = calloc(1, length);
		if (NULL == pM->payload)
			break;

		pM->size = length;
		return pM;
	}while (0);

	if (pM)
		free(pM);

	return NULL;
}

static inline void dataAreaFree(dataArea_t* pM)
{
	if (pM)
	{
		if (pM->payload)
			free(pM->payload);

		free(pM);
	}
}

#ifdef __cplusplus
}
#endif

#endif /* DATAAREA_H_ */
