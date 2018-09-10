#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "mipi2828.h"

#include "common.h"
#include "../ic_manager.h"

#define ICN_9706_ANALOG_GAMMA_LEN		(38)

#define ICN_9706_D3G_COMPONENT_NUMS		(3)
#define ICN_9706_D3G_REG_LEN			(26)

#define ICN_9706_ANALOG_GAMMA_REG		(0xC8)


#define ICN_9706_ENABLE_OTP		(1)

typedef struct tag_icn_9706_info
{
	unsigned char id[3];

	unsigned char id_ok;	// 0: id_error; 1: id_ok;
	
	unsigned int vcom1;
	unsigned int  vcom1_otp_times;
	
	unsigned int vcom2;
	unsigned int  vcom2_otp_times;

	unsigned char analog_gamma_reg_data[ICN_9706_ANALOG_GAMMA_LEN];
	unsigned char digital_gamma_reg_data[ICN_9706_D3G_COMPONENT_NUMS][ICN_9706_D3G_REG_LEN];
}icn_9706_info_t;

static icn_9706_info_t s_icn_9706_private_info[DEV_CHANNEL_NUM] = { 0 };

static int icn_9706_enable_level_2_cmd(DEV_CHANNEL_E channel)
{
	unsigned char param0[] = {0x5A, 0x5A,};
	mipi2828Write2IC(channel, 0xF0, param0, sizeof(param0));

	unsigned char param1[] = {0xA5, 0xA5,};
	mipi2828Write2IC(channel, 0xF1, param1, sizeof(param1));

	unsigned char param2[] = {0xB4, 0x4B,};
	mipi2828Write2IC(channel, 0xF0, param1, sizeof(param1));
	return 0;
}

static int icn_9706_get_vcom_otp_times(DEV_CHANNEL_E channel, unsigned char *p_vcom1_otp_times,
										unsigned char *p_vcom2_otp_times)
{
	icn_9706_enable_level_2_cmd(channel);
	unsigned char reg[3] = { 0 };
	ReadModReg(channel, 1, 0xB6, 3, reg);
	printf("icn_9706_get_vcom_otp_times: %02x, %02x, %02x.\n", reg[0], reg[1], reg[2]);

	*p_vcom1_otp_times = reg[2];
	*p_vcom2_otp_times = reg[2];
	
	return 0;
}

static int icn_9706_read_vcom(DEV_CHANNEL_E channel, int index, unsigned int *p_vcom)
{
	icn_9706_enable_level_2_cmd(channel);
	unsigned char reg[3] = { 0 };
	ReadModReg(channel, 1, 0xB6, 3, reg);

	if (index == 0)
		*p_vcom = reg[0];
	else if (index == 1)
		*p_vcom = reg[1];
	else
	{
		printf("icn_9706_read_vcom: invalid index = %d.\n", index);
	}
	
	return 0;
}

static int icn_9706_write_vcom(DEV_CHANNEL_E channel, unsigned int vcom_value)
{	
	icn_9706_enable_level_2_cmd(channel);
	unsigned char params[] = {0, 0,};
	params[0] = vcom_value;
	params[1] = vcom_value;

	mipi2828Write2IC(channel, 0xB6, params, sizeof(params));
	return 0;
}


static int icn_9706_read_nvm_vcom(DEV_CHANNEL_E channel, int index, unsigned int *p_vcom_otp_value)
{
	icn_9706_enable_level_2_cmd(channel);

	unsigned char reg[4] = { 0 };
	ReadModReg(channel, 1, 0xB6, 4, reg);

	printf("icn_9706_read_nvm_vcom: %02x, %02x, %02x, %02x.\n", reg[0], reg[1], reg[2], reg[3]);
	
	if (index == 0)
		*p_vcom_otp_value = reg[0];
	else if (index == 1)
		*p_vcom_otp_value = reg[1];
	else
	{
		printf("icn_9706_read_nvm_vcom: invalid index = %d.\n", index);
	}
	
	return 0;
}

