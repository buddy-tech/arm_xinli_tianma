/*
 * pwrSerial.cpp
 *
 *  Created on: 2018-7-21
 *      Author: tys
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "common.h"
#include "pwrSerial.h"
#include "serialDevApi.h"

static int onePktParse(void* pData, unsigned int length)
{
	char* pIn = (char *)pData;
	int ret = -1;

	do
	{
		if (NULL == pIn)
			break;

		if (length < sizeof(unsigned int))
		{
			ret = length + 1;
			break;
		}

		if (pIn[0] != 0x55 || pIn[1] != 0xaa)
			break;

		unsigned short payloadLen = *(unsigned short *)(pIn + 2);
		ret = payloadLen + sizeof(unsigned int);
	}while (0);

	return ret;
}

static _pwrRcvProcess pwrProcFunc = NULL;

static void pwrSerialDataProcess(void* pData, unsigned int length)
{
	if (NULL == pData || 0 == length)
		return;

	unsigned char* pDataArea = NULL;
	int bytes = doCom2H((const unsigned char*)pData, length, &pDataArea);
	if (bytes <= 0)
	{
		printf("doCom2H error...\r\n");
		return;
	}

	pwrProcFunc(pDataArea, bytes);
}

static serialDevM* pSgSerialDevM = NULL;

void pwrSerialInit(_pwrRcvProcess process)
{
	serialDevMParam_t param;
	strcpy(param.devName, "/dev/ttyPS1");
	param.parse = onePktParse;
	param.process = pwrSerialDataProcess;
	param.rate = E_B115200;
	param.dataBits = E_D_8_BIT;
	param.stopBits = E_S_1_BIT;
	param.parity = E_0_PARITY;
	pSgSerialDevM = serialDevMMalloc(param);

	pwrProcFunc = process;
}

void pwrSerialDeinit()
{
	if (pSgSerialDevM)
		pSgSerialDevM->free(pSgSerialDevM);
}

int pwrSnd2Serial(void* pData, unsigned int length)
{
	if (NULL == pData || length == 0)
	{
		printf("error:%s, line-%d\r\n", __FUNCTION__, __LINE__);
		return -1;
	}

	unsigned char* pOut = NULL;
	int bytes = doH2Com((const unsigned char*)pData, length, &pOut);
	bytes = pSgSerialDevM->send(pSgSerialDevM, pOut, bytes);
	free(pOut);
	return bytes;
}
