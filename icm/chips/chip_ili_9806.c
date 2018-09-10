#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "mipi2828.h"
#include "../ic_manager.h"
#include "common.h"

#define ENABLE_VCOM1				(1)
#define ENABLE_VCOM2				(0)


#define MAX_VCOM_OTP_TIMES		(3)

void ili_9806_set_page(DEV_CHANNEL_E channel, unsigned char page_no)
{
	char page[] = {0xFF, 0x98, 0x06, 0x04, 00};	
	page[4] = page_no;
	mipi2828Write2IC(channel, 0xFF, page, 6);
}


void ili_9806_set_dot_mode(DEV_CHANNEL_E channel)
{	
}

void ili_9806_set_row_mode(DEV_CHANNEL_E channel)
{	
}

static int ili_9806_check_id(DEV_CHANNEL_E channel)
{
	char reg[3] = { 0 };
	
    ili_9806_set_page(channel, 0);
	
	// page 1.
	ili_9806_set_page(channel, 1);

	// read id4
    ReadModReg(channel, 1, 0x00, 1, &reg[0]);
	ReadModReg(channel, 1, 0x01, 1, &reg[1]);
	ReadModReg(channel, 1, 0x02, 1, &reg[2]);
	DBG("read id4: %02x:%02x:%02x. \n", reg[2], reg[1], reg[0]);

	if (reg[0] == 0x98 && reg[1] == 0x06 && reg[2] == 0x04)
	{
		return 0;
	}
	else
	{
		printf("chip_ili_9806_check_id: read id failed!\n");
		return -1;
	}

	return 0;
}

static get_otp_times(unsigned char otp_value)
{
	unsigned char times = 0;
	
	switch (otp_value)
	{
		case 0:
			times = 0;
			break;

		case 1:
			times = 1;
			break;

		case 3:
			times = 2;
			break;

		case 7:
			times = 3;
			break;

		default:
			times = 3;
			break;
	}

	return times;
}

static int ili_9806_get_vcom_otp_times(DEV_CHANNEL_E channel, unsigned char* vcom1_otp_times,
										unsigned char* vcom2_otp_times)
{
	// set page 1.
	ili_9806_set_page(channel, 1);

	// read vcom write times
	// 0xE8
	unsigned char vcom_otp_status = 0x00;
	ReadModReg(channel, 1, 0xE8, 1, &vcom_otp_status);
	printf("read vcom write status value: %02x.\n",  vcom_otp_status);

	*vcom1_otp_times = vcom_otp_status & 0x07;
	*vcom2_otp_times = (vcom_otp_status >> 3) & 0x07;

	*vcom1_otp_times = get_otp_times(*vcom1_otp_times);
	*vcom2_otp_times = get_otp_times(*vcom2_otp_times);
	printf("chip_ili_9806_get_vcom_otp_times: %d, %d.\n",  *vcom1_otp_times, *vcom2_otp_times);

	return 0;
}

static int ili_9806_read_nvm_vcom(DEV_CHANNEL_E channel, int index, int *p_vcom)
{
	ili_9806_set_page(channel,1);

	unsigned char vcom1_otp_status = 0x00;
	unsigned char vcom2_otp_status = 0x00;
	unsigned char otp_write_times = 0;
	
	ili_9806_get_vcom_otp_times(channel, &vcom1_otp_status, &vcom2_otp_status);
	DBG("read vcom otp status: 0x%02x 0x%02x.\n", vcom1_otp_status, vcom2_otp_status);
	
	{
		
		unsigned char vcom_otp_read_addr1 = 0x15;
		mipi2828Write2IC(channel, 0xE0, &vcom_otp_read_addr1, 2);
	
		unsigned char vcom_otp_read_addr2 = 0x00;
		if (index == 0)
		{
			printf("Read VCOM1.\n");
			if (vcom1_otp_status == 1)
			{
				vcom_otp_read_addr2 = 0x16;
				otp_write_times = 1;
			}
			else if (vcom1_otp_status == 2)
			{
				vcom_otp_read_addr2 = 0x18;
				otp_write_times = 2;
			}
			else if (vcom1_otp_status == 3)
			{
				vcom_otp_read_addr2 = 0x19;
				otp_write_times = 3;
			}
			else
			{
				otp_write_times = 0;
			}
		}
		else if (index == 1)
		{
			printf("Read VCOM2.\n");
			if (vcom2_otp_status == 1)
			{
				vcom_otp_read_addr2 = 0x1A;
				otp_write_times = 1;
			}
			else if (vcom2_otp_status == 2)
			{
				vcom_otp_read_addr2 = 0x1C;
				otp_write_times = 2;
			}
			else if (vcom2_otp_status == 3)
			{
				vcom_otp_read_addr2 = 0x1D;
				otp_write_times = 3;
			}
			else
			{
				otp_write_times = 0;
			}
		}
		else
		{
			otp_write_times = 0;
		}
		
		mipi2828Write2IC(channel, 0xE1, &vcom_otp_read_addr2, 2);
		//printf("otp write status: times: %d times. \n", otp_write_times);
		printf("vcom read addr: 0x%02x 0x%02x.\n", vcom_otp_read_addr1, vcom_otp_read_addr2);
		

		// nv memory read protection key
		unsigned char reg_data[3] = { 0 };
		reg_data[0] = 0x11;	
		mipi2828Write2IC(channel, 0xE3, reg_data, 2);
		reg_data[0] = 0x66;	
		mipi2828Write2IC(channel, 0xE4, reg_data, 2);
		reg_data[0] = 0x88;	
		mipi2828Write2IC(channel, 0xE5, reg_data, 2);

		// nv memory data read.
		// 0xEA
		unsigned char vcom = 0;
		ReadModReg(channel, 1, 0xEA, 1, &vcom);	
		*p_vcom = vcom;
		printf("read otp vcom value: 0x%02x. %d. \n", vcom, vcom);
	}

	return 0;
}