static void setPasswd(DEV_CHANNEL_E channel)
{
	unsigned char passwd0[] = {0x5A, 0x5A,};
	mipi2828Write2IC(channel, 0xF0, passwd0, sizeof(passwd0));

	unsigned char passwd1[] = {0xA5, 0xA5,};
	mipi2828Write2IC(channel, 0xF1, passwd1, sizeof(passwd1));

	unsigned char passwd2[] = {0xB4, 0x4B,};
	mipi2828Write2IC(channel, 0xF0, passwd2, sizeof(passwd2));

	unsigned char passwd3[] = {0x02, 0x01, 0x01,};
	mipi2828Write2IC(channel, 0xCB, passwd3, sizeof(passwd3));
}
// OTP Bank:
//	0: ID;
//	1: VCOM;
//	2: Digigal Gamma;
//	3: Analog Gamma;
static int icn_9706_burn_vcom(DEV_CHANNEL_E channel, int otp_vcom_value)
{
	
	setMipi2828HighSpeedMode(channel, false);

	icn_9706_write_vcom(channel, otp_vcom_value);

	_msleep(100);
	setPasswd(channel);
	_msleep(100);
	
	setMipi2828HighSpeedMode(channel, true);

	unsigned int read_vcom = 0;
	icn_9706_read_vcom(channel, 0, &read_vcom);

	printf("icn_9706_read_vcom: %d.\n", read_vcom);
	return 0;	
}

static int icn_9706_burn_gamma_data(DEV_CHANNEL_E channel, unsigned char *p_analog_gamma_data, int analog_gamma_data_len,
										unsigned char *p_dgc_data_r, int dgc_data_r_len, 
										unsigned char *p_dgc_data_g, int dgc_data_g_len,
										unsigned char *p_dgc_data_b, int dgc_data_b_len )
{
	setMipi2828HighSpeedMode(channel, false);

	// write analog gamma
	mipi2828Write2IC(channel, 0xC8, p_analog_gamma_data, analog_gamma_data_len);

	// write dgc_control
	unsigned char regValue = 0x03;
	mipi2828Write2IC(channel, 0xE3, &regValue, sizeof(regValue));

	// write digital gamma 
	// r 
	mipi2828Write2IC(channel, 0xE4, p_dgc_data_r, dgc_data_r_len);
	
	// g
	mipi2828Write2IC(channel, 0xE5, p_dgc_data_g, dgc_data_g_len);
	
	// b
	mipi2828Write2IC(channel, 0xE6, p_dgc_data_b, dgc_data_b_len);

	_msleep(500);
	setPasswd(channel);
	_msleep(500);
	setMipi2828HighSpeedMode(channel, true);
	return 0;
}

static int icn_9706_burn_vcom_and_gamma_data(DEV_CHANNEL_E channel, int otp_vcom_value, int burn_vcom,
										unsigned char *p_analog_gamma_data, int analog_gamma_data_len,
										unsigned char *p_dgc_data_r, int dgc_data_r_len, 
										unsigned char *p_dgc_data_g, int dgc_data_g_len,
										unsigned char *p_dgc_data_b, int dgc_data_b_len )
{
	setMipi2828HighSpeedMode(channel, false);

	if (burn_vcom)
	{
		icn_9706_write_vcom(channel, otp_vcom_value);
	}

	// write analog gamma
	mipi2828Write2IC(channel, 0xC8, p_analog_gamma_data, analog_gamma_data_len);

	// write dgc_control
	unsigned char dgc_ctrl = 0x03;
	mipi2828Write2IC(channel, 0xE3, &dgc_ctrl, sizeof(dgc_ctrl));

	// write digital gamma 
	// r 
	mipi2828Write2IC(channel, 0xE4, p_dgc_data_r, dgc_data_r_len);
	
	// g
	mipi2828Write2IC(channel, 0xE5, p_dgc_data_g, dgc_data_g_len);
	
	// b
	mipi2828Write2IC(channel, 0xE6, p_dgc_data_b, dgc_data_b_len);

	_msleep(150);
	setPasswd(channel);
	_msleep(500);

	if (burn_vcom)
	{
		unsigned int read_vcom = 0;
		icn_9706_read_vcom(channel, 0, &read_vcom);
		printf("icn_9706_read_vcom: %d.\n", read_vcom);
	}
	
	setMipi2828HighSpeedMode(channel,true);
	return 0;
}

