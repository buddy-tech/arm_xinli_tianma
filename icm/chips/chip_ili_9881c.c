#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "mipi2828.h"
#include "../ic_manager.h"

#define ILI_9881C_ENABLE_OTP		(1)

typedef struct tag_ili_9881c_info
{
	unsigned char id[3];

	unsigned char id_ok;	// 0: id_error; 1: id_ok;
	
	unsigned int vcom1;
	unsigned int  vcom1_otp_times;
	
	unsigned int vcom2;
	unsigned int  vcom2_otp_times;
}ili_9881c_info_t;

static ili_9881c_info_t s_ili_9881c_private_info[DEV_CHANNEL_NUM] = { 0 };

void ili_9881c_set_page(const DEV_CHANNEL_E channel, unsigned char page_no)
{
	unsigned char params[] = {0x98, 0x81};
	mipi2828Write2IC(channel, 0xFF, params, sizeof(params));
}

static unsigned char ili_9881c_parse_vcom_otp_times(unsigned char org_value)
{
	unsigned char otp_times = 3; 
	switch (org_value)
	{
		case 0:
			org_value = 0;
			break;

		case 1:
			org_value = 1;
			break;

		case 3:
			org_value = 2;
			break;

		case 7:
			org_value = 3;
			break;
	}

	return org_value;
}

int ili_9881c_get_vcom_otp_times(DEV_CHANNEL_E channel, unsigned char *p_vcom1_otp_times,
										unsigned char *p_vcom2_otp_times)
{
	ili_9881c_set_page(channel, 1);

	unsigned char reg = { 0 };
	ReadModReg(channel, 1, 0xE8, 1, &reg);

	unsigned char vcom1_otp_times = reg & 0x07;
	unsigned char vcom2_otp_times = (reg >> 3) & 0x07;

	*p_vcom1_otp_times = ili_9881c_parse_vcom_otp_times(vcom1_otp_times);
	*p_vcom2_otp_times = ili_9881c_parse_vcom_otp_times(vcom2_otp_times);
	
	DBG("ili_9881c_get_vcom_otp_times: vcom1: %02x => %d times, vcom2: %02x => %d times.\n",
			vcom1_otp_times, *p_vcom1_otp_times, vcom2_otp_times, *p_vcom2_otp_times);
	return 0;
}

int ili_9881c_read_vcom(DEV_CHANNEL_E channel, int index, unsigned int *p_vcom)
{
	if (p_vcom == NULL)
	{
		DBG_ERR("ili_9881c_read_vcom: invalid p_vcom = NULL.\n");
		return -1;
	}
	
	if ( (index < 0) || (index > 1))
	{
		DBG_ERR("ili_9881c_read_vcom: invalid index = %d.\n", index);
		return -1;
	}
	
	ili_9881c_set_page(channel, 1);

	unsigned char reg[2] = { 0 };
	unsigned char data[2] = { 0 };

	if (index == 0)
	{
		reg[0] = 0x52;
		reg[1] = 0x53;
	}
	else
	{
		reg[0] = 0x54;
		reg[1] = 0x55;
	}
	
	ReadModReg(channel, 1, reg[0], 1, data);
	ReadModReg(channel, 1, reg[1], 1, data + 1);

	*p_vcom = data[0] << 8 | data[1];
	
	return 0;
}

static int ili_9881c_write_vcom(DEV_CHANNEL_E channel, unsigned int vcom_value)
{	
	ili_9881c_set_page(channel, 1);

	unsigned char param = (vcom_value >> 8) & 0x1;
	mipi2828Write2IC(channel, 0x52, &param, sizeof(param));

	param = (vcom_value) & 0xff;
	mipi2828Write2IC(channel, 0x53, &param, sizeof(param));

	param = 0x00;
	mipi2828Write2IC(channel, 0x56, &param, sizeof(param));

	DBG("set vcom: channel = %d, vcom = %d, 0x%0x.\n", channel, vcom_value, vcom_value);
	return 0;
}

int ili_9881c_read_nvm_vcom(DEV_CHANNEL_E channel, int index, unsigned int *p_vcom_otp_value)
{
	ili_9881c_read_vcom(channel, index, p_vcom_otp_value);
	
	return 0;
}

void ili_9881c_write(DEV_CHANNEL_E channel, int addr, int val)
{
	unsigned char param = val&0xff;
	mipi2828Write2IC(channel, addr, &param, sizeof(param));
}

