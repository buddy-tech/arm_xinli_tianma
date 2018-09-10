/*
 * chip_hx_8298c.c
 *
 *  Created on: 2018-8-30
 *      Author: cxy
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
#include "hi_unf_gpio.h"

//static icm_chip_t chip_hx8298c = {0};

typedef struct tag_hx8298c_info
{
	unsigned char id;

	unsigned char id_ok;
	
	unsigned int vcom;
	unsigned int  vcom_otp_times;
}hx8298c_info_t;

static hx8298c_info_t s_hx8298c_private_info[DEV_CHANNEL_NUM] = {0};

static void chip_hx8298c_spi_config(unsigned int channel)
{	
	SPI_CTRL_ID ctrlId = vspiCtrlInstance(channel);

	vspiCtrlSetMode(ctrlId, WIRE_3_LINE);
	vspiCtrlSetByteFormat(ctrlId, BYTE_8_BIT);

	vspiCtrlSetCsState(ctrlId, false);
	_msleep(1);
	vspiCtrlSetCsState(ctrlId, true);
}

static void chip_hx8298c_start_write_2_ic_reg(unsigned int channel, unsigned int addr)
{
	SPI_CTRL_ID ctrlId = vspiCtrlInstance(channel);

	chip_hx8298c_spi_config(channel);
	vspiCtrlWrite(ctrlId, addr);
}

static void chip_hx8298c_stop_write_2_ic_reg(unsigned int channel)
{
	SPI_CTRL_ID ctrlId = vspiCtrlInstance(channel);
	vspiCtrlSetCsState(ctrlId, false);
}


static void chip_hx8298c_send_reg_param_2_ic(unsigned int channel, unsigned int value)
{
	SPI_CTRL_ID ctrlId = vspiCtrlInstance(channel);
	vspiCtrlWrite(ctrlId, value);
}


static void parseInitCodeAndProcess(DEV_CHANNEL_E channel, const char* pInitCode)
{
	int i,j;
	cJSON* pRoot = cJSON_Parse(pInitCode);
	if (NULL == pRoot)
		return;

	do
	{
		cJSON* pCodeArea = cJSON_GetObjectItem(pRoot, "codeArea");
		if (NULL == pCodeArea)
			break;

		int lineArraySize = cJSON_GetArraySize(pCodeArea);
		for (i = 0; i < lineArraySize; i++)
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
						chip_hx8298c_start_write_2_ic_reg(channel, regAddr);

						cJSON* pParamsObj = cJSON_GetObjectItem(pLineObj, "params");
						if (NULL == pParamsObj)
							continue;

						int paramsSize = cJSON_GetArraySize(pParamsObj);
						for (j = 0; j < paramsSize; j++)
						{
							cJSON* pParamObj = cJSON_GetArrayItem(pParamsObj, j);
							if (NULL == pParamObj)
								continue;

							unsigned int param = pParamObj->valueint;
							chip_hx8298c_send_reg_param_2_ic(channel, param);
						}

						chip_hx8298c_stop_write_2_ic_reg(channel);
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

static void chip_hx8298c_set_page(DEV_CHANNEL_E channel,unsigned char pageNo)
{
	chip_hx8298c_start_write_2_ic_reg(channel,0x00);//address
	chip_hx8298c_send_reg_param_2_ic(channel,pageNo);
	chip_hx8298c_stop_write_2_ic_reg(channel);
}

static void chip_hx8298c_start_read_ic_reg(DEV_CHANNEL_E channel,unsigned char addr)
{
	SPI_CTRL_ID ctrlId = vspiCtrlInstance(channel);

	chip_hx8298c_spi_config(channel);
	vspiCtrlWrite(ctrlId,addr);
}

static unsigned char chip_hx8298c_read_reg_from_ic(DEV_CHANNEL_E channel)
{
	unsigned int value = 0;
	SPI_CTRL_ID ctrId = vspiCtrlInstance(channel);
	vspiCtrlRead(ctrId,&value);

	return (unsigned char)value;
}

static void chip_hx8298c_stop_read_ic_reg(DEV_CHANNEL_E channel)
{
	SPI_CTRL_ID ctrId = vspiCtrlInstance(channel);
	vspiCtrlSetCsState(ctrId,false);
}

static unsigned char hx8298c_read_vcom(DEV_CHANNEL_E channel)
{
	unsigned char value= 0;
		
	chip_hx8298c_set_page(channel,1);
	chip_hx8298c_start_read_ic_reg(channel,0x1A);
	value = chip_hx8298c_read_reg_from_ic(channel);
	chip_hx8298c_stop_read_ic_reg(channel);

	return value;
}

static unsigned char chip_hx8298c_read_vcom(DEV_CHANNEL_E channel, int* p_vcom_value)
{
	if(s_hx8298c_private_info[channel].id_ok == 1)
	{
		int vcom = 0;
		vcom = hx8298c_read_vcom(channel);
		*p_vcom_value = vcom;
		
		return 0;
	}

	printf("chip_hx8298c_read_vcom error: unknow chip!\n");
	return -1;
}

static int chip_hx8298c_get_chip_id(DEV_CHANNEL_E channel, unsigned char *p_id_data)
{
	int value = 0;
	chip_hx8298c_set_page(channel,0);

	chip_hx8298c_start_read_ic_reg(channel,0x1e);
	value = chip_hx8298c_read_reg_from_ic(channel);
	chip_hx8298c_stop_read_ic_reg(channel);

	*p_id_data = value;
	return 0;
}

static int chip_hx8298c_read_vcom_otp_times(DEV_CHANNEL_E channel,unsigned char *p_vcom_otp_times)
{
	unsigned char reg[2] = {0};
	
	chip_hx8298c_set_page(channel,1);
	
	chip_hx8298c_start_read_ic_reg(channel,0x1D);
	reg[0] = chip_hx8298c_read_reg_from_ic(channel);
	chip_hx8298c_stop_read_ic_reg(channel);
	
	chip_hx8298c_start_read_ic_reg(channel,0x1E);
	reg[1] = chip_hx8298c_read_reg_from_ic(channel);
	chip_hx8298c_stop_read_ic_reg(channel);

	*p_vcom_otp_times = reg[0] | (reg[1] << 8);

	return 0;
}

static int chip_hx8298c_read_vcom_otp_info(DEV_CHANNEL_E channel, int* p_otp_vcom_times,
												int *p_otp_vcom_value)
{
	if (s_hx8298c_private_info[channel].id_ok == 1)
	{
		// read vcom otp times
		unsigned char vcom_otp_times = 0;
		chip_hx8298c_read_vcom_otp_times(channel,&vcom_otp_times);

		*p_otp_vcom_times = vcom_otp_times;

		unsigned int vcom = 0;
		chip_hx8298c_read_vcom(channel,&vcom);

		*p_otp_vcom_value = vcom;

		printf("chip_hx8298c_read_chip_vcom_opt_info: vcom = %d end\n", *p_otp_vcom_value);

		return 0;
	}
	
	printf("chip_hx8298c_read_chip_vcom_opt_info error!\n");
	return -1;
}

static int chip_hx8298c_check_chip_ok(DEV_CHANNEL_E channel)
{
	chip_hx8298c_get_chip_id(channel,s_hx8298c_private_info[channel].id);
	
	unsigned char p_id = s_hx8298c_private_info[channel].id;
	if(p_id != 0x98)
	{
		printf("chip_hx8298c_check_chip_ok error: Invalid chip id: %02x\n",p_id);
		
		s_hx8298c_private_info[channel].id_ok = 0;
		return -1;
	}

	s_hx8298c_private_info[channel].id_ok = 1;

	// read vcom otp times
	unsigned char vcom_otp_times = 0;
	chip_hx8298c_read_vcom_otp_times(channel,&vcom_otp_times);
	s_hx8298c_private_info[channel].vcom_otp_times = vcom_otp_times;

	// read vcom otp value
	unsigned int vcom_otp_value = 0;
	chip_hx8298c_read_vcom(channel,&vcom_otp_value);
	s_hx8298c_private_info[channel].vcom = vcom_otp_value;

	printf("chip_hx8298c_check_chip_ok:chip:%d, mipi: %d. vcom_otp_times: %d. "
			"vcom_otp_value: %d[%x].\n", channel, channel,
				s_hx8298c_private_info[channel].vcom_otp_times,
				s_hx8298c_private_info[channel].vcom,s_hx8298c_private_info[channel].vcom);
	return 0;
}

static void setIcWriteStatus(DEV_CHANNEL_E channel,char enable)
{
	if(channel = 0 || channel == 1)
	{
		if(enable)
			gpio_set_output_value(TTL_GPIO_PIN_UD_L,0);
		else
			gpio_set_output_value(TTL_GPIO_PIN_UD_L,1);
	}
	else
	{
		if(enable)
			gpio_set_output_value(TTL_GPIO_PIN_UD_R,0);
		else
			gpio_set_output_value(TTL_GPIO_PIN_UD_R,1);
	}
}

static int hx8298c_write_vcom(DEV_CHANNEL_E channel,unsigned char value)
{
	setIcWriteStatus(channel,1);
	
	chip_hx8298c_set_page(channel,1);
	
	chip_hx8298c_start_write_2_ic_reg(channel,0x14);
	chip_hx8298c_send_reg_param_2_ic(channel,0x91);
	chip_hx8298c_stop_write_2_ic_reg(channel);
	
	chip_hx8298c_start_write_2_ic_reg(channel,0x1A);
	chip_hx8298c_send_reg_param_2_ic(channel,value);
	chip_hx8298c_stop_write_2_ic_reg(channel);
	
	setIcWriteStatus(channel,0);

	return 0;
}

static void chip_hx8298c_write_vcom(DEV_CHANNEL_E channel,unsigned char value)
{
	if(s_hx8298c_private_info[channel].id_ok == 1)
	{
		hx8298c_write_vcom(channel,value);
		return 0;
	}
	
	printf("chip_hx8298c_write_vcom error: unknow chip!\n");
	return -1;
}


static void chip_hx8298c_write_vcom_otp_value(DEV_CHANNEL_E channel,unsigned char value)
{
	unsigned char vcom = 0;
	chip_hx8298c_set_page(channel,9);

	chip_hx8298c_start_write_2_ic_reg(channel,0x02);//address
	chip_hx8298c_send_reg_param_2_ic(channel,0x66);//enable otp program password
	chip_hx8298c_stop_write_2_ic_reg(channel);
	
	_msleep(10);

	chip_hx8298c_start_write_2_ic_reg(channel,0x01);//address
	chip_hx8298c_send_reg_param_2_ic(channel,0x08);//setting vcom group
	chip_hx8298c_stop_write_2_ic_reg(channel);

	chip_hx8298c_write_vcom(channel,value);

	chip_hx8298c_start_write_2_ic_reg(channel,0x03);//address
	chip_hx8298c_send_reg_param_2_ic(channel,0x01);//enable otp
	chip_hx8298c_stop_write_2_ic_reg(channel);
	
	_msleep(13);
	
	chip_hx8298c_start_read_ic_reg(channel,0x0F);
	vcom = chip_hx8298c_read_reg_from_ic(channel);//read otp flag
	chip_hx8298c_stop_read_ic_reg(channel);
	if(0 != value)
		printf("Error: vcom burn error \r\n");

	chip_hx8298c_start_write_2_ic_reg(channel,0x02);//address
	chip_hx8298c_send_reg_param_2_ic(channel,0x00);//disable otp program password
	chip_hx8298c_stop_write_2_ic_reg(channel);
}

static int chip_hx8298c_check_vcom_otp_burn_ok(DEV_CHANNEL_E channel, int vcom_value, int last_otp_times, int *p_read_vcom)
{
	printf("chip_hx8298c_check_vcom_otp_burn_ok: last times: %d. last vcom: %d. \n", last_otp_times, vcom_value);
	// check id 
	int val = chip_hx8298c_check_chip_ok(channel);
	if (val != 0)
	{
		printf("chip_hx8298c_check_vcom_otp_burn_ok error: check id error, val = %d.\n", val);
		return E_ICM_READ_ID_ERROR;
	}

	if (p_read_vcom)
		*p_read_vcom = s_hx8298c_private_info[channel].vcom;
	
	#if 1
	// check times
	if (s_hx8298c_private_info[channel].vcom_otp_times != last_otp_times + 1)
	{
		printf("chip_icn_9706_check_vcom_otp_burn_ok error: check vcom otp times error. last times: %d, now: %d.\n", 
				last_otp_times, s_hx8298c_private_info[channel].vcom_otp_times);

	#ifdef ENABlE_OTP_BURN
		return E_ICM_VCOM_TIMES_NOT_CHANGE;
	#else
		return E_ICM_OK;
	#endif
	}
	#endif
	
	// check vcom value.
	if (s_hx8298c_private_info[channel].vcom != vcom_value)
	{
		printf("chip_hx8298c_check_vcom_otp_burn_ok error: check vcom value error. write: %d, read: %d.\n", 
				vcom_value, s_hx8298c_private_info[channel].vcom);

		return E_ICM_VCOM_VALUE_NOT_CHANGE;
	}
	
	return E_ICM_OK;
}

static icm_chip_t chip_hx8298c = 
{
	.name = "hx8298c",
	.snd_init_code = sndInitCode,

	.id = chip_hx8298c_get_chip_id,
	.check_ok = chip_hx8298c_check_chip_ok,
	.read_vcom_otp_times = chip_hx8298c_read_vcom_otp_times,
	.read_vcom_otp_info = chip_hx8298c_read_vcom_otp_info,

	.read_vcom = chip_hx8298c_read_vcom,
	.write_vcom = chip_hx8298c_write_vcom,

	.write_vcom_otp_value = chip_hx8298c_write_vcom_otp_value,
	.check_vcom_otp_burn_ok = chip_hx8298c_check_vcom_otp_burn_ok,
};

void hx8298cChipInit()
{
//	strcpy(chip_hx8298c.name, "hx8298c");
//	chip_hx8298c.snd_init_code = sndInitCode;
	
	registerIcChip(&chip_hx8298c);
}