static int chip_icn_9706_get_chip_id(DEV_CHANNEL_E channel, unsigned char *p_id_data, int id_data_len)
{
	char reg[3] = { 0 };	

	// read id4
    ReadModReg(channel, 1, 0x0A, 3, reg);
	printf("chip_icn_9706_get_chip_id: channel: %d, read id: %02x:%02x:%02x. \n",
				channel, reg[2], reg[1], reg[0]);

	if (reg[0] == 0x9C)
	{
		p_id_data[0] = reg[0];
		p_id_data[1] = reg[0];
		p_id_data[2] = reg[0];
		return 0;
	}
	
	return -1;
}


static int chip_icn_9706_check_chip_ok(DEV_CHANNEL_E channel)
{
	// read and check id;
	chip_icn_9706_get_chip_id(channel, s_icn_9706_private_info[channel].id,
								sizeof(s_icn_9706_private_info[channel].id));

	unsigned char* p_id = s_icn_9706_private_info[channel].id;
	//if ( (p_id[0] != 0x00) || (p_id[1] != 0x80) || (p_id[2] != 0x00) )
	//if ( (p_id[0] != 0x00) || (p_id[1] != 0x00) || (p_id[2] != 0x00) )
	if ( (p_id[0] != 0x9C) || (p_id[1] != 0x9C) || (p_id[2] != 0x9C) )
	{
		printf("chip_icn_9706_check_chip_ok error: Invalid chip id: %02x, %02x, %02x.\n",
					p_id[0], p_id[1], p_id[2]);
		
		s_icn_9706_private_info[channel].id_ok = 0;

		return -1;
	}

	s_icn_9706_private_info[channel].id_ok = 1;

	// read vcom otp times
	unsigned char vcom1_otp_times = 0;
	unsigned char vcom2_otp_times = 0;
	icn_9706_get_vcom_otp_times(channel, &vcom1_otp_times, &vcom2_otp_times);
	s_icn_9706_private_info[channel].vcom1_otp_times = vcom1_otp_times;
	s_icn_9706_private_info[channel].vcom2_otp_times = vcom2_otp_times;

	// read vcom otp value
	unsigned int vcom1_otp_value = 0;
	unsigned int vcom2_otp_value = 0;
	icn_9706_read_nvm_vcom(channel, 0, &vcom1_otp_value);
	icn_9706_read_nvm_vcom(channel, 1, &vcom2_otp_value);
	s_icn_9706_private_info[channel].vcom1 = vcom1_otp_value;
	s_icn_9706_private_info[channel].vcom2 = vcom2_otp_value;

	printf("chip_icn_9706_check_chip_ok:chip:%d, mipi: %d. vcom1_otp_times: %d, vcom2_otp_times: %d."
			"vcom1_otp_value: %d. vcom2_otp_value: %d.\n", channel, channel,
				s_icn_9706_private_info[channel].vcom1_otp_times,
				s_icn_9706_private_info[channel].vcom2_otp_times,
				s_icn_9706_private_info[channel].vcom1,
				s_icn_9706_private_info[channel].vcom2);
	return 0;
}

static int chip_icn_9706_read_chip_vcom_opt_times(DEV_CHANNEL_E channel, int* p_otp_vcom_times)
{
	if (s_icn_9706_private_info[channel].id_ok == 1)
	{
		// read vcom otp times
		unsigned char vcom1_otp_times = 0;
		unsigned char vcom2_otp_times = 0;
		icn_9706_get_vcom_otp_times(channel, &vcom1_otp_times, &vcom2_otp_times);

		*p_otp_vcom_times = vcom1_otp_times;

		printf("chip_icn_9706_read_chip_vcom_opt_times: end\n");

		return 0;
	}
	
	printf("chip_icn_9706_read_chip_vcom_opt_times error!\n");
	return -1;
}


static int chip_icn_9706_read_chip_vcom_opt_info(DEV_CHANNEL_E channel, int* p_otp_vcom_times,
												int *p_otp_vcom_value)
{
	if (s_icn_9706_private_info[channel].id_ok == 1)
	{
		// read vcom otp times
		unsigned char vcom1_otp_times = 0;
		unsigned char vcom2_otp_times = 0;
		icn_9706_get_vcom_otp_times(channel, &vcom1_otp_times, &vcom2_otp_times);

		*p_otp_vcom_times = vcom1_otp_times;

		unsigned int vcom = 0;
		icn_9706_read_vcom(channel, 0, &vcom);

		*p_otp_vcom_value = vcom;

		printf("chip_icn_9706_read_chip_vcom_opt_info: vcom = %d end\n", *p_otp_vcom_value);

		return 0;
	}
	
	printf("chip_icn_9706_read_chip_vcom_opt_info error!\n");
	return -1;
}


