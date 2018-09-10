/*
 * chip_s1d19k08.cpp
 *
 *  Created on: 2018-8-15
 *      Author: tys
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "common.h"

#include "hi_unf_spi.h"
#include "ic_manager.h"
#include "cJson.h"

static icm_chip_t chip_s1d19k08 = {0};

#define TRANSLATE_2_CMD(_value) (((_value)&0xff) | (0x0 << 8))
#define TRANSLATE_2_PARAM(_value) (((_value)&0xff) | (0x1 << 8))

void sndData2Ic(unsigned int channel, unsigned int value)
{
	SPI_CTRL_ID ctrlId = vspiCtrlInstance(channel);

	vspiCtrlSetMode(ctrlId, WIRE_4_LINE);
	vspiCtrlSetByteFormat(ctrlId, BYTE_9_BIT);

	vspiCtrlSetCsState(ctrlId, false);
	_msleep(1);
	vspiCtrlSetCsState(ctrlId, true);
	vspiCtrlWrite(ctrlId, value);
	vspiCtrlSetCsState(ctrlId, false);
}

static void parseInitCodeAndProcess(DEV_CHANNEL_E channel, const char* pInitCode)
{
	cJSON* pRoot = cJSON_Parse(pInitCode);
	if (NULL == pRoot)
		return;

	do
	{
		cJSON* pCodeArea = cJSON_GetObjectItem(pRoot, "codeArea");
		if (NULL == pCodeArea)
			break;

		int lineArraySize = cJSON_GetArraySize(pCodeArea);
		for (int i = 0; i < lineArraySize; i++)
		{
			cJSON* pLineObj = cJSON_GetArrayItem(pCodeArea, i);
			if (pLineObj)
			{
				cJSON* pSignObj = cJSON_GetObjectItem(pLineObj, "sign");
				if (pSignObj)
				{
					if (NULL == pSignObj->valuestring)
						continue;

					if (strcasecmp(pSignObj->valuestring, "isReg") == 0)
					{
						cJSON* pRegObj = cJSON_GetObjectItem(pLineObj, "regAddr");
						if (NULL == pRegObj)
							continue;

						unsigned int regAddr = pRegObj->valueint;
						sndData2Ic(channel, TRANSLATE_2_CMD(regAddr));

						cJSON* pParamsObj = cJSON_GetObjectItem(pLineObj, "params");
						if (NULL == pParamsObj)
							continue;

						int paramsSize = cJSON_GetArraySize(pParamsObj);
						for (int i = 0; i < paramsSize; i++)
						{
							cJSON* pParamObj = cJSON_GetArrayItem(pParamsObj, i);
							if (NULL == pParamObj)
								continue;

							unsigned int param = pParamObj->valueint;
							sndData2Ic(channel, TRANSLATE_2_PARAM(param));
						}
					}
					else if (strcasecmp(pSignObj->valuestring, "isDelay") == 0)
					{
						cJSON* pDelayObj = cJSON_GetObjectItem(pLineObj, "delay");
						if (pDelayObj)
							_msleep(pDelayObj->valueint);
					}
				}
			}
		}

	}while (0);

	cJSON_Delete(pRoot);
}

static DEV_CHANNEL_E translateDevChannel2Local(DEV_CHANNEL_E channel)
{
	if (channel == DEV_CHANNEL_1)
		return DEV_CHANNEL_3;
	else
		return channel;
}

static void sndInitCode(DEV_CHANNEL_E channel, const char* pInitCodeName)
{
	if (NULL == pInitCodeName)
		return;

	char codeName[128] = "";
	sprintf(codeName, "%s%s", INITCODE_PATH, pInitCodeName);

	int fd = open(codeName, O_RDONLY);
	if (fd < 0)
		return;

	char* pInitCode = (char *)calloc(1, INIT_CODE_MAX_LEN);
	read(fd, pInitCode, INIT_CODE_MAX_LEN);
	close(fd);

	channel = translateDevChannel2Local(channel);

	parseInitCodeAndProcess(channel, pInitCode);
	free(pInitCode);
}

void s1d19k08ChipInit()
{
	strcpy(chip_s1d19k08.name, "s1d19k08");
	chip_s1d19k08.snd_init_code = sndInitCode;
	registerIcChip(&chip_s1d19k08);
}
