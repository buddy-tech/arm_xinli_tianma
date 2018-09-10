#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "unistd.h"

#include "mipi2828.h"
#include "../ic_manager.h"

unsigned char qc_do_sleep_out(DEV_CHANNEL_E channel)
{
	unsigned char param = 0x00;
	mipi2828Write2IC(channel, 0xE0, &param, sizeof(param));
	mipi2828Write2IC(channel, 0x28, NULL, 0);
	_msleep(120);
}

unsigned char qc_do_sleep_in(DEV_CHANNEL_E channel)
{
	unsigned char param = 0x00;
	mipi2828Write2IC(channel, 0xE0, &param, sizeof(param));
	mipi2828Write2IC(channel, 0x29, NULL, 0);
	_msleep(120);
}


unsigned char qc_read_otp_data_reg(DEV_CHANNEL_E channel, unsigned short otp_index)
{
	unsigned char param = otp_index >> 8;
	mipi2828Write2IC(channel, 0xEA, &param, sizeof(param));

	param = otp_index & 0xFF;
	mipi2828Write2IC(channel, 0xEB, &param, sizeof(param));

	param = 0x11;
	mipi2828Write2IC(channel, 0xEC, &param, sizeof(param));

	param = 0x10;
	mipi2828Write2IC(channel, 0xEC, &param, sizeof(param));

	int read_len = 1;
	int real_read_len = 0;
	unsigned char read_buffer[4] = { 0 };
	real_read_len = ReadModReg(channel, 1, 0xED, read_len, read_buffer);
	DBG("read data len = %d, data: %02x.\n", real_read_len, read_buffer[0]);
	
	return read_buffer[0];
}

unsigned char qc_read_reg(DEV_CHANNEL_E channel, unsigned char page, unsigned char reg)
{
	unsigned char reg_data = 0;
	unsigned char param = page;
	mipi2828Write2IC(channel, 0xE0, &param, sizeof(param));

	int read_len = 1;
	int real_read_len = 0;
	unsigned char read_buffer[4] = { 0 };
	real_read_len = ReadModReg(channel, 1, reg, read_len, read_buffer);

	reg_data = read_buffer[0];
	printf("read data len = %d, data: %02x.\n", real_read_len, reg_data);

	
	return reg_data;
}

static void enableExtend(DEV_CHANNEL_E channel)
{
	unsigned char param = 0x00;
	mipi2828Write2IC(channel, 0xE0, &param, sizeof(param));
	param = 0x93;
	mipi2828Write2IC(channel, 0xE1, &param, sizeof(param));
	param = 0x65;
	mipi2828Write2IC(channel, 0xE2, &param, sizeof(param));
	param = 0xF8;
	mipi2828Write2IC(channel, 0xE3, &param, sizeof(param));
	param = 0x00;
	mipi2828Write2IC(channel, 0xE5, &param, sizeof(param));
}

static void initInternalPower(DEV_CHANNEL_E channel)
{
	unsigned char param = 0x01;
	mipi2828Write2IC(channel, 0xE0, &param, sizeof(param));

	param = 0x01;
	mipi2828Write2IC(channel, 0x1D, &param, sizeof(param));

	param = 0x03;
	mipi2828Write2IC(channel, 0x1E, &param, sizeof(param));

	param = 0x00;
	mipi2828Write2IC(channel, 0xE0, &param, sizeof(param));

	param = 0x02;
	mipi2828Write2IC(channel, 0xEC, &param, sizeof(param));

	param = 0x00;
	mipi2828Write2IC(channel, 0xE0, &param, sizeof(param));

	mipi2828Write2IC(channel, 0x11, NULL, 0);
}

static void writeVcomReg(DEV_CHANNEL_E channel, unsigned char vcomH, unsigned char vcomL)
{
	unsigned char param = 0x01;
	mipi2828Write2IC(channel, 0xE0, &param, sizeof(param));

	param = vcomH;
	mipi2828Write2IC(channel, 0x00, &param, sizeof(param));

	param = vcomL;
	mipi2828Write2IC(channel, 0x01, &param, sizeof(param));
}

static void enableUpKey(DEV_CHANNEL_E channel)
{
	unsigned char param = 0x00;
	mipi2828Write2IC(channel, 0xE0, &param, sizeof(param));

	param = 0x5A;
	mipi2828Write2IC(channel, 0x84, &param, sizeof(param));

	param = 0xA5;
	mipi2828Write2IC(channel, 0x85, &param, sizeof(param));
}