static int chip_icn_9706_write_chip_vcom_otp_value(DEV_CHANNEL_E channel, int otp_vcom_value)
{
	if (s_icn_9706_private_info[channel].id_ok == 1)
	{
		int val = 0;
		val = icn_9706_burn_vcom(channel, otp_vcom_value);
		return val;
	}

	printf("chip_icn_9706_write_chip_vcom_otp_value error!\n");
	return -1;
}


static int chip_icn_9706_read_vcom(DEV_CHANNEL_E channel, int* p_vcom_value)
{
	if (s_icn_9706_private_info[channel].id_ok == 1)
	{
		int vcom = 0;
		icn_9706_read_vcom(channel, 0, &vcom);
		*p_vcom_value = vcom;
		//printf("chip_icn_9706_read_vcom: end\n");

		return 0;
	}
	printf("chip_icn_9706_read_vcom error: unknow chip!\n");
	return -1;
}


static int chip_icn_9706_write_vcom(DEV_CHANNEL_E channel, int vcom_value)
{
	if (s_icn_9706_private_info[channel].id_ok == 1)
	{
		if (s_icn_9706_private_info[channel].vcom1_otp_times == 0)
			icn_9706_write_vcom(channel, vcom_value);
		else
			icn_9706_write_vcom(channel, vcom_value);

		return 0;
	}
	
	printf("chip_ili_9806e_write_vcom error: unknow chip!\n");
	
	return -1;
}

static void chip_icn_9706_get_and_reset_private_data(DEV_CHANNEL_E channel)
{
	printf("chip_icn_9706_get_and_reset_private_data.\n");
	s_icn_9706_private_info[channel].id_ok = 0;
	memset(s_icn_9706_private_info[channel].id, 0,
			sizeof(s_icn_9706_private_info[channel].id));
	s_icn_9706_private_info[channel].vcom1 = 0;
	s_icn_9706_private_info[channel].vcom1_otp_times = 0;
	s_icn_9706_private_info[channel].vcom2 = 0;
	s_icn_9706_private_info[channel].vcom2_otp_times = 0;
	memset(s_icn_9706_private_info[channel].analog_gamma_reg_data, 1,
			sizeof(s_icn_9706_private_info[channel].analog_gamma_reg_data));
	memset(s_icn_9706_private_info[channel].digital_gamma_reg_data, 2,
			sizeof(s_icn_9706_private_info[channel].digital_gamma_reg_data));
}


static int chip_icn_9706_check_vcom_otp_burn_ok(DEV_CHANNEL_E channel, int vcom_value,
													int last_otp_times, int *p_read_vcom)
{
	printf("chip_icn_9706_check_vcom_otp_burn_ok: last times: %d. last vcom: %d. \n", last_otp_times, vcom_value);
	// check id 
	int val = chip_icn_9706_check_chip_ok(channel);
	if (val != 0)
	{
		printf("chip_icn_9706_check_vcom_otp_burn_ok error: check id error, val = %d.\n", val);
		return E_ICM_READ_ID_ERROR;
	}

	if (p_read_vcom)
		*p_read_vcom = s_icn_9706_private_info[channel].vcom1;
	
	#if 1
	// check times
	if (s_icn_9706_private_info[channel].vcom1_otp_times != last_otp_times + 1)
	{
		printf("chip_icn_9706_check_vcom_otp_burn_ok error: check vcom otp times error. last times: %d, now: %d.\n", 
				last_otp_times, s_icn_9706_private_info[channel].vcom1_otp_times);

		return E_ICM_VCOM_TIMES_NOT_CHANGE;
	}
	#endif
	
	// check vcom value.
	if (s_icn_9706_private_info[channel].vcom1 != vcom_value)
	{
		printf("chip_icn_9706_check_vcom_otp_burn_ok error: check vcom value error. write: %d, read: %d.\n", 
				vcom_value, s_icn_9706_private_info[channel].vcom1);

		return E_ICM_VCOM_VALUE_NOT_CHANGE;
	}
	
	return E_ICM_OK;
}


