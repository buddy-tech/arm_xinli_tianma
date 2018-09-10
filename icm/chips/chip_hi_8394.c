#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "mipi2828.h"

#include "common.h"
#include "../ic_manager.h"

#define HX_8394_ANALOG_GAMMA_REG			(0xE0)

#define HX_8394_D3G_BANK_REG				(0xBD)
#define HX_8394_DIGITAL_GAMMA_REG			(0xC1)

#define HX_8394_ANALOG_GAMMA_LEN		(58)
#define HX_8394_D3G_R_REG_LEN			(43)
#define HX_8394_D3G_G_REG_LEN			(42)
#define HX_8394_D3G_B_REG_LEN			(42)
#define HX_8394_ALL_D3G_REG_LEN			(HX_8394_D3G_R_REG_LEN + HX_8394_D3G_G_REG_LEN \
												+ HX_8394_D3G_B_REG_LEN)

typedef struct tag_hx_8394_info
{
	unsigned char id[3];
	unsigned char id_ok;	// 0: id_error; 1: id_ok;
	unsigned int vcom1;
	unsigned int  vcom1_otp_times;
	unsigned int vcom2;
	unsigned int  vcom2_otp_times;
	unsigned char analog_gamma_reg_data[HX_8394_ANALOG_GAMMA_LEN];
	unsigned char digital_gamma_reg_data[3][HX_8394_D3G_R_REG_LEN];
}hx_8394_info_t;

static hx_8394_info_t s_hx_8394_private_info[DEV_CHANNEL_NUM] = { 0 };

void hi_8394_enable_extend_cmd(DEV_CHANNEL_E channel, int enable)
{
	// enable extend command
	if (enable)
	{
		unsigned char params[] =
		{
			0xFF, 0x83, 0x94,
		};

		mipi2828Write2IC(channel, 0xB9, params, sizeof(params));
	}
	else
	{
		unsigned char params[] =
		{
			0xFF, 0xFF, 0xFF,
		};

		mipi2828Write2IC(channel, 0xB9, params, sizeof(params));
	}
}

int hi_8394_get_vcom_otp_times(DEV_CHANNEL_E channel, unsigned char* p_vcom1_otp_times,
										unsigned char* p_vcom2_otp_times)
{
	unsigned char reg[3] = { 0 };

	// hardware reset.

	// delay 120 ms.

	// enable extend command
	hi_8394_enable_extend_cmd(channel, 1);
	
	ReadModReg(channel, 1, 0xB6, 3, reg);

	printf("hi_8394_get_vcom_otp_times: %02x, %02x, %02x.\n", reg[0], reg[1], reg[2]);

	reg[2] >>= 5;

	unsigned char otp_times = 0;
	switch (reg[2])
	{
		case 0x07:
			otp_times = 0;
			break;
		
		case 0x03:
			otp_times = 1;
			break;

		case 0x01: 
			otp_times = 2;
			break;

		case 0x00:
			otp_times = 3;
			break;

		default:
			printf("hi_8394_get_vcom_otp_times: invalid times = %d.\n", reg[2]);
			otp_times = 3;
			break;
	}

	*p_vcom1_otp_times = otp_times;
	*p_vcom2_otp_times = otp_times;
	
	printf("hi_8394_get_vcom_otp_times: channel: %d, vcom otp times: %d. \n", channel,
			*p_vcom1_otp_times);
	
	return 0;
}

int hi_8394_read_vcom(DEV_CHANNEL_E channel, int index, unsigned int *p_vcom)
{
	unsigned char reg[3] = { 0 };

	// enable extend command
	hi_8394_enable_extend_cmd(channel, 1);
	
	ReadModReg(channel, 1, 0xB6, 3, reg);
	printf("hi_8394_read_vcom: %02x, %02x, %02x.\n", reg[0], reg[1], reg[2]);

	if (index == 0)
	{
		//*p_vcom = reg[0] | ((reg[3] & 0x01) << 8);
		*p_vcom = reg[0];
	}
	else if (index == 1)
	{
		//*p_vcom = reg[1] | ((reg[3] & 0x02) << 8);
		*p_vcom = reg[1];
	}
	else
	{
		printf("hi_8394_read_vcom error: invalid index = %d.\n", index);
	}
	
	printf("hi_8394_read_vcom: channel: %d, vcom otp value: %#x. \n", channel,
			*p_vcom);
	return 0;
}

int hi_8394_write_vcom(DEV_CHANNEL_E channel, unsigned int vcom_value)
{
	unsigned char reg[3] = { 0 };

	reg[0] = vcom_value & 0xFF;
	reg[1] = vcom_value & 0xFF;

	if (vcom_value & 0x100)
	{
		reg[2] |= 0x01;
		reg[2] |= 0x02;
	}
	
	// enable extend command
	hi_8394_enable_extend_cmd(channel, 1);

	mipi2828Write2IC(channel, 0xB6, reg, sizeof(reg));
	
	return 0;
}