void ili9881c_page(DEV_CHANNEL_E channel, int page)
{
	unsigned char params[] = {0x98, 0x81, 0x00};
	params[2] = page & 0xff;
	mipi2828Write2IC(channel, 0xFF, params, sizeof(params));
}

static void ili9881c_reset(DEV_CHANNEL_E channel)
{
	ili9881c_page(channel, 0);
	mipi2828Write2IC(channel, 0x01, NULL, 0);
}

static void ili9881c_sleep_in(DEV_CHANNEL_E channel)
{
	ili9881c_page(channel, 0);
	mipi2828Write2IC(channel, 0x10, NULL, 0);
}

static void ili9881c_sleep_out(DEV_CHANNEL_E channel)
{
	ili9881c_page(channel, 0);
	mipi2828Write2IC(channel, 0x11, NULL, 0);
}

int ili_9881c_burn_vcom(DEV_CHANNEL_E channel, int vcom)
{
	DBG("ili9881c_vcom_otp. vcom = %d.\n", vcom);
	ili9881c_reset(channel);
	mipiIcReset(channel);

	//10ms
	_msleep(10);

	//D7
	ili9881c_page(channel, 4);

	//Power mode 2A
	unsigned char param = 0x2A;
	mipi2828Write2IC(channel, 0x6E, &param, sizeof(param));
	param = 0x35;
	mipi2828Write2IC(channel, 0x6F, &param, sizeof(param));
	param = 0x04;
	mipi2828Write2IC(channel, 0xD7, &param, sizeof(param));
	param = 0xEB;
	mipi2828Write2IC(channel, 0x8B, &param, sizeof(param));

	//[Page 0 / R11h]: Sleep Out
	//ili9881c_page(channel, 0);
	ili9881c_sleep_out(channel);
	
	//120ms
	usleep(1000 * 120); 

	//[Page xx]: Set NV Memory Programming Data
	ili9881c_page(channel, 1);

	param = vcom;
	mipi2828Write2IC(channel, 0xE0, &param, sizeof(param));

	param = 0x05;
	mipi2828Write2IC(channel, 0xE1, &param, sizeof(param));

	param = 0x00;
	mipi2828Write2IC(channel, 0xE2, &param, sizeof(param));
	
	DBG("burn vcom1: 0x%02x.=====\n", vcom, vcom);

	//NV Memory Protection Key
	ili9881c_page(channel, 1);
	param = 0x55;
	mipi2828Write2IC(channel, 0xE3, &param, sizeof(param));

	param = 0xAA;
	mipi2828Write2IC(channel, 0xE4, &param, sizeof(param));

	param = 0x66;
	mipi2828Write2IC(channel, 0xE5, &param, sizeof(param));
	
	//Max: Wait 1s
	_msleep(500);

	//Sleep In (Page 0 / R10h)
	//ili9881c_page(channel, 0);
	ili9881c_sleep_in(channel);
	
	//120ms
	_msleep(120);

	ili9881c_reset(channel);
	mipiIcReset(channel);
}