static void vcomOtpIndex(DEV_CHANNEL_E channel)
{
	unsigned char param = 0x00;
	mipi2828Write2IC(channel, 0xEA, &param, sizeof(param));

	param = 0x26;
	mipi2828Write2IC(channel, 0xEB, &param, sizeof(param));

	param = 0x03;
	mipi2828Write2IC(channel, 0xEC, &param, sizeof(param));
}

static void otpEnd(DEV_CHANNEL_E channel)
{
	unsigned char param = 0x00;
	mipi2828Write2IC(channel, 0x84, &param, sizeof(param));

	param = 0x00;
	mipi2828Write2IC(channel, 0x85, &param, sizeof(param));
}

int qc_otp_vcom(DEV_CHANNEL_E channel, unsigned char vcom_h, unsigned char vcom_l, int ptn_id)
{	
	DBG("%s\r\n", __FUNCTION__);
	
	// enable extend command
	enableExtend(channel);

	unsigned char param = 0x00;
	mipi2828Write2IC(channel, 0xE0, &param, sizeof(param));
	mipi2828Write2IC(channel, 0x28, NULL, 0);
	mipi2828Write2IC(channel, 0x10, NULL, 0);

	_msleep(240);

	// internal power
	initInternalPower(channel);

	_msleep(240);

	// write vcom reg
	writeVcomReg(channel, vcom_h, vcom_l);

	// enable up key
	enableUpKey(channel);
	
	// otp index
	vcomOtpIndex(channel);
	_msleep(200);
	
	// end.
	otpEnd(channel);

	client_pg_shutON(0, 0, ptn_id);

	char modelName[256] = "";
	read_cur_module_name(modelName);
	client_pg_shutON(1, modelName, ptn_id);
	return 0;
}

#define OTP_USED_INTERNAL_VOLT		(0)
#define VCOM_R_TEST					(1)
int qc_otp_vcom_by_mtp(DEV_CHANNEL_E channel, unsigned char vcom_h, unsigned char vcom_l, int ptn_id)
{
	DBG("%s\r\n", __FUNCTION__);
	
	// enable extend command
	enableExtend(channel);

	// E5
	unsigned char param = 0x00;
	mipi2828Write2IC(channel, 0xE5, &param, sizeof(param));

	param = 0x00;
	mipi2828Write2IC(channel, 0xE0, &param, sizeof(param));

	mipi2828Write2IC(channel, 0x28, NULL, 0);

	mipi2828Write2IC(channel, 0x10, NULL, 0);

	_msleep(1000);

	// internal power
	initInternalPower(channel);
	_msleep(240);

	// write vcom reg
	writeVcomReg(channel, vcom_h, vcom_l);

	// enable up key
	enableUpKey(channel);
	
	// otp index
	vcomOtpIndex(channel);
	_msleep(200);
	
	// end.
	otpEnd(channel);

	client_pg_shutON(0, 0, ptn_id);

	char modelName[256] = "";
	read_cur_module_name(modelName);
	client_pg_shutON(1, modelName, ptn_id);

	return 0;
}

static void writeIdReg(DEV_CHANNEL_E channel, unsigned char id[])
{
	unsigned char param = 0x00;
	mipi2828Write2IC(channel, 0xE0, &param, sizeof(param));

	param = id[0];
	mipi2828Write2IC(channel, 0x78, &param, sizeof(param));

	param = id[1];
	mipi2828Write2IC(channel, 0x79, &param, sizeof(param));

	param = id[2];
	mipi2828Write2IC(channel, 0x7A, &param, sizeof(param));
}

static void idOtpIndex(DEV_CHANNEL_E channel)
{
	unsigned char param = 0x00;
	mipi2828Write2IC(channel, 0xEA, &param, sizeof(param));

	param = 0x06;
	mipi2828Write2IC(channel, 0xEB, &param, sizeof(param));

	param = 0x03;
	mipi2828Write2IC(channel, 0xEC, &param, sizeof(param));
}