int hi_8394_read_nvm_vcom_z(DEV_CHANNEL_E channel, int index, unsigned int* p_vcom_otp_value)
{
	// hw reset
	mipi2828Write2IC(channel, 0x01, NULL, 0);

	// sleep out
	mipi2828Write2IC(channel, 0x11, NULL, 0);

	_msleep(120);
	
	// enable extend command
	hi_8394_enable_extend_cmd(channel, 1);

	// set otp index: 0xBB
	unsigned char otp_key[7] = { 0 };
	otp_key[1] = 0x0D;	// 0x0D -> 0x14.
	mipi2828Write2IC(channel, 0xBB, otp_key, sizeof(otp_key));

	// set otp por: 0xbb	
	otp_key[3] = 0x80;	// 0x0D -> 0x14.
	mipi2828Write2IC(channel, 0xBB, otp_key, sizeof(otp_key));

	// clear otp por: 0xbb	
	otp_key[3] = 0x00;	// 0x0D -> 0x14.
	mipi2828Write2IC(channel, 0xBB, otp_key, sizeof(otp_key));

	// read data: 0xbb
	unsigned char read_data[5] = { 0 };
	ReadModReg(channel, 1, 0xBB, sizeof(read_data), read_data);
	return 0;
}


static int hi_8394_read_nvm_vcom(DEV_CHANNEL_E channel, int index, unsigned int* p_vcom_otp_value)
{
	return 0;
}

int hi_8394_burn_vcom(DEV_CHANNEL_E channel, unsigned int otp_vcom_value)
{
	int use_internal_vpp = 1;

	DBG("hi_8394_burn_vcom: channel=%d, vcom=%d.\n", channel, otp_vcom_value);

	setMipi2828HighSpeedMode(channel, false);
	mipiIcReset(channel);
	mipi2828Write2IC(channel, 0x11, NULL, 0);
	_msleep(150);
	// password
	unsigned char passwd[] =
	{
		0xFF, 0x83, 0x94,
	};

	mipi2828Write2IC(channel, 0xB9, passwd, sizeof(passwd));
	// vcom
	unsigned char vcom_value[] =
	{
		0x00, 0x00, 0x00,
	};

	vcom_value[0] = otp_vcom_value;
	vcom_value[1] = otp_vcom_value;

	mipi2828Write2IC(channel, 0xB6, vcom_value, sizeof(vcom_value));
	_msleep(100);
	
	// otp key
	unsigned char otp_key[] =
	{
		0x00, 0x00, 0x00, 0x00, 0x00, 0xAA, 0x55,
	};

	mipi2828Write2IC(channel, 0xBB, otp_key, sizeof(otp_key));

	// interal power
	unsigned char otp_power[] =
	{
		0x80, 0x0D,
	};

	mipi2828Write2IC(channel, 0xBB, otp_power, sizeof(otp_power));

	unsigned char otp_power2[] =
	{
		0x80, 0x0D, 0x00, 0x01,
	};

	mipi2828Write2IC(channel, 0xBB, otp_power2, sizeof(otp_power2));
	
	_msleep(550);

	unsigned char otp_end[] =
	{
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	};

	mipi2828Write2IC(channel, 0xBB, otp_end, sizeof(otp_end));

	mipi2828Write2IC(channel, 0xB9, passwd, sizeof(passwd));

	mipi2828Write2IC(channel, 0x10, NULL, 0);
	
	setMipi2828HighSpeedMode(channel, true);
	return 0;
}

static int hi_8394_select_d3g_bank_r(DEV_CHANNEL_E channel)
{
	unsigned char param = 0x00;
	mipi2828Write2IC(channel, 0xBD, &param, sizeof(param));
}

static int hi_8394_select_d3g_bank_g(DEV_CHANNEL_E channel)
{
	unsigned char param = 0x01;
	mipi2828Write2IC(channel, 0xBD, &param, sizeof(param));
}

static int hi_8394_select_d3g_bank_b(DEV_CHANNEL_E channel)
{
	unsigned char param = 0x02;
	mipi2828Write2IC(channel, 0xBD, &param, sizeof(param));
}


int hx_8394_write_d3g_reg_2(DEV_CHANNEL_E channel, unsigned char *p_d3g_r_reg_data, int d3g_r_reg_data_len,
							unsigned char *p_d3g_g_reg_data, int d3g_g_reg_data_len,
							unsigned char *p_d3g_b_reg_data, int d3g_b_reg_data_len)
{
	// R
	unsigned char param = 0x00;
	mipi2828Write2IC(channel, 0xBD, &param, sizeof(param));
	mipi2828Write2IC(channel, 0xC1, p_d3g_r_reg_data, HX_8394_D3G_R_REG_LEN);

	// G
	param = 0x01;
	mipi2828Write2IC(channel, 0xBD, &param, sizeof(param));
	mipi2828Write2IC(channel, 0xC1, p_d3g_g_reg_data, HX_8394_D3G_G_REG_LEN);
	
	// B
	param = 0x02;
	mipi2828Write2IC(channel, 0xBD, &param, sizeof(param));
	mipi2828Write2IC(channel, 0xC1, p_d3g_b_reg_data, HX_8394_D3G_B_REG_LEN);
	return 0;
}
							

static int chip_hi_8394_get_chip_id(DEV_CHANNEL_E channel, unsigned char *p_id_data, int id_data_len)
{
	unsigned char reg[3];
	ReadModReg(channel, 1, 0x04, 3, reg);
	printf("chip_hi_8394_get_chip_id: channel: %d, read id4: %02x:%02x:%02x. \n",
			channel, reg[0], reg[1], reg[2]);

	if (reg[0] == 0x83 && reg[1] == 0x94 && reg[2] == 0x0f)
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

	printf("chip_hi_8394_get_chip_id: channel: %d, : read id failed!\n",channel);
	return -1;
}


