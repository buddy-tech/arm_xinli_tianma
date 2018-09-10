#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "mipi2828.h"
#include "../ic_manager.h"

#define MAX_VCOM_OTP_TIMES		(3)

static void otm_8019a_ready(DEV_CHANNEL_E channel)
{
	unsigned char param = 0x00;
	mipi2828Write2IC(channel, 0x00, &param, sizeof(param));

	unsigned char params0[] = {0x80, 0x19, 0x01,};
	mipi2828Write2IC(channel, 0xff, &params0, sizeof(params0));

	param = 0x80;
	mipi2828Write2IC(channel, 0x00, &param, sizeof(param));

	unsigned char params1[] = {0x80, 0x19,};
	mipi2828Write2IC(channel, 0xff, &params1, sizeof(params1));
}

typedef struct tag_otm_8019a_info
{
	unsigned char id[5];
	unsigned char id_ok;	// 0: id_error; 1: id_ok;
	unsigned int vcom;
	unsigned int vcom_otp_times;
}otm_8019a_info_t;

static otm_8019a_info_t s_otm_8019a_private_info[DEV_CHANNEL_NUM] = { 0 };

static int otm_8019a_read_vcom(DEV_CHANNEL_E channel, int *p_vcom_value)
{
	unsigned char param = 0x00;
	mipi2828Write2IC(channel, 0x00, &param, sizeof(param));

	unsigned char data[8] = {0xff,0xff,0xff,0xff};
	int ret = ReadModReg(channel, 1, 0xD9, 1, data);                         //D9
	DBG("chip_otm_8019a_read_chip_vcom_opt_info: channel: %d, VCOM1|VCOM2 0xD9 = 0x%02X(%d)   ret=%d\n",
				channel, data[0], data[0], ret);
	if (ret <= 0)
	{
		DBG_ERR("otm_8019a_read_vcom error! ret = %d.\n", ret);
		return -1;
	}
	
	if (data[0] == 0xFF)
	{
		DBG_ERR("chip_otm_8019a_read_chip_vcom_opt_info error!\n");
		return -1;
	}

	if (p_vcom_value)
		*p_vcom_value = data[0];

	return 0;
}


static int otm_8019a_write_vcom(DEV_CHANNEL_E channel, int vcom_value)
{
	unsigned char param = 0x00;
	mipi2828Write2IC(channel, 0x00, &param, sizeof(param));

	param = vcom_value & 0xff;
	mipi2828Write2IC(channel, 0xD9, &param, sizeof(param));
	return 0;
}

static int chip_otm_8019a_get_chip_id(DEV_CHANNEL_E channel, unsigned char *p_id_data, int id_data_len)
{	
    unsigned char data[8] = {0,0,0,0,0,0,0,0};
	memset(data, 0, sizeof(data));
	int ret = ReadModReg(channel, 1, 0xA1, 5, data);
	
	printf("chip_otm_8019a_get_chip_id: channel: %d, Driver_ID 0xA1 = %02X %02X %02X %02X %02X   ret=%d\n",
				channel, data[0], data[1], data[2], data[3], data[4], ret);
	if( (data[0]== 0x01) && (data[1]==0x8B) && (data[2]==0x80) 
			&& (data[3]==0x19) && (data[4]==0xFF) )
	{
		if (p_id_data)
		{
			p_id_data[0] = data[0];
			p_id_data[1] = data[1];
			p_id_data[2] = data[2];
			p_id_data[3] = data[3];
			p_id_data[4] = data[4];
		}
		
		return 5;
	}

	printf("chip_otm_8019a_get_chip_id: read id failed!\n");
	return -1;
}


static int chip_otm_8019a_read_chip_vcom_opt_times(DEV_CHANNEL_E channel, int* p_otp_vcom_times)
{
	if (s_otm_8019a_private_info[channel].id_ok == 1)
	{
		otm_8019a_ready(channel);
		
		// read vcom otp times
		unsigned char param = 0x02;
		mipi2828Write2IC(channel, 0x00, &param, sizeof(param));

		unsigned char data[8] = {0xff,0xff,0xff,0xff};
		int ret = ReadModReg(channel, 1, 0xF1, 1, data);

		short opt_count = 0;
		if(data[0] & 0x80) opt_count += 1;
		if(data[0] & 0x40) opt_count += 1;
		if(data[0] & 0x20) opt_count += 1;
		if(data[0] & 0x10) opt_count += 1;
		printf("chip_otm_8019a_read_chip_vcom_opt_times: channel: %d, OTP-VCOM-TIMES 0xF1 = 0x%X (%d)   ret=%d\n",
					channel, data[0], opt_count, ret);
		
		*p_otp_vcom_times = opt_count;
		return 0;
	}
	
	printf("chip_otm_8019a_read_chip_vcom_opt_times error!\n");
	return -1;
}