int qc_otp_id(DEV_CHANNEL_E channel, unsigned char id1, unsigned char id2, unsigned short id3, int ptn_id)
{
	printf("ID OTP:\n");
	
	// enable extend command
	enableExtend(channel);

	// E5
	unsigned char param = 0x00;
	mipi2828Write2IC(channel, 0xE5, &param, sizeof(param));
	
	param = 0x00;
	mipi2828Write2IC(channel, 0xE0, &param, sizeof(param));

	mipi2828Write2IC(channel, 0x28, NULL, 0);

	mipi2828Write2IC(channel, 0x10, NULL, 0);

	_msleep(240);

	// internal power
	initInternalPower(channel);
	_msleep(240);

	// write id reg
	unsigned id[3] = {0};
	id[0] = id1;
	id[1] = id2;
	id[2] = id3;
	writeIdReg(channel, id);

	// enable up key
	enableUpKey(channel);

	// otp index
	idOtpIndex(channel);
	_msleep(200);
	
	// end.
	otpEnd(channel);

	_msleep(200);

	param = 0x00;
	mipi2828Write2IC(channel, 0xE0, &param, sizeof(param));

	mipi2828Write2IC(channel, 0x29, NULL, 0);

	client_pg_shutON(0, 0, ptn_id);

	char modelName[256] = "";
	read_cur_module_name(modelName);
	client_pg_shutON(1, modelName, ptn_id);

	return 0;
}


int qc_otp_data_read_id(DEV_CHANNEL_E channel, unsigned char* p_id1, unsigned char *p_id2, unsigned char* p_id3,
						unsigned char* p_otp_times)
{
	unsigned short id_otp_index = 0x06;
	unsigned char otp_times = 0;
	unsigned char id_reg_page = 0;
	unsigned char id_otp_time_reg = 0x7C;
	
	qc_do_sleep_out(channel);

	// read id otp times.
	otp_times = qc_read_reg(channel, id_reg_page, id_otp_time_reg);
	*p_otp_times = otp_times;
	
	switch(otp_times)
	{
		case 1:
			id_otp_index = 0x06;
			break;

		case 2:
			id_otp_index = 0x0B;
			break;

		case 3:
			id_otp_index = 0x10;
			break;

		case 4:
			id_otp_index = 0x15;
			break;

		case 5:
			id_otp_index = 0x1A;
			break;

		default:
			printf("qc_otp_data_read_id: Invalid OTP Times. %d.\n", otp_times);
			qc_do_sleep_in(channel);
			return -1;
			break;
	}

	printf("id otp times: %d, otp read offset: 0x%02X.\n", otp_times, id_otp_index);
	
	
	*p_id1 = qc_read_otp_data_reg(channel, id_otp_index + 1);
	*p_id2 = qc_read_otp_data_reg(channel, id_otp_index + 2);
	*p_id3 = qc_read_otp_data_reg(channel, id_otp_index + 3);
	
	qc_do_sleep_in(channel);

	*p_otp_times = otp_times;

	return 0;
}

int qc_otp_data_read_vcom(DEV_CHANNEL_E channel, unsigned char* p_vcom_h, unsigned char *p_vcom_l, unsigned char* p_otp_times)
{
	unsigned short vcom_otp_index = 0x26;
	unsigned char otp_times = 0;
	unsigned char vcom_reg_page = 1;
	unsigned char vcom_otp_time_reg = 0x02;
	
	qc_do_sleep_out(channel);

	// read id otp times.
	otp_times = qc_read_reg(channel, vcom_reg_page, vcom_otp_time_reg);
	*p_otp_times = otp_times;
	
	switch(otp_times)
	{
		case 1:
			vcom_otp_index = 0x26;
			break;

		case 2:
			vcom_otp_index = 0x28;
			break;

		case 3:
			vcom_otp_index = 0x2A;
			break;

		case 4:
			vcom_otp_index = 0x2C;
			break;

		case 5:
			vcom_otp_index = 0x2E;
			break;

		default:
			printf("qc_otp_data_read_vcom: Invalid OTP Times. %d.\n", otp_times);
			qc_do_sleep_in(channel);
			return -1;
			break;
	}

	printf("vcom otp times: %d, otp read offset: 0x%02X.\n", otp_times, vcom_otp_index);
	
	
	*p_vcom_h = qc_read_otp_data_reg(channel, vcom_otp_index);
	*p_vcom_l = qc_read_otp_data_reg(channel, vcom_otp_index + 1);
	
	*p_otp_times = otp_times;
	
	qc_do_sleep_in(channel);

	return 0;
}