static int chip_hi_8394_check_chip_ok(DEV_CHANNEL_E channel)
{
	chip_hi_8394_get_chip_id(channel, s_hx_8394_private_info[channel].id,
								sizeof(s_hx_8394_private_info[channel].id));

	unsigned char* p_id = s_hx_8394_private_info[channel].id;
	if ( (p_id[0] != 0x83) || (p_id[1] != 0x94) || (p_id[2] != 0x0F) )
	{
		printf("chip_hi_8394_check_chip_ok error: Invalid chip id: %02x, %02x, %02x.\n",
					p_id[0], p_id[1], p_id[2]);
		
		s_hx_8394_private_info[channel].id_ok = 0;
		return -1;
	}

	s_hx_8394_private_info[channel].id_ok = 1;

	// read vcom otp times
	unsigned char vcom1_otp_times = 0;
	unsigned char vcom2_otp_times = 0;
	hi_8394_get_vcom_otp_times(channel, &vcom1_otp_times, &vcom2_otp_times);
	s_hx_8394_private_info[channel].vcom1_otp_times = vcom1_otp_times;
	s_hx_8394_private_info[channel].vcom2_otp_times = vcom2_otp_times;

	// read vcom otp value
	unsigned int vcom1_otp_value = 0;
	unsigned int vcom2_otp_value = 0;

	hi_8394_read_vcom(channel, 0, &vcom1_otp_value);
	
	s_hx_8394_private_info[channel].vcom1 = vcom1_otp_value;
	s_hx_8394_private_info[channel].vcom2 = vcom1_otp_value;

	printf("chip_hi_8394_check_chip_ok:channel:%d. vcom1_otp_times: %d, vcom2_otp_times: %d."
			"vcom1_otp_value: %d. vcom2_otp_value: %d.\n", channel,
				s_hx_8394_private_info[channel].vcom1_otp_times,
				s_hx_8394_private_info[channel].vcom2_otp_times,
				s_hx_8394_private_info[channel].vcom1,
				s_hx_8394_private_info[channel].vcom2);
	return 0;
}

static int chip_hi_8394_read_chip_vcom_opt_times(DEV_CHANNEL_E channel, int* p_otp_vcom_times)
{
	if (s_hx_8394_private_info[channel].id_ok == 1)
	{
		// read vcom otp times
		unsigned char vcom1_otp_times = 0;
		unsigned char vcom2_otp_times = 0;
		hi_8394_get_vcom_otp_times(channel, &vcom1_otp_times, &vcom2_otp_times);

		*p_otp_vcom_times = vcom1_otp_times;

		printf("chip_hi_8394_read_chip_vcom_opt_times: end\n");

		return 0;
	}
	
	printf("chip_hi_8394_read_chip_vcom_opt_times error!\n");
	return -1;
}


static int chip_hi_8394_read_chip_vcom_opt_info(DEV_CHANNEL_E channel, int* p_otp_vcom_times,
												int *p_otp_vcom_value)
{
	if (s_hx_8394_private_info[channel].id_ok == 1)
	{
		// read vcom otp times
		unsigned char vcom1_otp_times = 0;
		unsigned char vcom2_otp_times = 0;
		hi_8394_get_vcom_otp_times(channel, &vcom1_otp_times, &vcom2_otp_times);

		// read vcom otp value
		int vcom1_otp_value = 0;
		int vcom2_otp_value = 0;
		hi_8394_read_nvm_vcom(channel, 0, &vcom1_otp_value);
		hi_8394_read_nvm_vcom(channel, 1, &vcom2_otp_value);

		*p_otp_vcom_times = vcom1_otp_times;
		*p_otp_vcom_value = vcom1_otp_value;

		printf("chip_hi_8394_read_chip_vcom_opt_info: end\n");

		return 0;
	}
	
	printf("chip_hi_8394_read_chip_vcom_opt_info error: unknow chip!\n");
	return -1;
}


static int chip_hi_8394_write_chip_vcom_otp_value(DEV_CHANNEL_E channel, int otp_vcom_value)
{
	if (s_hx_8394_private_info[channel].id_ok == 1)
	{
		int val = 0;
		val = hi_8394_burn_vcom(channel, otp_vcom_value);
		return val;
	}

	printf("chip_hi_8394_write_chip_vcom_otp_value error!\n");
	return -1;
}


static int chip_hi_8394_read_vcom(DEV_CHANNEL_E channel, int* p_vcom_value)
{
	if (s_hx_8394_private_info[channel].id_ok == 1)
	{
		int vcom = 0;
		hi_8394_read_vcom(channel, 0, &vcom);
		*p_vcom_value = vcom;
		//printf("chip_hi_8394_read_vcom: end\n");

		return 0;
	}
	printf("chip_hi_8394_read_vcom error: unknow chip!\n");
	return -1;
}


static int chip_hi_8394_write_vcom(DEV_CHANNEL_E channel, int vcom_value)
{
	if (s_hx_8394_private_info[channel].id_ok == 1)
	{
		if (s_hx_8394_private_info[channel].vcom1_otp_times == 0)
			hi_8394_write_vcom(channel, vcom_value);
		else
			hi_8394_write_vcom(channel, vcom_value);
		
		//printf("chip_hi_8394_write_vcom: end\n");

		return 0;
	}
	
	printf("chip_hi_8394_write_vcom error: unknow chip!\n");
	
	return -1;
}