static int ili_9806_read_vcom(DEV_CHANNEL_E channel, int* p_vcom)
{
	char reg[3] = { 0 };

	ili_9806_set_page(channel, 0);	

	// set page 1.
	ili_9806_set_page(channel, 1);

#if	ENABLE_VCOM1
	ReadModReg(channel, 1, 0x52, 1, &reg[0]);
	ReadModReg(channel, 1, 0x53, 1, &reg[1]);
	//printf("read vcom1 data: %02x:%02x. \n", reg[0], reg[1]);
#endif

#if	ENABLE_VCOM2
	ReadModReg(channel, 1, 0x54, 1, &reg[0]);
	ReadModReg(channel, 1, 0x55, 1, &reg[1]);
	//printf("read vcom2 data: %02x:%02x. \n", reg[0], reg[1]);
#endif
	
	*p_vcom = (reg[0] << 8) | reg[1];
	return 0;
}

int ili_9806_write_vcom(DEV_CHANNEL_E channel, int vcom, int otp_burned)
{
	//printf("chip_ili_9806_write_vcom channel: %d. [%d, 0x%02x] ...\n", channel, vcom, vcom);
	char reg[3] = { 0 };

	//channel = 4;
	
	ili_9806_set_page(channel, 0);

	// set page 1.
	ili_9806_set_page(channel, 1);

	unsigned char temp = 0x00;
	if (otp_burned)
		temp = 0x11;

	mipi2828Write2IC(channel, 0x56, &temp, sizeof(temp));

#if ENABLE_VCOM1
	unsigned char temp_vcom = (vcom >> 8) & 0x01;
	mipi2828Write2IC(channel, 0x52, &temp_vcom, sizeof(temp_vcom));

	temp_vcom = vcom & 0xFF;
	mipi2828Write2IC(channel, 0x53, &temp_vcom, sizeof(temp_vcom));
#endif
	return 0;
}