int qc_set_vcom_value(DEV_CHANNEL_E channel, unsigned short vcom)
{
	DBG("channel: %d, vcom: %d. 0x%04X.\n", channel, vcom, vcom);
	writeVcomReg(channel, vcom >> 8, vcom & 0xFF);
	return 0;
}

#define CABC_CHANNEL_1_INPUT_PIN		(71)
#define CABC_CHANNEL_2_INPUT_PIN		(59)
#define CABC_CHANNEL_3_INPUT_PIN		(62)
#define CABC_CHANNEL_4_INPUT_PIN		(55)


int init_cabc_io_config()
{
	// id check channel 1.
	HI_UNF_GPIO_SetDirBit(CABC_CHANNEL_1_INPUT_PIN, 1);

	// id check channel 2.
	HI_UNF_GPIO_SetDirBit(CABC_CHANNEL_2_INPUT_PIN, 1);

	// id check channel 3.
	HI_UNF_GPIO_SetDirBit(CABC_CHANNEL_3_INPUT_PIN, 1);

	// id check channel 4.
	HI_UNF_GPIO_SetDirBit(CABC_CHANNEL_4_INPUT_PIN, 1);
}


int qc_cabc_test(DEV_CHANNEL_E channel, unsigned short *p_pwm_freq, unsigned short *p_duty)
{
	unsigned int high_time = 0;
	unsigned int low_time = 0;
	unsigned int volt_status = 0;

	if (p_pwm_freq == NULL)
	{
		printf("qc_cabc_test: p_pwm_freq = NULL.\n");
		return -1;
	}

	if (p_duty == NULL)
	{
		printf("qc_cabc_test: p_duty = NULL.\n");
		return -1;
	}

	switch(channel)
	{
		case 1:	// GPIO 71. => FPGA ID_Check Channel 2.
		case 2:
		{	
			printf("channel: 1.\n", channel);
			unsigned short high_time_l = fpga_read(0xE0);
			unsigned short high_time_h = fpga_read(0xE1);
			unsigned short low_time_l = fpga_read(0xE4);
			unsigned short low_time_h = fpga_read(0xE5);

			high_time = (high_time_h << 16) | high_time_l;
			low_time = (low_time_h << 16) | low_time_l;
			volt_status = gpin_read_value(CABC_CHANNEL_1_INPUT_PIN);
		}
		break;
		
		case 3:
		case 4:	// GPIO 55 => FPGA ID_Check Channel 1.
		{
			printf("channel: 4.\n", channel);
			unsigned short high_time_l = fpga_read(0xDE);
			unsigned short high_time_h = fpga_read(0xDF);
			unsigned short low_time_l = fpga_read(0xE2);
			unsigned short low_time_h = fpga_read(0xE3);

			high_time = (high_time_h << 16) | high_time_l;
			low_time = (low_time_h << 16) | low_time_l;
			volt_status = gpin_read_value(CABC_CHANNEL_4_INPUT_PIN);
		}
		break;
		
		default:
		break;
	}

	printf("high time: %d, %#x.\n", high_time, high_time);
	printf("low time: %d, %#x.\n", low_time, low_time);

	if (high_time == 0 || low_time == 0)
	{
		printf("qc_cabc_test: Invalid high time or low time! volt_status = %d.\n", volt_status);
		if (volt_status == 0)
		{
			printf("duty: 0.\n");
			*p_duty = 0;
		}
		else
		{
			printf("duty: 100.\n");
			*p_duty = 100;
		}

		*p_pwm_freq = 0;
		
	}
	else
	{
		double f_duty = (high_time * 100.00) / (high_time + low_time);
		double f_freq = (1 * 1000 * 1000 * 1000.00F) / ((high_time + low_time) * 20);

		printf("freq: %f, duty: %f.\n", f_freq, f_duty);

		*p_pwm_freq = f_freq;
		*p_duty = f_duty;
	}
}

#define ENABLE_VCOM1				(1)
#define ENABLE_VCOM2				(0)
#define MAX_VCOM_OTP_TIMES		(3)

void jd_9367_set_page(DEV_CHANNEL_E channel, unsigned char page_no)
{
	mipi2828Write2IC(channel, 0xE0, &page_no, sizeof(page_no));
}