static void chip_hi_8394_get_and_reset_private_data(DEV_CHANNEL_E channel)
{
	printf("chip_hi_8394_get_and_reset_private_data.\n");
	hx_8394_info_t* pInfo = &s_hx_8394_private_info[channel];
	memset(pInfo, '\0', sizeof(hx_8394_info_t));
}

static int chip_hi_8394_check_vcom_otp_burn_ok(DEV_CHANNEL_E channel, int vcom_value,
													int last_otp_times, int *p_read_vcom)
{
	// check id 
	int val = chip_hi_8394_check_chip_ok(channel);
	if (val != 0)
	{
		printf("chip_hi_8394_check_vcom_otp_burn_ok error: check id error, val = %d.\n", val);
		return E_ICM_READ_ID_ERROR;
	}

	if (p_read_vcom)
		*p_read_vcom = s_hx_8394_private_info[channel].vcom1;

	// check times
	if (s_hx_8394_private_info[channel].vcom1_otp_times != last_otp_times + 1)
	{
		printf("chip_hi_8394_check_vcom_otp_burn_ok error: check vcom otp times error. last times: %d, now: %d.\n", 
				last_otp_times, s_hx_8394_private_info[channel].vcom1_otp_times);

		return E_ICM_VCOM_TIMES_NOT_CHANGE;
	}

	// check vcom value.
	if (s_hx_8394_private_info[channel].vcom1 != vcom_value)
	{
		printf("chip_hi_8394_check_vcom_otp_burn_ok error: check vcom value error. write: %d, read: %d.\n", 
				vcom_value, s_hx_8394_private_info[channel].vcom1);

		return E_ICM_VCOM_VALUE_NOT_CHANGE;
	}

	return E_ICM_OK;
}

// gamma
static int chip_hi_8394_write_analog_reg(DEV_CHANNEL_E channel, unsigned char *p_analog_gamma_reg_data,
								int analog_gamma_data_len)
{
	setMipi2828HighSpeedMode(channel, false);
	mipi2828Write2IC(channel, HX_8394_ANALOG_GAMMA_REG, p_analog_gamma_reg_data, HX_8394_ANALOG_GAMMA_LEN);
	setMipi2828HighSpeedMode(channel, true);
	return 0;
}

int hx_8394_digital_gamma_control(DEV_CHANNEL_E channel, unsigned char value)
{
	setMipi2828HighSpeedMode(channel, false);
	hi_8394_enable_extend_cmd(channel, 1);
	hi_8394_select_d3g_bank_r(channel);

	mipi2828Write2IC(channel, HX_8394_D3G_BANK_REG, &value, sizeof(value));

	setMipi2828HighSpeedMode(channel, true);
	return 0;
}

int hx_8394_write_analog_gamma_reg_data(DEV_CHANNEL_E channel, unsigned char *p_analog_gamma_reg_data,
										int analog_gamma_reg_data_len)
{	
	hx_8394_info_t* p_chip_info = &s_hx_8394_private_info[channel];
	
	if ( (p_analog_gamma_reg_data == NULL) || (analog_gamma_reg_data_len <= 0) )
	{
		printf("hx_8394_write_analog_gamma_reg_data: Invalid params!\n");
		return -1;
	}
	
	memcpy(p_chip_info->analog_gamma_reg_data, p_analog_gamma_reg_data, HX_8394_ANALOG_GAMMA_LEN);

	setMipi2828HighSpeedMode(channel, false);
	
	hi_8394_enable_extend_cmd(channel, 1);

	chip_hi_8394_write_analog_reg(channel, p_chip_info->analog_gamma_reg_data,
								HX_8394_ANALOG_GAMMA_LEN);

	setMipi2828HighSpeedMode(channel, true);
	return 0;
}

static unsigned char s_analog_gamma_reg_data[HX_8394_ANALOG_GAMMA_LEN] = 
{ 
	0x00, 0x08, 0x16, 0x20, 0x24, 0x2a, 0x30, 0x30, 0x66, 0x7b, 0x8e, 0x8d, 0x94, 0xa1, 0xa2, 0xa2, 
	0xab, 0xad, 0xab, 0xb7, 0xc6, 0x63, 0x61, 0x65, 0x68, 0x6c, 0x73, 0x7b, 0x7f, 0x00, 0x08, 0x15, 
	0x20, 0x24, 0x2a, 0x30, 0x30, 0x66, 0x7b, 0x8c, 0x8b, 0x93, 0xa2, 0xa0, 0xa1, 0xac, 0xae, 0xab, 
	0xb9, 0xc9, 0x61, 0x60, 0x64, 0x68, 0x6c, 0x73, 0x7b, 0x7f
};
	
int hx_8394_write_fix_analog_gamma_reg_data(DEV_CHANNEL_E channel)
{
	
	setMipi2828HighSpeedMode(channel, false);
	
	hi_8394_enable_extend_cmd(channel, 1);

	chip_hi_8394_write_analog_reg(channel, s_analog_gamma_reg_data, HX_8394_ANALOG_GAMMA_LEN);

	// write d3g enable
	hx_8394_digital_gamma_control(channel, 0x00);

	setMipi2828HighSpeedMode(channel, true);
	return 0;
}

