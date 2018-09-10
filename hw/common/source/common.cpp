/*
 * common.cpp
 *
 *  Created on: 2018-8-13
 *      Author: tys
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <locale.h>

#include "common.h"

void dump_data1(unsigned char* p_data, int data_len)
{
	int i = 0;
	for (; i < data_len; i ++)
	{
		if ((i % 16) == 0 && i != 0)
			printf("\n");

		printf("%02x ", p_data[i]);
	}

	printf("\n");
}

void dump_data1Ex(unsigned char* p_data, int data_len)
{
	int i = 0;
	for (; i < data_len; i ++)
	{
		if ((i % 16) == 0 && i != 0)
			printf("\n");

		printf("%2x ", p_data[i]);
	}

	printf("\n");
}

static inline void _wait(unsigned long second, unsigned long usecond)
{
	struct timeval waitTime = {0};
	waitTime.tv_sec = second;
	waitTime.tv_usec = usecond;
	select(0, NULL, NULL, NULL, &waitTime);
}

void _usleep(unsigned long usecond)
{
	_wait(0, usecond);
}

void _msleep(unsigned long msecond)
{
	_wait(0, msecond*1000);
}

void _sleep(unsigned long second)
{
	_wait(second, 0);
}

unsigned int doCrc32(const unsigned char* pData, unsigned int len)
{
	unsigned int crc = 0xFFFFFFFF;
	if (NULL == pData || len == 0)
		return crc;

	while (len--)
	{
		crc ^= *pData++;
		int i = 0;
		for (i = 0; i < 8; i++)
		{
			crc = (crc >> 1) ^ ((crc & 0x1) ? CRCPOLY_LE : 0);
		}
	}

	return crc;
}

unsigned int doCom2H(const unsigned char* pIn, unsigned int length, unsigned char** pOut)
{
	int ret = 0;

	do
	{
		if (NULL == pIn || length < PKT_SERIAL_HEADER_LEN)
			break;

		if (pIn[0] != 0x55 || pIn[1] != 0xaa)
			break;

		unsigned short payloadLen = *(unsigned short *)(pIn+2);
		if (payloadLen + sizeof(int) != length)
			break;

		/* Calculate the length of the data area which contain crc
		 * data area length = payloadLen - startSign - endSign */
		int primLen = payloadLen - 2*sizeof(unsigned short);
		unsigned char* pTemp = (unsigned char*)calloc(1, primLen);
		*pOut = pTemp;

		int dataOffset = 2*sizeof(char) + sizeof(short) + 2* sizeof(char);
		unsigned char* pData = (unsigned char*)(pIn + dataOffset);
		int i = 0;
		for (i = 0; i < primLen; i++)
		{
			if (i+1 < primLen)
			{
				if (pData[i] == 0x7d)
				{
					if (pData[i+1] == 0x5e)
					{
						*pTemp++ = 0x7e;
						i++;
					}
					else if (pData[i+1] == 0x5d)
					{
						*pTemp++ = 0x7d;
						i++;
					}
					else
						*pTemp++ = 0x7d;
				}
				else
					*pTemp++ = pData[i];
			}
			else
				*pTemp++ = pData[i];
		}

		ret = pTemp - *pOut - sizeof(unsigned int);
		unsigned int crc = ~doCrc32(*pOut, ret);
		if (crc != *(unsigned int *)(pTemp - sizeof(unsigned int)))
		{
			free(*pOut);
			*pOut = NULL;
			ret = 0;
		}
	}while (0);

	return ret;
}

unsigned int doH2Com(const unsigned char* pIn, unsigned int length, unsigned char** pOut)
{
	if (NULL == pIn || length == 0)
		return 0;

	unsigned int crc = ~doCrc32(pIn, length);
	unsigned char* pPayload = (unsigned char *)calloc(1, length + sizeof(unsigned int));
	memcpy(pPayload, pIn, length);
	*(unsigned int *)(pPayload + length) = crc;

	unsigned int outMaxLen = PKT_SERIAL_HEADER_LEN + 2*PKT_SERIAL_CRC_LEN + length*2;
	unsigned char* pTemp = (unsigned char *)calloc(1, outMaxLen);
	*pOut = pTemp;

	*pTemp++ = 0x55;
	*pTemp++ = 0xaa;

	pTemp+= sizeof(short);

	*pTemp++ = 0x7e;
	*pTemp++ = 0x7e;

	int i = 0;
	for (i = 0; i < length; i++)
	{
		if (pPayload[i] == 0x7d || pPayload[i] == 0x7e)
		{
			*pTemp++ = 0x7d;
			*pTemp++ = 0x50 | (pPayload[i] & 0xf);
		}
		else
			*pTemp++ = pPayload[i];
	}

	*pTemp++ = 0x7e;
	*pTemp++ = 0x7e;

	int actualLen = pTemp - *pOut;
	*(unsigned short *)(*pOut + sizeof(short)) = actualLen - sizeof(int);

	free(pPayload);
	return actualLen;
}

static int codeConvert(const char* fromCharSet, const char* toCharSet, const char* pSrc, char** pDst)
{
	int ret = -1;
	wchar_t *pUnicodeStr = NULL;
	do
	{
		if (NULL == fromCharSet || NULL == toCharSet)
			break;

		if (NULL == pSrc || NULL == pDst)
			break;

		if (NULL == setlocale(LC_ALL, fromCharSet))
		{
			printf("Error: set local charSet<%s> failed.\r\n", fromCharSet);
			break;
		}

		int unicodeLen = mbstowcs(NULL, pSrc, 0);
		if (unicodeLen <= 0)
		{
			printf("Error: cann't transfer src to local.\r\n");
			break;
		}

	    pUnicodeStr = (wchar_t *) calloc(sizeof(wchar_t), unicodeLen + 1);
	    mbstowcs(pUnicodeStr, pSrc, strlen(pSrc));

		if (NULL == setlocale(LC_ALL, toCharSet))
		{
			printf("Error: set local charSet<%s> failed.\r\n", toCharSet);
			break;
		}

		int dstCodeLen = wcstombs(NULL, pUnicodeStr, 0);
		if (dstCodeLen <= 0)
		{
			printf("Error: cann't transfer local to dst charSet.\r\n");
			break;
		}

		*pDst = (char *)calloc(1, dstCodeLen);
		wcstombs(*pDst, pUnicodeStr, dstCodeLen);
		ret = dstCodeLen;
	}while (0);

	if (ret < 0 && pUnicodeStr)
		free(pUnicodeStr);

	return ret;
}

int fromUtf8ToGBK(const char* pSrc, char** pDst)
{
	return codeConvert("zh_CN.UTF-8", "zh_cn.GBK", pSrc, pDst);
}

int fromGBK2Utf8(const char* pSrc, char** pDst)
{
	return codeConvert("zh_cn.GBK", "zh_CN.UTF-8", pSrc, pDst);
}