static int ili_9806_burn_vcom(DEV_CHANNEL_E channel, int vcom)
{
	unsigned char reg = 0;
	int power_channel = 0;
	
	// reset
	ili_9806_set_page(channel, 0);
	mipi2828Write2IC(channel, 0x01, NULL, 0);

	// sleep out	
	mipi2828Write2IC(channel, 0x11, NULL, 0);

	_msleep(120);

	ili_9806_set_page(channel, 1);

	// check E8
	unsigned char vcom1_count = 0x00;
	unsigned char vcom2_count = 0x00;
	ili_9806_get_vcom_otp_times(channel, &vcom1_count, &vcom2_count);

	if (vcom1_count == 0x07)
		return -1;

	if (vcom2_count == 0x07)
		return -1;

	// vsp power on
	if (channel == 0 || channel == 1)
	{
		pwrContorl(true, PWR_CHANNEL_LEFT);
	}
	else
		pwrContorl(true, PWR_CHANNEL_RIGHT);

	_msleep(20);

	// write addr.
	// vcom1
	#if	ENABLE_VCOM1
	{
		unsigned char vcom_otp_addr = 0x04;
		unsigned char vcom_data = (vcom >> 8) & 0x01;		

		DBG("write vcom1: %02x ", vcom_data);
		mipi2828Write2IC(channel, 0xE1, &vcom_otp_addr, sizeof(vcom_otp_addr));
		mipi2828Write2IC(channel, 0xE0, &vcom_data, sizeof(vcom_data));

		vcom_otp_addr = 0x05;
		vcom_data = vcom & 0xFF;
		DBG("%02x.\n", vcom_data);
		mipi2828Write2IC(channel, 0xE1, &vcom_otp_addr, sizeof(vcom_otp_addr));
		mipi2828Write2IC(channel, 0xE0, &vcom_data, sizeof(vcom_data));

		// nv memory write protection key
		unsigned char reg_data[2] = { 0 };
		reg_data[0] = 0x55;	
		mipi2828Write2IC(channel, 0xE3, reg_data, sizeof(reg_data));
		reg_data[0] = 0xAA;	
		mipi2828Write2IC(channel, 0xE4, reg_data, sizeof(reg_data));
		reg_data[0] = 0x66;	
		mipi2828Write2IC(channel, 0xE5, reg_data, sizeof(reg_data));

		unsigned char otp_is_busy = 1;
		do
		{
			_msleep(10);
			ReadModReg(channel, 1, 0xE9, 1, &otp_is_busy);
			DBG("otp busy: %#x.\n", otp_is_busy);
		}while (otp_is_busy == 0x01);
	}
	#endif
	
	// vsp power off
	if (channel == 0 || channel == 1)
	{
		pwrContorl(false, PWR_CHANNEL_LEFT);
	}
	else
		pwrContorl(false, PWR_CHANNEL_RIGHT);

	// reset
	mipi2828Write2IC(channel, 0x01, NULL, 0);

	_msleep(200);

	return 0;
}

static int ili_9806_otp_check_vcom(DEV_CHANNEL_E channel, int new_vcom, int old_otp_times)
{
	// check id
	int val = ili_9806_check_id(channel);
	if (val != 0)
	{
		printf("chip_ili_9806_otp_check_vcom: check module id failed!\n");
		return -1;
	}

	#if 1
	// check otp times
	unsigned char vcom1_otp_time = 0;
	unsigned char vcom2_otp_times = 0;
	val = ili_9806_get_vcom_otp_times(channel, &vcom1_otp_time, &vcom2_otp_times);
	if (val != 0)
	{
		printf("chip_ili_9806_otp_check_vcom: get vcom otp times failed!\n");
		return -2;
	}

	if (vcom1_otp_time <= old_otp_times)
	{
		printf("chip_ili_9806_otp_check_vcom: vcom otp times is not change! last: %d. now: %d.\n",
				old_otp_times, vcom1_otp_time);
		return -3;
	}
	#endif
	
	// check vcom
	int index = 0;
	int vcom = 0;
	val = ili_9806_read_nvm_vcom(channel, index, &vcom);
	if (val != 0)
	{
		printf("chip_ili_9806_otp_check_vcom: read nvm vcom failed!\n");
		return -4;
	}

	if (vcom != new_vcom)
	{
		printf("chip_ili_9806_otp_check_vcom: nvm vcom is not change! read vcom: %d. burn vcom: %d.\n",
				vcom, new_vcom);
		return -5;
	}
	
	return 0;
}


#if 1

typedef struct tag_ili_9806e_info
{
	unsigned char id[3];

	unsigned char id_ok;	// 0: id_error; 1: id_ok;
	
	unsigned int vcom1;
	unsigned int  vcom1_otp_times;
	
	unsigned int vcom2;
	unsigned int  vcom2_otp_times;
}ili_9806e_info_t;

static ili_9806e_info_t s_9806e_private_info[DEV_CHANNEL_NUM] = { 0 };
// read chip id

// check chip id

// read chip otp times

// read chip otp values