int hx_8394_read_analog_gamma_reg_data(DEV_CHANNEL_E channel, unsigned char *p_analog_gamma_reg_data,
										int analog_gamma_reg_data_len)
{
	setMipi2828HighSpeedMode(channel, false);
	
	hi_8394_enable_extend_cmd(channel, 1);
	
	ReadModReg(channel, 1, HX_8394_ANALOG_GAMMA_REG, HX_8394_ANALOG_GAMMA_LEN, p_analog_gamma_reg_data);

	printf("read alanog reg: 0x%02x, len: 0x%02x.\n", HX_8394_ANALOG_GAMMA_REG, HX_8394_ANALOG_GAMMA_LEN);
	dump_data1(p_analog_gamma_reg_data, HX_8394_ANALOG_GAMMA_LEN);

	setMipi2828HighSpeedMode(channel, true);
	return HX_8394_ANALOG_GAMMA_LEN;
}

static int hx_8394_write_d3g_control(DEV_CHANNEL_E channel, int enable)
{
	printf("hx_8394_write_d3g_control!\n");
	hx_8394_digital_gamma_control(channel, enable);
	return 0;
}
										
int hx_8394_write_analog_digital_gamma_reg_data(DEV_CHANNEL_E channel,unsigned char *p_digital_gamma_reg_data,
										int digital_gamma_reg_data_len)
{
	hx_8394_info_t* p_chip_info = &s_hx_8394_private_info[channel];
	
	if ( (p_digital_gamma_reg_data == NULL) || (digital_gamma_reg_data_len <= 0) )
	{
		printf("hx_8394_write_analog_digital_gamma_reg_data: Invalid params!\n");
		return -1;
	}

	setMipi2828HighSpeedMode(channel, false);
	hi_8394_enable_extend_cmd(channel, 1);
					
	// write new analog gamma reg data.
	chip_hi_8394_write_analog_reg(channel, p_chip_info->analog_gamma_reg_data, HX_8394_ANALOG_GAMMA_LEN);

	// write d3g enable
	memcpy(p_chip_info->digital_gamma_reg_data[0], p_digital_gamma_reg_data, HX_8394_D3G_R_REG_LEN);
	memcpy(p_chip_info->digital_gamma_reg_data[1], p_digital_gamma_reg_data + HX_8394_D3G_R_REG_LEN, 
			HX_8394_D3G_G_REG_LEN);
	memcpy(p_chip_info->digital_gamma_reg_data[2], p_digital_gamma_reg_data + HX_8394_D3G_R_REG_LEN + HX_8394_D3G_G_REG_LEN, 
			HX_8394_D3G_B_REG_LEN);
	
	// write new digital gamma value.
	hx_8394_write_d3g_reg_2(channel, p_chip_info->digital_gamma_reg_data[0], HX_8394_D3G_R_REG_LEN,
 								p_chip_info->digital_gamma_reg_data[1], HX_8394_D3G_G_REG_LEN, 
								p_chip_info->digital_gamma_reg_data[2], HX_8394_D3G_B_REG_LEN);

	setMipi2828HighSpeedMode(channel, true);
	
	return 0;
}

static int hx_8394_write_fix_digital_gamma_reg_data(DEV_CHANNEL_E channel)
{
	printf("hx_8394_write_fix_digital_gamma_reg_data: do nothing!\n");
	return 0;
}