// gamma
static unsigned char s_icn_9706_default_analog_gamma_code[ICN_9706_ANALOG_GAMMA_LEN] = 
					{ 
						0x7c, 0x57, 0x41, 0x31, 0x2a, 0x1a, 0x1f, 0x0b, 
						0x28, 0x2a, 0x2c, 0x4d, 0x3d, 0x47, 0x3b, 0x3a, 
						0x2e, 0x1b, 0x06, 
						0x7c, 0x57, 0x41, 0x31, 0x2a, 0x1a, 0x1f, 0x0b, 
						0x28, 0x2a, 0x2c, 0x4d, 0x3d, 0x47, 0x3b, 0x3a, 
						0x2e, 0x1b, 0x06
					}; 

static int icn_9706_write_analog_reg(DEV_CHANNEL_E channel, unsigned char *p_analog_gamma_reg_data,
								int analog_gamma_data_len)
{
	mipi2828Write2IC(channel, 0xC8, p_analog_gamma_reg_data, analog_gamma_data_len);
	return 0;
}

static int icn_9706_digital_gamma_control(DEV_CHANNEL_E channel, unsigned char value)
{
	mipi2828Write2IC(channel, 0xE3, &value, 1);
	return 0;
}

static int icn_9706_write_d3g_reg(DEV_CHANNEL_E channel, unsigned char reg, unsigned char *p_d3g_reg_data, int d3g_reg_data_len)
{
	mipi2828Write2IC(channel, reg, p_d3g_reg_data, d3g_reg_data_len);
	return 0;
}

static int chip_icn_9706_write_analog_gamma_reg_data(DEV_CHANNEL_E channel,unsigned char *p_analog_gamma_reg_data, int analog_gamma_reg_len)
{
	icn_9706_info_t* p_chip_info = &s_icn_9706_private_info[channel];

	memcpy(p_chip_info->analog_gamma_reg_data, p_analog_gamma_reg_data, ICN_9706_ANALOG_GAMMA_LEN);

	setMipi2828HighSpeedMode(channel, false);
	
	icn_9706_enable_level_2_cmd(channel);
	
	icn_9706_write_analog_reg(channel, p_chip_info->analog_gamma_reg_data, ICN_9706_ANALOG_GAMMA_LEN);

	setMipi2828HighSpeedMode(channel, true);
	return 0;
}

static int chip_icn_9706_read_analog_gamma_reg_data(DEV_CHANNEL_E channel,unsigned char *p_analog_gamma_reg_data_buffer, int analog_gamma_reg_len)
{
	setMipi2828HighSpeedMode(channel, false);
	
	icn_9706_enable_level_2_cmd(channel);

	ReadModReg(channel, 1, ICN_9706_ANALOG_GAMMA_REG, ICN_9706_ANALOG_GAMMA_LEN, p_analog_gamma_reg_data_buffer);
	setMipi2828HighSpeedMode(channel, true);
	
	printf("chip_icn_9706_read_analog_gamma_reg_data: read alanog reg: 0x%02x, len: 0x%02x.\n", p_analog_gamma_reg_data_buffer, ICN_9706_ANALOG_GAMMA_LEN);
	dump_data1(p_analog_gamma_reg_data_buffer, ICN_9706_ANALOG_GAMMA_LEN);
	
	return ICN_9706_ANALOG_GAMMA_LEN;
}

static int chip_icn_9706_write_fix_analog_gamma_reg_data(DEV_CHANNEL_E channel)
{
	printf("chip_icn_9706_write_fix_analog_gamma_reg_data:");
	setMipi2828HighSpeedMode(channel, false);
	icn_9706_enable_level_2_cmd(channel);
	icn_9706_write_analog_reg(channel, s_icn_9706_default_analog_gamma_code, ICN_9706_ANALOG_GAMMA_LEN);
	setMipi2828HighSpeedMode(channel, true);
	return 0;
}