static int chip_ili_9806e_get_chip_id(DEV_CHANNEL_E channel, unsigned char *p_id_data, int id_data_len)
{
	char reg[3] = { 0 };
	
    ili_9806_set_page(channel, 0);
	
	// page 1.
	ili_9806_set_page(channel, 1);

	// read id4
    ReadModReg(channel, 1, 0x00, 1, &reg[0]);
	ReadModReg(channel, 1, 0x01, 1, &reg[1]);
	ReadModReg(channel, 1, 0x02, 1, &reg[2]);
	printf("chip channel: %d, channel: %d, read id4: %02x:%02x:%02x. \n",
				channel, channel, reg[2], reg[1], reg[0]);

	if (reg[0] == 0x98 && reg[1] == 0x06 && reg[2] == 0x04)
	{
		if (p_id_data && id_data_len >= 3)
		{
			p_id_data[0] = 0x98;
			p_id_data[1] = 0x06;
			p_id_data[2] = 0x04;
			return 3;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		printf("chip channel: %d, channel: %d, chip_ili_9806e_get_chip_id: read id failed!\n",
				channel, channel);
		
		return -1;
	}
	
	return -1;
}

static int chip_ili_9806e_check_chip_ok(DEV_CHANNEL_E channel)
{	
	// read and check id;
	chip_ili_9806e_get_chip_id(channel, s_9806e_private_info[channel].id,
								sizeof(s_9806e_private_info[channel].id));

	unsigned char* p_id = s_9806e_private_info[channel].id;
	if ( (p_id[0] != 0x98) || (p_id[1] != 0x06) || (p_id[2] != 0x04) )
	{
		printf("chip_ili_9806e_check_chip_ok error: Invalid chip id: %02x, %02x, %02x.\n",
					p_id[0], p_id[1], p_id[2]);
		
		s_9806e_private_info[channel].id_ok = 0;

		return -1;
	}

	s_9806e_private_info[channel].id_ok = 1;

	// read vcom otp times
	unsigned char vcom1_otp_times = 0;
	unsigned char vcom2_otp_times = 0;
	ili_9806_get_vcom_otp_times(channel, &vcom1_otp_times, &vcom2_otp_times);
	s_9806e_private_info[channel].vcom1_otp_times = vcom1_otp_times;
	s_9806e_private_info[channel].vcom2_otp_times = vcom2_otp_times;

	// read vcom otp value
	unsigned int vcom1_otp_value = 0;
	unsigned int vcom2_otp_value = 0;
	ili_9806_read_nvm_vcom(channel, 0, &vcom1_otp_value);
	ili_9806_read_nvm_vcom(channel, 1, &vcom2_otp_value);
	s_9806e_private_info[channel].vcom1 = vcom1_otp_value;
	s_9806e_private_info[channel].vcom2 = vcom2_otp_value;

	printf("chip_ili_9806e_check_chip_ok:chip:%d, mipi: %d. vcom1_otp_times: %d, vcom2_otp_times: %d."
			"vcom1_otp_value: %d. vcom2_otp_value: %d.\n", channel, channel,
				s_9806e_private_info[channel].vcom1_otp_times,
				s_9806e_private_info[channel].vcom2_otp_times,
				s_9806e_private_info[channel].vcom1,
				s_9806e_private_info[channel].vcom2);
	return 0;
}

static void chip_ili_9806e_get_and_reset_private_data(DEV_CHANNEL_E channel)
{
	printf("chip_ili_9806e_get_and_reset_private_data.\n");
	s_9806e_private_info[channel].id_ok = 0;
	memset(s_9806e_private_info[channel].id, 0, sizeof(s_9806e_private_info[channel].id));
	s_9806e_private_info[channel].vcom1 = 0;
	s_9806e_private_info[channel].vcom1_otp_times = 0;
	s_9806e_private_info[channel].vcom2 = 0;
	s_9806e_private_info[channel].vcom2_otp_times = 0;
}

static int chip_ili_9806e_read_chip_vcom_opt_times(DEV_CHANNEL_E channel, int* p_otp_vcom_times)
{
	if (s_9806e_private_info[channel].id_ok == 1)
	{
		// read vcom otp times
		unsigned char vcom1_otp_times = 0;
		unsigned char vcom2_otp_times = 0;
		ili_9806_get_vcom_otp_times(channel, &vcom1_otp_times, &vcom2_otp_times);

		*p_otp_vcom_times = vcom1_otp_times;

		printf("chip_ili_9806e_read_chip_vcom_opt_times: end\n");

		return 0;
	}
	
	printf("chip_ili_9806e_read_chip_vcom_opt_times error!\n");
	return -1;
}


static int chip_ili_9806e_read_chip_vcom_opt_info(DEV_CHANNEL_E channel, int* p_otp_vcom_times,
												int *p_otp_vcom_value)
{
	if (s_9806e_private_info[channel].id_ok == 1)
	{
		// read vcom otp times
		unsigned char vcom1_otp_times = 0;
		unsigned char vcom2_otp_times = 0;
		ili_9806_get_vcom_otp_times(channel, &vcom1_otp_times, &vcom2_otp_times);

		// read vcom otp value
		int vcom1_otp_value = 0;
		int vcom2_otp_value = 0;
		ili_9806_read_nvm_vcom(channel, 0, &vcom1_otp_value);
		ili_9806_read_nvm_vcom(channel, 1, &vcom2_otp_value);

		*p_otp_vcom_times = vcom1_otp_times;
		*p_otp_vcom_value = vcom1_otp_value;

		printf("chip_ili_9806e_read_chip_vcom_opt_info: end\n");

		return 0;
	}
	
	printf("chip_ili_9806e_read_chip_vcom_opt_info error: unknow chip!\n");
	return -1;
}

static int chip_ili_9806e_read_vcom(DEV_CHANNEL_E channel, int* p_vcom_value)
{
	if (s_9806e_private_info[channel].id_ok == 1)
	{
		int vcom = 0;
		ili_9806_read_vcom(channel, &vcom);
		*p_vcom_value = vcom;
		//printf("chip_ili_9806e_read_vcom: end\n");

		return 0;
	}
	printf("chip_ili_9806e_read_vcom error: unknow chip!\n");
	return -1;
}


static int chip_ili_9806e_write_vcom(DEV_CHANNEL_E channel, int vcom_value)
{
	//printf("chip_ili_9806e_write_vcom: chip channel = %d, channel: %d. ...\n",
	//		channel, channel);
	
	if (s_9806e_private_info[channel].id_ok == 1)
	{
		if (s_9806e_private_info[channel].vcom1_otp_times == 0)
		{
			ili_9806_write_vcom(channel, vcom_value, 0);
		}
		else
		{
			ili_9806_write_vcom(channel, vcom_value, 1);
		}
		
		//printf("chip_ili_9806e_read_vcom: end\n");

		return 0;
	}
	
	printf("chip_ili_9806e_write_vcom error: unknow chip!\n");
	
	return -1;
}

static int chip_ili_9806e_write_chip_vcom_otp_value(DEV_CHANNEL_E channel,int otp_vcom_value)
{

	if (s_9806e_private_info[channel].id_ok == 1)
	{
		int val = 0;
		val = ili_9806_burn_vcom(channel, otp_vcom_value);
		return val;
	}

	printf("chip_ili_9806e_write_chip_vcom_otp_value error!\n");
	return -1;
}


static int chip_ili_9806e_check_vcom_otp_burn_ok(DEV_CHANNEL_E channel, int vcom_value, int last_otp_times, int *p_read_vcom)
{
	// check id 
	int val = chip_ili_9806e_check_chip_ok(channel);
	if (val != 0)
	{
		printf("chip_ili_9806e_check_vcom_otp_burn_ok error: check id error, val = %d.\n", val);
		return E_ICM_READ_ID_ERROR;
	}

	if (p_read_vcom)
		*p_read_vcom = s_9806e_private_info[channel].vcom1;

	#if 1
	// check times
	if (s_9806e_private_info[channel].vcom1_otp_times != last_otp_times + 1)
	{
		printf("chip_ili_9806e_check_vcom_otp_burn_ok error: check vcom otp times error. last times: %d, now: %d.\n", 
				last_otp_times, s_9806e_private_info[channel].vcom1_otp_times);

		#ifdef ENABlE_OTP_BURN
		return E_ICM_VCOM_TIMES_NOT_CHANGE;
		#else
		return E_ICM_OK;
		#endif
	}
	#endif

	// check vcom value.
	if (s_9806e_private_info[channel].vcom1 != vcom_value)
	{
		printf("chip_ili_9806e_check_vcom_otp_burn_ok error: check vcom value error. write: %d, read: %d.\n", 
				vcom_value, s_9806e_private_info[channel].vcom1);

		return E_ICM_VCOM_VALUE_NOT_CHANGE;
	}

	return 0;
}
#endif

static icm_chip_t chip_ili_9806e =
{
	.name = "ili9806e",

	.reset_private_data = chip_ili_9806e_get_and_reset_private_data,

	.id = chip_ili_9806e_get_chip_id,
	.check_ok = chip_ili_9806e_check_chip_ok,
	.read_vcom_otp_times = chip_ili_9806e_read_chip_vcom_opt_times,
	.read_vcom_otp_info = chip_ili_9806e_read_chip_vcom_opt_info,

	.read_vcom = chip_ili_9806e_read_vcom,
	.write_vcom = chip_ili_9806e_write_vcom,
	
	.write_vcom_otp_value = chip_ili_9806e_write_chip_vcom_otp_value,
	.check_vcom_otp_burn_ok = chip_ili_9806e_check_vcom_otp_burn_ok,
};

void ili9806eInit()
{
	registerIcChip(&chip_ili_9806e);
}