int hx_8394_burn_gamma_data_2nd(DEV_CHANNEL_E channel, unsigned int vcom, int burn_vcom,unsigned char *p_analog_gamma_reg, int analog_gamma_reg_len,
									unsigned char *p_d3g_r_reg, int d3g_r_reg_len,
									unsigned char *p_d3g_g_reg, int d3g_g_reg_len,
									unsigned char *p_d3g_b_reg, int d3g_b_reg_len)
{
	int use_internal_vpp = 1;	

	printf("hx_8394_burn_gamma_data_2nd: channel=%d.\n", channel);

	setMipi2828HighSpeedMode(channel, false);
	mipiIcReset(0);
	mipiIcReset(1);
	mipi2828Write2IC(channel, 0x11, NULL, 0);
	_msleep(150);

	// password
	unsigned char passwd[] =
	{
		0xFF, 0x83, 0x94,
	};

	mipi2828Write2IC(channel, 0xB9, passwd, sizeof(passwd));

	if (burn_vcom)
	{
		// vcom
		unsigned char vcom_value[] =
		{
			0x00, 0x00, 0x00,
		};

		vcom_value[0] = vcom;
		vcom_value[1] = vcom;
		mipi2828Write2IC(channel, 0xB6, passwd, sizeof(passwd));
	}

	// gamma	
	chip_hi_8394_write_analog_reg(channel, p_analog_gamma_reg, analog_gamma_reg_len);

	// d3g
	hx_8394_write_d3g_reg_2(channel, p_d3g_r_reg, d3g_r_reg_len, p_d3g_g_reg, d3g_g_reg_len,
								p_d3g_b_reg, d3g_b_reg_len);
	_msleep(100);
	
	// otp key
	unsigned char otp_key[] =
	{
		0x00, 0x00, 0x00, 0x00, 0x00, 0xAA, 0x55,
	};

	mipi2828Write2IC(channel, 0xBB, otp_key, sizeof(otp_key));

	if (burn_vcom)
	{
		// vcom
		unsigned char vcom_otp_power[] =
		{
			0x80, 0x0D,
		};

		mipi2828Write2IC(channel, 0xBB, vcom_otp_power, sizeof(vcom_otp_power));

		unsigned char vcom_otp_power2[] =
		{
			0x80, 0x0D, 0x00, 0x01,
		};

		mipi2828Write2IC(channel, 0xBB, vcom_otp_power2, sizeof(vcom_otp_power2));
	}
	
	_msleep(550);

	// second analog gamma
	unsigned char gamma_otp_power[] =
	{
		0x81, 0x00, 0x00, 0x00,
	};

	mipi2828Write2IC(channel, 0xBB, gamma_otp_power, sizeof(gamma_otp_power));

	unsigned char gamma_otp_power2[] =
	{
		0x81, 0x00, 0x00, 0x01,
	};

	mipi2828Write2IC(channel, 0xBB, gamma_otp_power2, sizeof(gamma_otp_power2));
	
	_msleep(700);

	// d3g
	unsigned char dgc_otp_power[] =
	{
		0x81, 0x3A, 0x00, 0x00,
	};		

	mipi2828Write2IC(channel, 0xBB, dgc_otp_power, sizeof(dgc_otp_power));

	unsigned char dgc_otp_power2[] =
	{
		0x81, 0x3A, 0x00, 0x01,
	};

	mipi2828Write2IC(channel, 0xBB, dgc_otp_power2, sizeof(dgc_otp_power2));
	
	_msleep(1400);

	// gamma replacement otp
	unsigned char gamma_replace_otp_cmd[] =
	{
		0x80, 0xFF, 0x00, 0x00,
	};		

	mipi2828Write2IC(channel, 0xBB, gamma_replace_otp_cmd, sizeof(gamma_replace_otp_cmd));

	unsigned char gamma_replace_otp_cmd2[] =
	{
		0x80, 0xFF, 0x00, 0x01,
	};

	mipi2828Write2IC(channel, 0xBB, gamma_replace_otp_cmd2, sizeof(gamma_replace_otp_cmd2));

	unsigned char otp_end[100] = 
	{
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	};

	mipi2828Write2IC(channel, 0xBB, otp_end, sizeof(otp_end));
	
	mipi2828Write2IC(channel, 0xB9, passwd, sizeof(passwd));
	mipi2828Write2IC(channel, 0x10, NULL, 0);
	
	setMipi2828HighSpeedMode(channel, true);
	return 0;
}


int hx_8394_burn_vcom_and_gamma_data(DEV_CHANNEL_E channel, unsigned int vcom,  int burn_vcom,
 										unsigned char *p_analog_gamma_reg, int analog_gamma_reg_len,
										unsigned char *p_d3g_r_reg, int d3g_r_reg_len,
										unsigned char *p_d3g_g_reg, int d3g_g_reg_len,
										unsigned char *p_d3g_b_reg, int d3g_b_reg_len)
{
	int use_internal_vpp = 1;	

	printf("hx_8394_burn_vcom_and_gamma_data: channel=%d, vcom=%d.\n", channel, vcom);

	setMipi2828HighSpeedMode(channel, false);
	mipiIcReset(0);
	mipiIcReset(1);

	mipi2828Write2IC(channel, 0x11, NULL, 0);

	_msleep(150);

	// password
	unsigned char passwd[] =
	{
		0xFF, 0x83, 0x94,
	};

	mipi2828Write2IC(channel, 0xB9, passwd, sizeof(passwd));

	if (burn_vcom)
	{
		// vcom
		unsigned char vcom_value[] =
		{
			0x00, 0x00, 0x00,
		};

		vcom_value[0] = vcom;
		vcom_value[1] = vcom;
		mipi2828Write2IC(channel, 0xB6, vcom_value, sizeof(vcom_value));
	}

	// gamma	
	chip_hi_8394_write_analog_reg(channel, p_analog_gamma_reg, analog_gamma_reg_len);

	// d3g
	hx_8394_write_d3g_reg_2(channel, p_d3g_r_reg, d3g_r_reg_len, p_d3g_g_reg, d3g_g_reg_len,
								p_d3g_b_reg, d3g_b_reg_len);

	_msleep(100);
	
	// otp key
	unsigned char otp_key[] =
	{
		0x00, 0x00, 0x00, 0x00, 0x00, 0xAA, 0x55,
	};

	mipi2828Write2IC(channel, 0xBB, otp_key, sizeof(otp_key));

	if (burn_vcom)
	{
		// vcom
		unsigned char vcom_otp_power[] =
		{
			0x80, 0x0D,
		};

		mipi2828Write2IC(channel, 0xBB, vcom_otp_power, sizeof(vcom_otp_power));

		unsigned char vcom_otp_power2[] =
		{
			0x80, 0x0D, 0x00, 0x01,
		};

		mipi2828Write2IC(channel, 0xBB, vcom_otp_power2, sizeof(vcom_otp_power2));
	}
	
	_msleep(550);

	// analog gamma
	unsigned char gamma_otp_power[] =
	{
		0x80, 0x45,
	};

	mipi2828Write2IC(channel, 0xBB, gamma_otp_power, sizeof(gamma_otp_power));

	unsigned char gamma_otp_power2[] =
	{
		0x80, 0x45, 0x00, 0x01,
	};

	mipi2828Write2IC(channel, 0xBB, gamma_otp_power2, sizeof(gamma_otp_power2));
	
	_msleep(700);

	// d3g
	unsigned char dgc_otp_power[] =
	{
		0x80, 0x7F,
	};

	mipi2828Write2IC(channel, 0xBB, dgc_otp_power, sizeof(dgc_otp_power));

	unsigned char dgc_otp_power2[] =
	{
		0x80, 0x7F, 0x00, 0x01,
	};

	mipi2828Write2IC(channel, 0xBB, dgc_otp_power2, sizeof(dgc_otp_power2));
	
	_msleep(1400);

	unsigned char otp_end[] =
	{
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	};

	mipi2828Write2IC(channel, 0xBB, otp_end, sizeof(otp_end));
	mipi2828Write2IC(channel, 0xB9, passwd, sizeof(passwd));

	mipi2828Write2IC(channel, 0x10, NULL, 0);
	
	setMipi2828HighSpeedMode(channel, true);
	return 0;
}