static int chip_icn_9706_write_digital_gamma_reg_data(DEV_CHANNEL_E channel, unsigned char *p_digital_gamma_reg_data, int digital_gamma_reg_len)
{
	icn_9706_info_t* p_chip_info = &s_icn_9706_private_info[channel];
	int i = 0;
	
	for (i = 0; i < ICN_9706_D3G_REG_LEN; i ++)
	{
		// r
		p_chip_info->digital_gamma_reg_data[0][i] = p_digital_gamma_reg_data[i];

		// g
		p_chip_info->digital_gamma_reg_data[1][i] = p_digital_gamma_reg_data[i + ICN_9706_D3G_REG_LEN];

		// b
		p_chip_info->digital_gamma_reg_data[2][i] = p_digital_gamma_reg_data[i + ICN_9706_D3G_REG_LEN * 2];

		// fix d3g error.
		if (i == 13)
		{
			if (p_chip_info->digital_gamma_reg_data[0][i] 
					== p_chip_info->digital_gamma_reg_data[0][i-1])
				p_chip_info->digital_gamma_reg_data[0][i] = p_chip_info->digital_gamma_reg_data[0][i-1] - 1;

			if (p_chip_info->digital_gamma_reg_data[1][i] 
					== p_chip_info->digital_gamma_reg_data[1][i-1])
				p_chip_info->digital_gamma_reg_data[1][i] = p_chip_info->digital_gamma_reg_data[1][i-1] - 1;

			if (p_chip_info->digital_gamma_reg_data[2][i] 
					== p_chip_info->digital_gamma_reg_data[2][i-1])
				p_chip_info->digital_gamma_reg_data[2][i] = p_chip_info->digital_gamma_reg_data[2][i-1] - 1;
		}
	}

	setMipi2828HighSpeedMode(channel, false);
	
	icn_9706_enable_level_2_cmd(channel);
					
	// write new analog gamma reg data.
	icn_9706_write_analog_reg(channel, p_chip_info->analog_gamma_reg_data,
								ICN_9706_ANALOG_GAMMA_LEN);

	// write d3g enable
	icn_9706_digital_gamma_control(channel, 0x03);
	icn_9706_write_d3g_reg(channel, 0xE4, p_chip_info->digital_gamma_reg_data[0], ICN_9706_D3G_REG_LEN);
	icn_9706_write_d3g_reg(channel, 0xE5, p_chip_info->digital_gamma_reg_data[1], ICN_9706_D3G_REG_LEN);
	icn_9706_write_d3g_reg(channel, 0xE6, p_chip_info->digital_gamma_reg_data[2], ICN_9706_D3G_REG_LEN);

	setMipi2828HighSpeedMode(channel, true);
	return 0;
}

static int chip_icn_9706_write_fix_digital_gamma_reg_data(DEV_CHANNEL_E channel)
{
	icn_9706_info_t* p_chip_info = &s_icn_9706_private_info[channel];
	unsigned char default_d3g_data[ICN_9706_D3G_REG_LEN] = {
																255, 254, 252, 250, 248, 244, 240, 232, 
																224, 208, 192, 160, 128, 127, 95,  63, 
																47,  31,  23,  15,  11,  7,   5,   3, 
																1, 0
															};

	setMipi2828HighSpeedMode(channel, false);
	
	icn_9706_enable_level_2_cmd(channel);

	// write d3g enable
	icn_9706_digital_gamma_control(channel, 0x03);
	icn_9706_write_d3g_reg(channel, 0xE4, default_d3g_data, ICN_9706_D3G_REG_LEN);
	icn_9706_write_d3g_reg(channel, 0xE5, default_d3g_data, ICN_9706_D3G_REG_LEN);
	icn_9706_write_d3g_reg(channel, 0xE6, default_d3g_data, ICN_9706_D3G_REG_LEN);

	setMipi2828HighSpeedMode(channel, true);
	
	return 0;
}

static int chip_icn_9706_write_d3g_control(DEV_CHANNEL_E channel,int enable)
{
	setMipi2828HighSpeedMode(channel, false);
	
	if (enable)
		icn_9706_digital_gamma_control(channel, 0x01);
	else
		icn_9706_digital_gamma_control(channel, 0x00);

	setMipi2828HighSpeedMode(channel, true);
	
	return 0;
}