static int chip_otm_8019a_check_chip_ok(DEV_CHANNEL_E channel)
{
	// read and check id;
	int val = chip_otm_8019a_get_chip_id(channel, s_otm_8019a_private_info[channel].id,
								sizeof(s_otm_8019a_private_info[channel].id));

	if ( val < 0)
	{
		printf("chip_otm_8019a_check_chip_ok error: read chip id failed!\n");		
		s_otm_8019a_private_info[channel].id_ok = 0;

		return -1;
	}

	s_otm_8019a_private_info[channel].id_ok = 1;

	// read vcom otp times
	int vcom_otp_times = 0;
	chip_otm_8019a_read_chip_vcom_opt_times(channel, &vcom_otp_times);
	s_otm_8019a_private_info[channel].vcom_otp_times = vcom_otp_times;

	// read vcom otp value
	unsigned int vcom_otp_value = 0;
	otm_8019a_read_vcom(channel, &vcom_otp_value);
	s_otm_8019a_private_info[channel].vcom = vcom_otp_value;

	printf("chip_otm_8019a_check_chip_ok:chip:%d, mipi: %d. vcom_otp_times: %d, "
			"vcom_otp_value: %d. \n", channel, channel,
				s_otm_8019a_private_info[channel].vcom_otp_times,
				s_otm_8019a_private_info[channel].vcom);
	
	return 0;
}

static void chip_otm_8019a_get_and_reset_private_data(DEV_CHANNEL_E channel)
{
	printf("chip_otm_8019a_get_and_reset_private_data.\n");
	s_otm_8019a_private_info[channel].id_ok = 0;
	memset(s_otm_8019a_private_info[channel].id, 0, sizeof(s_otm_8019a_private_info[channel].id));
	s_otm_8019a_private_info[channel].vcom = 0;
	s_otm_8019a_private_info[channel].vcom_otp_times = 0;
}


static int chip_otm_8019a_read_chip_vcom_opt_info(DEV_CHANNEL_E channel, int* p_otp_vcom_times,
												int *p_otp_vcom_value)
{
	if (s_otm_8019a_private_info[channel].id_ok == 1)
	{
		// read vcom otp times
		int vcom_otp_times = 0;
		//ili_9806_get_vcom_otp_times(channel, &vcom1_otp_times, &vcom2_otp_times);
		chip_otm_8019a_read_chip_vcom_opt_times(channel, &vcom_otp_times);
		

		// read vcom otp value
		int vcom_otp_value = 0;
		int val = otm_8019a_read_vcom(channel, &vcom_otp_value);
		if (val != 0)
		{
			printf("chip_otm_8019a_read_chip_vcom_opt_info error!\n");
			return -1;
		}

		*p_otp_vcom_times = vcom_otp_times;
		*p_otp_vcom_value = vcom_otp_value;

		printf("chip_otm_8019a_read_chip_vcom_opt_info: end\n");

		return 0;
	}
	
	printf("chip_otm_8019a_read_chip_vcom_opt_info error: unknow chip!\n");
	return -1;
}

static int chip_otm_8019a_read_vcom(DEV_CHANNEL_E channel, int* p_vcom_value)
{
	if (s_otm_8019a_private_info[channel].id_ok == 1)
	{
		int vcom = 0;

		otm_8019a_ready(channel);
		if (otm_8019a_read_vcom(channel, &vcom) != 0)
		{
		}
		*p_vcom_value = vcom;
		printf("chip_otm_8019a_read_vcom: end\n");

		return 0;
	}
	printf("chip_otm_8019a_read_vcom error: unknow chip!\n");
	return -1;
}


static int chip_otm_8019a_write_vcom(DEV_CHANNEL_E channel, int vcom_value)
{
	//printf("chip_otm_8019a_write_vcom: chip channel = %d, channel: %d. ...\n",
	//		channel, channel);
	
	if (s_otm_8019a_private_info[channel].id_ok == 1)
	{
		//otm_8019a_ready(channel);
		otm_8019a_write_vcom(channel, vcom_value);

		#if 0
		int temp_vcom = 0;
		otm_read_vcom(channel, &temp_vcom);
		
		printf("chip_otm_8019a_write_vcom:  read vcom = %d. end\n", temp_vcom);
		#endif
		
		return 0;
	}
	
	printf("chip_otm_8019a_write_vcom error: unknow chip!\n");
	
	return -1;
}


static void otm_8019a_unlock(DEV_CHANNEL_E channel)
{
	unsigned char param = 0x00;
	mipi2828Write2IC(channel, 0x00, &param, sizeof(param));

	unsigned char params0[] = {0x80, 0x19, 0x01,};
	mipi2828Write2IC(channel, 0xff, &params0, sizeof(params0));

	param = 0x80;
	mipi2828Write2IC(channel, 0x00, &param, sizeof(param));

	unsigned char params1[] = {0x80, 0x19,};
	mipi2828Write2IC(channel, 0xff, &params1, sizeof(params1));
}

static void otm_8019a_sleep_out(DEV_CHANNEL_E channel)
{
	mipi2828Write2IC(channel, 0x11, NULL, 0);
}

static void otm_8019a_display_on(DEV_CHANNEL_E channel)
{
	mipi2828Write2IC(channel, 0x29, NULL, 0);
}


static void otm_8019a_display_off(DEV_CHANNEL_E channel)
{
	mipi2828Write2IC(channel, 0x28, NULL, 0);
}