static int hx_8394_burn_gamma_otp_values(DEV_CHANNEL_E channel, int burn_flag,int enable_burn_vcom, int vcom,
											unsigned char *p_analog_gamma_reg_data, int analog_gamma_reg_data_len,
											unsigned char *p_d3g_r_reg_data, int d3g_r_reg_data_len,
											unsigned char *p_d3g_g_reg_data, int deg_g_reg_data_len,
											unsigned char *p_d3g_b_reg_data, int deg_b_reg_data_len)
{

	if (burn_flag & ICM_BURN_SECOND_FLAG)
	{
		printf("== burn gamma 2nd: vcom_burn: %d.\n", enable_burn_vcom);
		hx_8394_burn_gamma_data_2nd(channel, vcom, enable_burn_vcom,
											p_analog_gamma_reg_data, HX_8394_ANALOG_GAMMA_LEN,
											p_d3g_r_reg_data, HX_8394_D3G_R_REG_LEN,
											p_d3g_g_reg_data, HX_8394_D3G_G_REG_LEN,
											p_d3g_b_reg_data, HX_8394_D3G_B_REG_LEN);
	}
	else
	{
		hx_8394_burn_vcom_and_gamma_data(channel, vcom, enable_burn_vcom,
											p_analog_gamma_reg_data, HX_8394_ANALOG_GAMMA_LEN,
											p_d3g_r_reg_data, HX_8394_D3G_R_REG_LEN,
											p_d3g_g_reg_data, HX_8394_D3G_G_REG_LEN,
											p_d3g_b_reg_data, HX_8394_D3G_B_REG_LEN);
	}
	
	return 0;
}

static int hx_8394_check_gamma_otp_values(DEV_CHANNEL_E channel, int enable_burn_vcom, int new_vcom_value, int last_vcom_otp_times,
											unsigned char *p_analog_gamma_reg_data, int analog_gamma_reg_data_len,
											unsigned char *p_d3g_r_reg_data, int d3g_r_reg_data_len,
											unsigned char *p_d3g_g_reg_data, int deg_g_reg_data_len,
											unsigned char *p_d3g_b_reg_data, int deg_b_reg_data_len)
{
	int check_error = 0;
	unsigned int read_vcom = 0;
	unsigned char otp_times = 0;
	
	setMipi2828HighSpeedMode(channel, false);

	// enable vcom burn
	if (enable_burn_vcom)
	{
		hi_8394_read_vcom(channel, 0, &read_vcom);
		hi_8394_get_vcom_otp_times(channel, &otp_times, &otp_times);

		printf("read vcom: %d. 0x02%x, otp times: %d.\n", read_vcom, read_vcom, otp_times);

		// check times
		if (last_vcom_otp_times + 1 != otp_times)
		{
			printf("Otp error: last times: %d, now times: %d.\n", last_vcom_otp_times, otp_times);
			check_error = 1;
		}

		// check vcom value
		if (new_vcom_value != read_vcom)
		{
			printf("Otp error: write vcom: %d, read vcom: %d.\n", new_vcom_value, read_vcom);
			check_error = 2;
		}
	}
	
	// check analog gamma reg value
	unsigned char analog_gamma_data[HX_8394_ANALOG_GAMMA_LEN] = { 0 };
	ReadModReg(channel, 1, HX_8394_ANALOG_GAMMA_REG, HX_8394_ANALOG_GAMMA_LEN, analog_gamma_data);
	printf("read analog gamma data: \n");
	dump_data1(analog_gamma_data, HX_8394_ANALOG_GAMMA_LEN);
	
	if (memcmp(analog_gamma_data, p_analog_gamma_reg_data, HX_8394_ANALOG_GAMMA_LEN) != 0)
	{
		printf("Otp error: check analog gamma reg data error!\n");
		check_error = 3;
	}

	// check dgc r reg value
	// check dgc g reg value
	// check dgc b reg value
	unsigned char d3g_r_data[HX_8394_D3G_R_REG_LEN] = { 0 };
	unsigned char d3g_g_data[HX_8394_D3G_G_REG_LEN] = { 0 };
	unsigned char d3g_b_data[HX_8394_D3G_B_REG_LEN] = { 0 };

	unsigned char dgc_bank_index = 0x00;
	unsigned char dgc_bank_reg = 0xBD;
	unsigned char dgc_data_reg = 0xC1;
	
	// set 00 => BD
	hi_8394_select_d3g_bank_r(channel);
	ReadModReg(channel, 1, dgc_data_reg, HX_8394_D3G_R_REG_LEN, d3g_r_data);
	printf("read d3g r data: \n");
	dump_data1(d3g_r_data, HX_8394_D3G_R_REG_LEN);

	// set 01 => BD
	hi_8394_select_d3g_bank_g(channel);
	ReadModReg(channel, 1, dgc_data_reg, HX_8394_D3G_G_REG_LEN, d3g_g_data);
	printf("read d3g g data: \n");
	dump_data1(d3g_g_data, HX_8394_D3G_G_REG_LEN);	

	// set 02 => BD
	hi_8394_select_d3g_bank_b(channel);
	ReadModReg(channel, 1, dgc_data_reg, HX_8394_D3G_B_REG_LEN, d3g_b_data);
	printf("read d3g b data: \n");
	dump_data1(d3g_b_data, HX_8394_D3G_B_REG_LEN);

	//d3g_r_data[0] = 0x01;
	if (memcmp(d3g_r_data, p_d3g_r_reg_data, HX_8394_D3G_R_REG_LEN) != 0)
	{
		printf("Otp error: check dgc r reg data error!\n");
		check_error = 4;
	}

	if (memcmp(d3g_g_data, p_d3g_g_reg_data, HX_8394_D3G_G_REG_LEN) != 0)
	{
		printf("Otp error: check dgc g reg data error!\n");
		check_error = 5;
	}

	if (memcmp(d3g_b_data, p_d3g_b_reg_data, HX_8394_D3G_B_REG_LEN) != 0)
	{
		printf("Otp error: check dgc b reg data error!\n");
		check_error = 6;
	}
			
	setMipi2828HighSpeedMode(channel, true);
	
	return check_error;
}