static int jd_9367_check_id(DEV_CHANNEL_E channel)
{
	char reg[3] = { 0 };
	
    jd_9367_set_page(channel, 0);
	DBG("read id4: %02x:%02x:%02x. \n", reg[2], reg[1], reg[0]);

	if (reg[0] == 0x93 && reg[1] == 0x67 && reg[2] == 0xFF)
	{
		return 0;
	}
	else
	{
		DBG_ERR("jd_9367_check_id: read id failed!\n");
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

static int jd_9367_get_vcom_otp_times(DEV_CHANNEL_E channel, unsigned char* vcom1_otp_times,
										unsigned char* vcom2_otp_times)
{
	// set page 1.
	jd_9367_set_page(channel, 1);

	// read vcom write times
	// 0xE8
	unsigned char vcom_otp_status = 0x00;
	ReadModReg(channel, 1, 0xE8, 1, &vcom_otp_status);
	printf("read vcom write status value: %02x.\n",  vcom_otp_status);

	*vcom1_otp_times = vcom_otp_status & 0x07;
	*vcom2_otp_times = (vcom_otp_status >> 3) & 0x07;

	*vcom1_otp_times = get_otp_times(*vcom1_otp_times);
	*vcom2_otp_times = get_otp_times(*vcom2_otp_times);
	printf("chip_jd_9367_get_vcom_otp_times: %d, %d.\n",  *vcom1_otp_times, *vcom2_otp_times);

	return 0;
}

static int jd_9367_read_nvm_vcom(DEV_CHANNEL_E channel, int index, int *p_vcom)
{
	jd_9367_set_page(channel,1);

	unsigned char vcom1_otp_status = 0x00;
	unsigned char vcom2_otp_status = 0x00;
	unsigned char otp_write_times = 0;
	
	jd_9367_get_vcom_otp_times(channel, &vcom1_otp_status, &vcom2_otp_status);
	printf("read vcom otp status: 0x%02x 0x%02x.\n", vcom1_otp_status, vcom2_otp_status);
	
	{
		
		unsigned char vcom_otp_read_addr1 = 0x15;
		mipi2828Write2IC(channel, 0xE0, &vcom_otp_read_addr1, sizeof(vcom_otp_read_addr1));
	
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
		
		mipi2828Write2IC(channel, 0xE1, &vcom_otp_read_addr2, sizeof(vcom_otp_read_addr2));
		//printf("otp write status: times: %d times. \n", otp_write_times);
		printf("vcom read addr: 0x%02x 0x%02x.\n", vcom_otp_read_addr1, vcom_otp_read_addr2);
		

		// nv memory read protection key
		unsigned char reg_data[2] = { 0 };
		reg_data[0] = 0x11;	
		mipi2828Write2IC(channel, 0xE3, reg_data, sizeof(reg_data));
		reg_data[0] = 0x66;	
		mipi2828Write2IC(channel, 0xE4, reg_data, sizeof(reg_data));
		reg_data[0] = 0x88;	
		mipi2828Write2IC(channel, 0xE5, reg_data, sizeof(reg_data));

		// nv memory data read.
		// 0xEA
		unsigned char vcom = 0;
		ReadModReg(channel, 1, 0xEA, 1, &vcom);	
		*p_vcom = vcom;
		printf("read otp vcom value: 0x%02x. %d. \n", vcom, vcom);
	}

	return 0;
}

static int jd_9367_read_vcom(DEV_CHANNEL_E channel, int* p_vcom)
{
	char reg[3] = { 0 };

	jd_9367_set_page(channel, 0);	

	// set page 1.
	jd_9367_set_page(channel, 1);

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

int jd_9367_write_vcom(DEV_CHANNEL_E channel, int vcom, int otp_burned)
{
	//printf("chip_ili_9806_write_vcom channel: %d. [%d, 0x%02x] ...\n", channel, vcom, vcom);
	char reg[3] = { 0 };

	//channel = 4;
	
	jd_9367_set_page(channel, 0);

	// set page 1.
	jd_9367_set_page(channel, 1);

	unsigned char temp = 0x00;
	if (otp_burned)
		temp = 0x11;

	mipi2828Write2IC(channel, 0x56, &temp, sizeof(temp));

#if ENABLE_VCOM1
	unsigned char temp_vcom = (vcom >> 8) & 0x01;
	mipi2828Write2IC(channel, 0x52, &temp_vcom, sizeof(temp));

	temp_vcom = vcom & 0xFF;
	mipi2828Write2IC(channel, 0x53, &temp_vcom, sizeof(temp));
#endif
	return 0;
}


static int jd_9367_burn_vcom(DEV_CHANNEL_E channel, int vcom)
{
	unsigned char reg = 0;
	// reset
	jd_9367_set_page(channel, 0);
	mipi2828Write2IC(channel, 0x01, NULL, 0);

	// sleep out	
	mipi2828Write2IC(channel, 0x11, NULL, 0);

	_msleep(120);

	jd_9367_set_page(channel, 1);

	// check E8
	unsigned char vcom1_count = 0x00;
	unsigned char vcom2_count = 0x00;
	jd_9367_get_vcom_otp_times(channel, &vcom1_count, &vcom2_count);

	if (vcom1_count == 0x07)
		return -1;

	if (vcom2_count == 0x07)
		return -1;

	_msleep(20);

	// vcom1
	#if	ENABLE_VCOM1
	{
		unsigned char vcom_otp_addr = 0x04;
		unsigned char vcom_data = (vcom >> 8) & 0x01;		

		DBG("write vcom1: %02x ", vcom_data);
		mipi2828Write2IC(channel, 0xE1, &vcom_otp_addr, sizeof(unsigned char));
		mipi2828Write2IC(channel, 0xE0, &vcom_data, sizeof(unsigned char));

		vcom_otp_addr = 0x05;
		vcom_data = vcom & 0xFF;
		DBG("%02x.\n", vcom_data);
		mipi2828Write2IC(channel, 0xE1, &vcom_otp_addr, sizeof(unsigned char));
		mipi2828Write2IC(channel, 0xE0, &vcom_data, sizeof(unsigned char));

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

	// reset
	mipi2828Write2IC(channel, 0x01, NULL, 0);
	_msleep(200);

	return 0;
}

static int jd_9367_otp_check_vcom(DEV_CHANNEL_E channel, int new_vcom, int old_otp_times)
{
	// check id
	int val = jd_9367_check_id(channel);
	if (val != 0)
	{
		printf("chip_jd_9367_otp_check_vcom: check module id failed!\n");
		return -1;
	}

	#if 1
	// check otp times
	unsigned char vcom1_otp_time = 0;
	unsigned char vcom2_otp_times = 0;
	val = jd_9367_get_vcom_otp_times(channel, &vcom1_otp_time, &vcom2_otp_times);
	if (val != 0)
	{
		printf("chip_jd_9367_otp_check_vcom: get vcom otp times failed!\n");
		return -2;
	}

	if (vcom1_otp_time <= old_otp_times)
	{
		printf("chip_jd_9367_otp_check_vcom: vcom otp times is not change! last: %d. now: %d.\n",
				old_otp_times, vcom1_otp_time);
		return -3;
	}
	#endif
	
	// check vcom
	int index = 0;
	int vcom = 0;
	val = jd_9367_read_nvm_vcom(channel, index, &vcom);
	if (val != 0)
	{
		printf("chip_jd_9367_otp_check_vcom: read nvm vcom failed!\n");
		return -4;
	}

	if (vcom != new_vcom)
	{
		printf("chip_jd_9367_otp_check_vcom: nvm vcom is not change! read vcom: %d. burn vcom: %d.\n",
				vcom, new_vcom);
		return -5;
	}
	
	return 0;
}


#if 1

typedef struct tag_jd_9367_info
{
	unsigned char id[3];

	unsigned char id_ok;	// 0: id_error; 1: id_ok;
	
	unsigned int vcom1;
	unsigned int  vcom1_otp_times;
	
	unsigned int vcom2;
	unsigned int  vcom2_otp_times;
}jd_9367_info_t;

static jd_9367_info_t s_jd_9367_private_info[DEV_CHANNEL_NUM] = { 0 };
// read chip id

// check chip id

// read chip otp times

// read chip otp values


static int chip_jd_9367_get_chip_id(DEV_CHANNEL_E channel, unsigned char *p_id_data, int id_data_len)
{
	unsigned char reg[3] = { 0 };

	printf("chip_jd_9367_get_chip_id: channel = %d.\n", channel);
	qc_do_sleep_out(channel);
	reg[0] = qc_read_otp_data_reg(channel, 0x07);
	reg[1] = qc_read_otp_data_reg(channel, 0x08);
	reg[2] = qc_read_otp_data_reg(channel, 0x09);
	qc_do_sleep_in(channel);

	p_id_data[0] = reg[0];
	p_id_data[1] = reg[1];
	p_id_data[2] = reg[2];
	
	return -1;
}

static int chip_jd_9367_check_chip_ok(DEV_CHANNEL_E channel)
{	
	return 0;
}

static void chip_jd_9367_get_and_reset_private_data(DEV_CHANNEL_E channel)
{
	printf("chip_jd_9367_get_and_reset_private_data.\n");
	s_jd_9367_private_info[channel].id_ok = 0;
	memset(s_jd_9367_private_info[channel].id, 0, sizeof(s_jd_9367_private_info[channel].id));
	s_jd_9367_private_info[channel].vcom1 = 0;
	s_jd_9367_private_info[channel].vcom1_otp_times = 0;
	s_jd_9367_private_info[channel].vcom2 = 0;
	s_jd_9367_private_info[channel].vcom2_otp_times = 0;
}

static int chip_jd_9367_read_chip_vcom_opt_times(DEV_CHANNEL_E channel, int* p_otp_vcom_times)
{
	if (s_jd_9367_private_info[channel].id_ok == 1)
	{
		// read vcom otp times
		unsigned char vcom1_otp_times = 0;
		unsigned char vcom2_otp_times = 0;
		jd_9367_get_vcom_otp_times(channel, &vcom1_otp_times, &vcom2_otp_times);

		*p_otp_vcom_times = vcom1_otp_times;

		printf("chip_jd_9367_read_chip_vcom_opt_times: end\n");

		return 0;
	}
	
	printf("chip_jd_9367_read_chip_vcom_opt_times error!\n");
	return -1;
}


static int chip_jd_9367_read_chip_vcom_opt_info(DEV_CHANNEL_E channel, int* p_otp_vcom_times,
												int *p_otp_vcom_value)
{
	unsigned char vcom_h = 0;
	unsigned char vcom_l = 0;
	unsigned char vcom_otp_times = 0;
	
	qc_otp_data_read_vcom(channel, &vcom_h, &vcom_l, &vcom_otp_times);

	*p_otp_vcom_times = vcom_otp_times;
	*p_otp_vcom_value = (vcom_h << 8) | vcom_l;
	
	printf("chip_jd_9367_read_chip_vcom_opt_info: otp times = %d, vcom = %d, %#x.\n", vcom_otp_times, *p_otp_vcom_value);
	return -1;
}

static int chip_jd_9367_read_vcom(DEV_CHANNEL_E channel, int* p_vcom_value)
{
	if (s_jd_9367_private_info[channel].id_ok == 1)
	{
		int vcom = 0;
		jd_9367_read_vcom(channel, &vcom);
		*p_vcom_value = vcom;
		//printf("chip_ili_9806e_read_vcom: end\n");

		return 0;
	}
	printf("chip_jd_9367_read_vcom error: unknow chip!\n");
	return -1;
}


static int chip_jd_9367_write_vcom(DEV_CHANNEL_E channel, int vcom_value)
{
	//printf("chip_jd_9367_write_vcom: chip channel = %d, channel: %d. ...\n",
	//		channel, channel);
	
	if (s_jd_9367_private_info[channel].id_ok == 1)
	{
		if (s_jd_9367_private_info[channel].vcom1_otp_times == 0)
		{
			jd_9367_write_vcom(channel, vcom_value, 0);
		}
		else
		{
			jd_9367_write_vcom(channel, vcom_value, 1);
		}
		
		//printf("chip_jd_9367_read_vcom: end\n");

		return 0;
	}
	
	printf("chip_jd_9367_write_vcom error: unknow chip!\n");
	
	return -1;
}

static int chip_jd_9367_write_chip_vcom_otp_value(DEV_CHANNEL_E channel, int otp_vcom_value)
{

	if (s_jd_9367_private_info[channel].id_ok == 1)
	{
		int val = 0;
		val = jd_9367_burn_vcom(channel, otp_vcom_value);
		return val;
	}

	printf("chip_jd_9367_write_chip_vcom_otp_value error!\n");
	return -1;
}


static int chip_jd_9367_check_vcom_otp_burn_ok(DEV_CHANNEL_E channel, int vcom_value,
													int last_otp_times, int *p_read_vcom)
{
	// check id 
	int val = chip_jd_9367_check_chip_ok(channel);
	if (val != 0)
	{
		printf("chip_jd_9367_check_vcom_otp_burn_ok error: check id error, val = %d.\n", val);
		return E_ICM_READ_ID_ERROR;
	}

	if (p_read_vcom)
		*p_read_vcom = s_jd_9367_private_info[channel].vcom1;

	#if 1
	// check times
	if (s_jd_9367_private_info[channel].vcom1_otp_times != last_otp_times + 1)
	{
		printf("chip_jd_9367_check_vcom_otp_burn_ok error: check vcom otp times error. last times: %d, now: %d.\n", 
				last_otp_times, s_jd_9367_private_info[channel].vcom1_otp_times);

		return E_ICM_VCOM_TIMES_NOT_CHANGE;
	}
	#endif

	// check vcom value.
	if (s_jd_9367_private_info[channel].vcom1 != vcom_value)
	{
		printf("chip_jd_9367_check_vcom_otp_burn_ok error: check vcom value error. write: %d, read: %d.\n", 
				vcom_value, s_jd_9367_private_info[channel].vcom1);

		#ifdef ENABlE_OTP_BURN
		return E_ICM_VCOM_VALUE_NOT_CHANGE;
		#else
		return E_ICM_OK;
		#endif
	}

	return 0;
}
#endif

static int chip_jd_9367_get_chip_id_otp_info(DEV_CHANNEL_E channel, unsigned char *p_id_data,
											int* p_id_data_len, unsigned char* p_otp_times)
{
	if (p_id_data == NULL)
	{
		printf("chip_jd_9367_get_chip_id_otp_info error: Invalid param, p_id_data = NULL.\n");
		return -1;
	}
	
	if (p_id_data_len == NULL)
	{
		printf("chip_jd_9367_get_chip_id_otp_info error: Invalid param, id_data_len = NULL.\n");
		return -1;
	}

	if (*p_id_data_len < 3)
	{
		printf("chip_jd_9367_get_chip_id_otp_info error: Invalid param, id_data_len = %d.\n", *p_id_data_len);
		return -1;
	}

	if (p_otp_times == NULL)
	{
		printf("chip_jd_9367_get_chip_id_otp_info error: Invalid param, p_otp_times = NULL.\n");
		return -1;
	}

	unsigned char id1 = 0;
	unsigned char id2 = 0;
	unsigned char id3 = 0;
	unsigned char otp_times = 0;
	
	qc_otp_data_read_id(channel, &id1, &id2, &id3, &otp_times);

	p_id_data[0] = id1;
	p_id_data[1] = id2;
	p_id_data[2] = id3;
	*p_otp_times = otp_times;
	*p_id_data_len = 3;
	
	return 0;
}

static int chip_jd_9367_burn_chip_id(DEV_CHANNEL_E channel, unsigned char *p_id_data,
											int id_data_len, int ptn_id)
{
	qc_otp_id(channel, p_id_data[0], p_id_data[1], p_id_data[2], ptn_id);
	
	return 0;
}

static icm_chip_t chip_jd_9367 =
{
	.name = "jd9367",

	.reset_private_data = chip_jd_9367_get_and_reset_private_data,

	.id = chip_jd_9367_get_chip_id,
	.check_ok = chip_jd_9367_check_chip_ok,
	.read_vcom_otp_times = chip_jd_9367_read_chip_vcom_opt_times,
	.read_vcom_otp_info = chip_jd_9367_read_chip_vcom_opt_info,

	.read_vcom = chip_jd_9367_read_vcom,
	.write_vcom = chip_jd_9367_write_vcom,
	
	.write_vcom_otp_value = chip_jd_9367_write_chip_vcom_otp_value,
	.check_vcom_otp_burn_ok = chip_jd_9367_check_vcom_otp_burn_ok,

	.get_id_otp_info = chip_jd_9367_get_chip_id_otp_info,
	.burn_id = chip_jd_9367_burn_chip_id,
};

void jd9367Init()
{
	registerIcChip(&chip_jd_9367);
}