int chip_ili_9881c_get_chip_id(const DEV_CHANNEL_E channel, unsigned char *p_id_data, int id_data_len)
{
	char reg[3] = { 0 };
    ili_9881c_set_page(channel, 0);
	ili_9881c_set_page(channel, 1);

	// read id4
    ReadModReg(channel, 1, 0x00, 1, &reg[0]);
	ReadModReg(channel, 1, 0x01, 1, &reg[1]);
	ReadModReg(channel, 1, 0x02, 1, &reg[2]);
	DBG("chip channel: %d, channel: %d, read id4: %02x:%02x:%02x. \n",
				channel, channel, reg[0], reg[1], reg[2]);

	if (reg[0] == 0x98 && reg[1] == 0x81 && reg[2] == 0x5C)
	{
		if (p_id_data && id_data_len >= 3)
		{
			p_id_data[0] = reg[0];
			p_id_data[1] = reg[1];
			p_id_data[2] = reg[2];
			return 3;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		printf("chip channel: %d, channel: %d, chip_ili_9881c_get_chip_id: read id failed!\n",
				channel, channel);
		
		return -1;
	}
	
	return -1;
}


static int chip_ili_9881c_check_chip_ok(DEV_CHANNEL_E channel)
{
	printf("chip_ili_9881c_check_chip_ok: channel = %d.\n", channel);
	// read and check id;
	chip_ili_9881c_get_chip_id(channel, s_ili_9881c_private_info[channel].id,
								sizeof(s_ili_9881c_private_info[channel].id));

	unsigned char* p_id = s_ili_9881c_private_info[channel].id;
	if ( (p_id[0] != 0x98) || (p_id[1] != 0x81) || (p_id[2] != 0x5C) )
	{
		printf("chip_ili_9881c_check_chip_ok error: Invalid chip id: %02x, %02x, %02x.\n",
					p_id[0], p_id[1], p_id[2]);
		
		s_ili_9881c_private_info[channel].id_ok = 0;

		return -1;
	}

	s_ili_9881c_private_info[channel].id_ok = 1;

	// read vcom otp times
	unsigned char vcom1_otp_times = 0;
	unsigned char vcom2_otp_times = 0;
	ili_9881c_get_vcom_otp_times(channel, &vcom1_otp_times, &vcom2_otp_times);
	s_ili_9881c_private_info[channel].vcom1_otp_times = vcom1_otp_times;
	s_ili_9881c_private_info[channel].vcom2_otp_times = vcom2_otp_times;

	// read vcom otp value
	unsigned int vcom1_otp_value = 0;
	unsigned int vcom2_otp_value = 0;
	ili_9881c_read_nvm_vcom(channel, 0, &vcom1_otp_value);
	ili_9881c_read_nvm_vcom(channel, 1, &vcom2_otp_value);
	s_ili_9881c_private_info[channel].vcom1 = vcom1_otp_value;
	s_ili_9881c_private_info[channel].vcom2 = vcom2_otp_value;

	printf("chip_ili_9881c_check_chip_ok:chip:%d, mipi: %d. vcom1_otp_times: %d, vcom2_otp_times: %d. "
			"vcom1_otp_value: %d[%x]. vcom2_otp_value: %d[%x].\n", channel, channel,
				s_ili_9881c_private_info[channel].vcom1_otp_times,
				s_ili_9881c_private_info[channel].vcom2_otp_times,
				s_ili_9881c_private_info[channel].vcom1,s_ili_9881c_private_info[channel].vcom1,
				s_ili_9881c_private_info[channel].vcom2,s_ili_9881c_private_info[channel].vcom2);
	return 0;
}

static int chip_ili_9881c_read_chip_vcom_opt_times(DEV_CHANNEL_E channel, int* p_otp_vcom_times)
{
	if (s_ili_9881c_private_info[channel].id_ok == 1)
	{
		// read vcom otp times
		unsigned char vcom1_otp_times = 0;
		unsigned char vcom2_otp_times = 0;
		ili_9881c_get_vcom_otp_times(channel, &vcom1_otp_times, &vcom2_otp_times);

		*p_otp_vcom_times = vcom1_otp_times;

		printf("chip_ili_9881c_read_chip_vcom_opt_times: end\n");

		return 0;
	}
	
	printf("chip_ili_9881c_read_chip_vcom_opt_times error!\n");
	return -1;
}


static int chip_ili_9881c_read_chip_vcom_opt_info(DEV_CHANNEL_E channel, int* p_otp_vcom_times,
												int *p_otp_vcom_value)
{
	if (s_ili_9881c_private_info[channel].id_ok == 1)
	{
		// read vcom otp times
		unsigned char vcom1_otp_times = 0;
		unsigned char vcom2_otp_times = 0;
		ili_9881c_get_vcom_otp_times(channel, &vcom1_otp_times, &vcom2_otp_times);

		*p_otp_vcom_times = vcom1_otp_times;

		unsigned int vcom = 0;
		ili_9881c_read_vcom(channel, 0, &vcom);

		*p_otp_vcom_value = vcom;

		printf("chip_ili_9881c_read_chip_vcom_opt_info: vcom = %d end\n", *p_otp_vcom_value);

		return 0;
	}
	
	printf("chip_ili_9881c_read_chip_vcom_opt_info error!\n");
	return -1;
}


static int chip_ili_9881c_write_chip_vcom_otp_value(DEV_CHANNEL_E channel, int otp_vcom_value)
{
	if (s_ili_9881c_private_info[channel].id_ok == 1)
	{
		int val = 0;
		val = ili_9881c_burn_vcom(channel, otp_vcom_value);
		return val;
	}

	printf("chip_ili_9881c_write_chip_vcom_otp_value error!\n");
	return -1;
}


static int chip_ili_9881c_read_vcom(DEV_CHANNEL_E channel, int* p_vcom_value)
{
	if (s_ili_9881c_private_info[channel].id_ok == 1)
	{
		int vcom = 0;
		ili_9881c_read_vcom(channel, 0, &vcom);
		*p_vcom_value = vcom;
		//printf("chip_ili_9881c_read_vcom: end\n");

		return 0;
	}
	printf("chip_ili_9881c_read_vcom error: unknow chip!\n");
	return -1;
}

static int chip_ili_9881c_write_vcom(DEV_CHANNEL_E channel, int vcom_value)
{
	if (s_ili_9881c_private_info[channel].id_ok == 1)
	{
		if (s_ili_9881c_private_info[channel].vcom1_otp_times == 0)
		{
			ili_9881c_write_vcom(channel, vcom_value);
		}
		else
		{
			ili_9881c_write_vcom(channel, vcom_value);
		}
		
		//printf("chip_ili_9881c_read_vcom: end\n");

		return 0;
	}
	
	printf("chip_ili_9881c_write_vcom error: unknow chip!\n");
	
	return -1;
}

static void chip_ili_9881c_get_and_reset_private_data(DEV_CHANNEL_E channel)
{
	printf("chip_ili_9881c_get_and_reset_private_data.\n");
	s_ili_9881c_private_info[channel].id_ok = 0;
	memset(s_ili_9881c_private_info[channel].id, 0, sizeof(s_ili_9881c_private_info[channel].id));
	s_ili_9881c_private_info[channel].vcom1 = 0;
	s_ili_9881c_private_info[channel].vcom1_otp_times = 0;
	s_ili_9881c_private_info[channel].vcom2 = 0;
	s_ili_9881c_private_info[channel].vcom2_otp_times = 0;
}


static int chip_ili_9881c_check_vcom_otp_burn_ok(DEV_CHANNEL_E channel, int vcom_value, int last_otp_times, int *p_read_vcom)
{
	printf("chip_ili_9881c_check_vcom_otp_burn_ok: last times: %d. last vcom: %d. \n", last_otp_times, vcom_value);
	// check id 
	int val = chip_ili_9881c_check_chip_ok(channel);
	if (val != 0)
	{
		printf("chip_icn_9706_check_vcom_otp_burn_ok error: check id error, val = %d.\n", val);
		return E_ICM_READ_ID_ERROR;
	}

	if (p_read_vcom)
		*p_read_vcom = s_ili_9881c_private_info[channel].vcom1;
	
	#if 1
	// check times
	if (s_ili_9881c_private_info[channel].vcom1_otp_times != last_otp_times + 1)
	{
		printf("chip_icn_9706_check_vcom_otp_burn_ok error: check vcom otp times error. last times: %d, now: %d.\n", 
				last_otp_times, s_ili_9881c_private_info[channel].vcom1_otp_times);

	#ifdef ENABlE_OTP_BURN
		return E_ICM_VCOM_TIMES_NOT_CHANGE;
	#else
		return E_ICM_OK;
	#endif
	}
	#endif
	
	// check vcom value.
	if (s_ili_9881c_private_info[channel].vcom1 != vcom_value)
	{
		printf("chip_ili_9881c_check_vcom_otp_burn_ok error: check vcom value error. write: %d, read: %d.\n", 
				vcom_value, s_ili_9881c_private_info[channel].vcom1);

		return E_ICM_VCOM_VALUE_NOT_CHANGE;
	}
	
	return E_ICM_OK;
}

static icm_chip_t chip_ili_9881c =
{
	.name = "ili9881c",

	.reset_private_data = chip_ili_9881c_get_and_reset_private_data,
	
	.id = chip_ili_9881c_get_chip_id,
	.check_ok = chip_ili_9881c_check_chip_ok,
	.read_vcom_otp_times = chip_ili_9881c_read_chip_vcom_opt_times,
	.read_vcom_otp_info = chip_ili_9881c_read_chip_vcom_opt_info,
	
	.read_vcom = chip_ili_9881c_read_vcom,
	.write_vcom = chip_ili_9881c_write_vcom,
	
	.write_vcom_otp_value = chip_ili_9881c_write_chip_vcom_otp_value,
	.check_vcom_otp_burn_ok = chip_ili_9881c_check_vcom_otp_burn_ok,
};

void ili9881cInit()
{
	registerIcChip(&chip_ili_9881c);
}