static int chip_icn_9706_burn_gamma_otp_values(DEV_CHANNEL_E channel, int burn_flag,int enable_burn_vcom, int vcom,
											unsigned char *p_analog_gamma_reg_data, int analog_gamma_reg_data_len,
											unsigned char *p_d3g_r_reg_data, int d3g_r_reg_data_len,
											unsigned char *p_d3g_g_reg_data, int deg_g_reg_data_len,
											unsigned char *p_d3g_b_reg_data, int deg_b_reg_data_len)
{
	printf("analog gamma: \n");
	dump_data1(p_analog_gamma_reg_data, ICN_9706_ANALOG_GAMMA_LEN);

	printf("otp write d3g r data: \n");
	dump_data1(p_d3g_r_reg_data, ICN_9706_D3G_REG_LEN);
	printf("otp write d3g g data: \n");
	dump_data1(p_d3g_g_reg_data, ICN_9706_D3G_REG_LEN);
	printf("otp write d3g b data: \n");
	dump_data1(p_d3g_b_reg_data, ICN_9706_D3G_REG_LEN);
			
	setMipi2828HighSpeedMode(channel, false);

	icn_9706_burn_vcom_and_gamma_data(channel, vcom, enable_burn_vcom,
												p_analog_gamma_reg_data, analog_gamma_reg_data_len,
												p_d3g_r_reg_data, ICN_9706_D3G_REG_LEN,
												p_d3g_g_reg_data, ICN_9706_D3G_REG_LEN,
												p_d3g_b_reg_data, ICN_9706_D3G_REG_LEN);
	return 0;
}