static int hx_8394_get_analog_gamma_reg_data(DEV_CHANNEL_E channel,unsigned char *p_analog_gamma_reg_data, int *p_analog_gamma_reg_data_len)
{
	hx_8394_info_t* p_chip_info = &s_hx_8394_private_info[channel];
	
	memcpy(p_analog_gamma_reg_data, p_chip_info->analog_gamma_reg_data, HX_8394_ANALOG_GAMMA_LEN);
	*p_analog_gamma_reg_data_len = HX_8394_ANALOG_GAMMA_LEN;
	
	printf("hx_8394_get_analog_gamma_reg_data: \n");
	dump_data1(p_analog_gamma_reg_data, HX_8394_ANALOG_GAMMA_LEN);
	return -1;
}

static int hx_8394_get_digital_gamma_reg_data(DEV_CHANNEL_E channel,unsigned char *p_d3g_r_reg_data, int *p_d3g_r_reg_data_len,
											unsigned char *p_d3g_g_reg_data, int *p_d3g_g_reg_data_len,
											unsigned char *p_d3g_b_reg_data, int *p_d3g_b_reg_data_len)
{
	hx_8394_info_t* p_chip_info = &s_hx_8394_private_info[channel];
	
	memcpy(p_d3g_r_reg_data, p_chip_info->digital_gamma_reg_data[0], HX_8394_D3G_R_REG_LEN);
	*p_d3g_r_reg_data_len = HX_8394_D3G_R_REG_LEN;
	memcpy(p_d3g_g_reg_data, p_chip_info->digital_gamma_reg_data[1], HX_8394_D3G_G_REG_LEN);
	*p_d3g_g_reg_data_len = HX_8394_D3G_G_REG_LEN;
	memcpy(p_d3g_b_reg_data, p_chip_info->digital_gamma_reg_data[2], HX_8394_D3G_B_REG_LEN);
	*p_d3g_b_reg_data_len = HX_8394_D3G_B_REG_LEN;
	return 0;
}


static icm_chip_t chip_hi_8394 =
{
	.name = "hi8394",

	.reset_private_data = chip_hi_8394_get_and_reset_private_data,
	
	.id = chip_hi_8394_get_chip_id,
	.check_ok = chip_hi_8394_check_chip_ok,
	.read_vcom_otp_times = chip_hi_8394_read_chip_vcom_opt_times,
	.read_vcom_otp_info = chip_hi_8394_read_chip_vcom_opt_info,
	
	.read_vcom = chip_hi_8394_read_vcom,
	.write_vcom = chip_hi_8394_write_vcom,
	
	.write_vcom_otp_value = chip_hi_8394_write_chip_vcom_otp_value,
	.check_vcom_otp_burn_ok = chip_hi_8394_check_vcom_otp_burn_ok,

	// gamma
	.write_analog_gamma_reg_data = hx_8394_write_analog_gamma_reg_data,
	.read_analog_gamma_reg_data = hx_8394_read_analog_gamma_reg_data,
	.write_fix_analog_gamma_reg_data = hx_8394_write_fix_analog_gamma_reg_data,
	
	.d3g_control = hx_8394_write_d3g_control,
	.write_digital_gamma_reg_data = hx_8394_write_analog_digital_gamma_reg_data,
	.write_fix_digital_gamma_reg_data = hx_8394_write_fix_digital_gamma_reg_data,

	.burn_gamma_otp_values = hx_8394_burn_gamma_otp_values,
	.check_gamma_otp_values = hx_8394_check_gamma_otp_values,
	
	.get_analog_gamma_reg_data = hx_8394_get_analog_gamma_reg_data,
	.get_digital_gamma_reg_data = hx_8394_get_digital_gamma_reg_data,
};

void hi8394Init()
{
	registerIcChip(&chip_hi_8394);
}