static void otm_8019a_sleep_in(DEV_CHANNEL_E channel)
{
	mipi2828Write2IC(channel, 0x10, NULL, 0);
}

static void otm_8019a_program_on(DEV_CHANNEL_E channel)
{
	unsigned char param = 0x00;
	mipi2828Write2IC(channel, 0x00, &param, sizeof(param));

	param = 0x81;
	mipi2828Write2IC(channel, 0xEB, &param, sizeof(param));
}

static void otm_8019a_program_end(DEV_CHANNEL_E channel)
{
	unsigned char param = 0x00;
	mipi2828Write2IC(channel, 0x00, &param, sizeof(param));

	param = 0x00;
	mipi2828Write2IC(channel, 0xEB, &param, sizeof(param));
}

static int chip_otm_8019a_write_chip_vcom_otp_value(DEV_CHANNEL_E channel, int otp_vcom_value)
{

	if (s_otm_8019a_private_info[channel].id_ok == 1)
	{
		int val = 0;

		setMipi2828HighSpeedMode(channel, false);
		
		printf("mipi reset.\n");
		mipiIcReset(channel);
		
		usleep(1000 * 5);

		// unlock
		printf("unlock \n");
		otm_8019a_unlock(channel);

		// sleep out
		printf("sleep out \n");
		otm_8019a_sleep_out(channel);

		usleep(150 * 1000);

		// write vcom
		printf("write vcom \n");
		otm_8019a_write_vcom(channel, otp_vcom_value);

		// check vcom value.
		int read_vcom = 0;
		printf("read vcom\n");
		otm_8019a_read_vcom(channel, &read_vcom);
		if (read_vcom != otp_vcom_value)
		{
			printf("write vcom check error: write: %d, read: %d. \n", otp_vcom_value, read_vcom);
			val = -1;
		}

		usleep(1000 * 10);

		// display off
		otm_8019a_display_off(channel);
		usleep(100 * 1000);

		// sleep in.
		printf("sleep in\n");
		otm_8019a_sleep_in(channel);

		usleep(800 * 1000); 
		
		// mtp power on
		printf("mtp power on.\n");
		mtp_power_on(channel);
		usleep(800 * 1000);

		// program on
		printf("program ...\n");
		otm_8019a_program_on(channel);
		
		usleep(1200 * 1000);

		// program end.
		printf("program end.\n");
		otm_8019a_program_end(channel);

		usleep(100*1000);

		// mtp power off
		printf("mtp power off.\n");
		mtp_power_off(channel);

		usleep(100 * 1000);

		printf("mipi reset.\n");
		mipiIcReset(channel);

		usleep(100 * 1000);
		
		// sleep out
		printf("sleep out.\n");
		otm_8019a_sleep_out(channel);
		
		usleep(150 * 1000);
		
		return val;
	}

	printf("chip_otm_8019a_write_chip_vcom_otp_value error!\n");
	return -1;
}


static int chip_otm_8019a_check_vcom_otp_burn_ok(DEV_CHANNEL_E channel, int vcom_value,
													int last_otp_times, int *p_read_vcom)
{
	printf(" === chip_otm_8019a_check_vcom_otp_burn_ok  ... === ");

	// sleep out
	otm_8019a_sleep_out(channel);

	// unlock
	otm_8019a_unlock(channel);

	// read id
	if (0 != chip_otm_8019a_check_chip_ok(channel))
	{
		printf("chip_otm_8019a_check_vcom_otp_burn_ok: read chip id error!\n");
		return E_ICM_READ_ID_ERROR;
	}

	if (p_read_vcom)
		*p_read_vcom = vcom_value;

	if (s_otm_8019a_private_info[channel].vcom_otp_times <= last_otp_times)
	{
		printf("chip_otm_8019a_check_vcom_otp_burn_ok: vcom otp times not change:!\n");
		return E_ICM_VCOM_TIMES_NOT_CHANGE;
	}

	if (s_otm_8019a_private_info[channel].vcom != vcom_value)
	{
		printf("chip_otm_8019a_check_vcom_otp_burn_ok: vcom otp value not change:!\n");
		return E_ICM_VCOM_VALUE_NOT_CHANGE;
	}
	
	return E_ICM_OK;
}

static icm_chip_t chip_otm_8019a =
{
	.name = "otm8019a",

	.reset_private_data = chip_otm_8019a_get_and_reset_private_data,

	.id = chip_otm_8019a_get_chip_id,
	.check_ok = chip_otm_8019a_check_chip_ok,
	.read_vcom_otp_times = chip_otm_8019a_read_chip_vcom_opt_times,
	.read_vcom_otp_info = chip_otm_8019a_read_chip_vcom_opt_info,

	.read_vcom = chip_otm_8019a_read_vcom,
	.write_vcom = chip_otm_8019a_write_vcom,
	
	.write_vcom_otp_value = chip_otm_8019a_write_chip_vcom_otp_value,
	.check_vcom_otp_burn_ok = chip_otm_8019a_check_vcom_otp_burn_ok,
};

void otm8019aInit()
{
	registerIcChip(&chip_otm_8019a);
}