static int chip_icn_9706_check_gamma_otp_values(DEV_CHANNEL_E channel, int enable_burn_vcom, int new_vcom_value, int last_vcom_otp_times,
											unsigned char *p_analog_gamma_reg_data, int analog_gamma_reg_data_len,
											unsigned char *p_d3g_r_reg_data, int d3g_r_reg_data_len,
											unsigned char *p_d3g_g_reg_data, int deg_g_reg_data_len,
											unsigned char *p_d3g_b_reg_data, int deg_b_reg_data_len)
{
	int otp_times = 0;
	int read_vcom = 0;
	int check_error = 0;
	
	setMipi2828HighSpeedMode(channel, false);

	if (enable_burn_vcom)
	{
		icn_9706_read_vcom(channel, 0, &read_vcom);
		icn_9706_get_vcom_otp_times(channel, &otp_times, &otp_times);

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
	unsigned char analog_gamma_data[ICN_9706_ANALOG_GAMMA_LEN] = { 0 };
	ReadModReg(channel, 1, 0xC8, ICN_9706_ANALOG_GAMMA_LEN, analog_gamma_data);
	printf("read analog gamma data: \n");
	dump_data1(analog_gamma_data, ICN_9706_ANALOG_GAMMA_LEN);
	if (memcmp(analog_gamma_data, p_analog_gamma_reg_data, ICN_9706_ANALOG_GAMMA_LEN) != 0)
	{
		printf("Otp error: check analog gamma reg data error!\n");
		check_error = 3;
	}

	// check dgc r reg value
	// check dgc g reg value
	// check dgc b reg value
	unsigned char d3g_r_data[ICN_9706_D3G_REG_LEN] = { 0 };
	unsigned char d3g_g_data[ICN_9706_D3G_REG_LEN] = { 0 };
	unsigned char d3g_b_data[ICN_9706_D3G_REG_LEN] = { 0 };			
	ReadModReg(channel, 1, 0xE4, ICN_9706_D3G_REG_LEN, d3g_r_data);
	printf("read d3g r data: \n");
	dump_data1(d3g_r_data, ICN_9706_D3G_REG_LEN);			
	ReadModReg(channel, 1, 0xE5, ICN_9706_D3G_REG_LEN, d3g_g_data);
	printf("read d3g g data: \n");
	dump_data1(d3g_g_data, ICN_9706_D3G_REG_LEN);			
	ReadModReg(channel, 1, 0xE6, ICN_9706_D3G_REG_LEN, d3g_b_data);
	printf("read d3g b data: \n");
	dump_data1(d3g_b_data, ICN_9706_D3G_REG_LEN);

	if (memcmp(d3g_r_data, p_d3g_r_reg_data, ICN_9706_D3G_REG_LEN) != 0)
	{
		printf("Otp error: check dgc r reg data error!\n");
		check_error = 4;
	}

	if (memcmp(d3g_g_data, p_d3g_g_reg_data, ICN_9706_D3G_REG_LEN) != 0)
	{
		printf("Otp error: check dgc g reg data error!\n");
		check_error = 5;
	}

	if (memcmp(d3g_b_data, p_d3g_b_reg_data, ICN_9706_D3G_REG_LEN) != 0)
	{
		printf("Otp error: check dgc b reg data error!\n");
		check_error = 6;
	}

	setMipi2828HighSpeedMode(channel, true);
	
	return check_error;
}

static int chip_icn_9706_get_analog_gamma_reg_data(DEV_CHANNEL_E channel, unsigned char *p_analog_gamma_reg_data, int *p_analog_gamma_reg_data_len)
{
	icn_9706_info_t* p_chip_info = &s_icn_9706_private_info[channel];
	
	memcpy(p_analog_gamma_reg_data, p_chip_info->analog_gamma_reg_data, ICN_9706_ANALOG_GAMMA_LEN);
	*p_analog_gamma_reg_data_len = ICN_9706_ANALOG_GAMMA_LEN;
	
	printf("analog gamma: \n");
	dump_data1(p_analog_gamma_reg_data, ICN_9706_ANALOG_GAMMA_LEN);

	return 0;
}

static int chip_icn_9706_get_digital_gamma_reg_data(DEV_CHANNEL_E channel, unsigned char *p_d3g_r_reg_data, int *p_d3g_r_reg_data_len,
											unsigned char *p_d3g_g_reg_data, int *p_d3g_g_reg_data_len,
											unsigned char *p_d3g_b_reg_data, int *p_d3g_b_reg_data_len)
{
	icn_9706_info_t* p_chip_info = &s_icn_9706_private_info[channel];
	
	memcpy(p_d3g_r_reg_data, p_chip_info->digital_gamma_reg_data[0], ICN_9706_D3G_REG_LEN);
	*p_d3g_r_reg_data_len = ICN_9706_D3G_REG_LEN;
	memcpy(p_d3g_g_reg_data, p_chip_info->digital_gamma_reg_data[1], ICN_9706_ANALOG_GAMMA_LEN);
	*p_d3g_g_reg_data_len = ICN_9706_D3G_REG_LEN;
	memcpy(p_d3g_b_reg_data, p_chip_info->digital_gamma_reg_data[2], ICN_9706_ANALOG_GAMMA_LEN);
	*p_d3g_b_reg_data_len = ICN_9706_D3G_REG_LEN;
	return 0;
}

static icm_chip_t chip_icn_9706 =
{
	.name = "icn9706",

	.reset_private_data = chip_icn_9706_get_and_reset_private_data,
	
	.id = chip_icn_9706_get_chip_id,
	.check_ok = chip_icn_9706_check_chip_ok,
	.read_vcom_otp_times = chip_icn_9706_read_chip_vcom_opt_times,
	.read_vcom_otp_info = chip_icn_9706_read_chip_vcom_opt_info,
	
	.read_vcom = chip_icn_9706_read_vcom,
	.write_vcom = chip_icn_9706_write_vcom,
	
	.write_vcom_otp_value = chip_icn_9706_write_chip_vcom_otp_value,
	.check_vcom_otp_burn_ok = chip_icn_9706_check_vcom_otp_burn_ok,

	.d3g_control = chip_icn_9706_write_d3g_control,
	
	.write_analog_gamma_reg_data = chip_icn_9706_write_analog_gamma_reg_data,
	.read_analog_gamma_reg_data = chip_icn_9706_read_analog_gamma_reg_data,
	.write_fix_analog_gamma_reg_data = chip_icn_9706_write_fix_analog_gamma_reg_data,
	
	.write_digital_gamma_reg_data = chip_icn_9706_write_digital_gamma_reg_data,
	.write_fix_digital_gamma_reg_data = chip_icn_9706_write_fix_digital_gamma_reg_data,

	.burn_gamma_otp_values = chip_icn_9706_burn_gamma_otp_values,
	.check_gamma_otp_values = chip_icn_9706_check_gamma_otp_values,
	
	.get_analog_gamma_reg_data = chip_icn_9706_get_analog_gamma_reg_data,
	.get_digital_gamma_reg_data = chip_icn_9706_get_digital_gamma_reg_data,
};

void icn9706Init()
{
	registerIcChip(&chip_icn_9706);
}

