#include <stdio.h>
#include "../ic_manager.h"

static int default_chip_get_chip_id(DEV_CHANNEL_E channel, unsigned char *p_id_data, int id_data_len)
{
	printf("default_chip_get_chip_id error: unknow chip!\n");
	return -1;
}


static int default_chip_check_chip_ok(DEV_CHANNEL_E channel)
{
	printf("default_chip_check_chip_ok error: unknow chip!\n");
	return -1;
}

static int default_chip_read_chip_vcom_opt_times(DEV_CHANNEL_E channel, int* p_otp_vcom_times)
{
	printf("default_chip_read_chip_vcom_opt_times error: unknow chip!\n");
	return -1;
}


static int default_chip_read_chip_vcom_opt_info(DEV_CHANNEL_E channel, int* p_otp_vcom_times,
												int *p_otp_vcom_value)
{
	printf("default_chip_read_chip_vcom_opt_info error: unknow chip!\n");
	return -1;
}


static int default_chip_write_chip_vcom_otp_value(DEV_CHANNEL_E channel, int otp_vcom_value)
{
	printf("default_chip_write_chip_vcom_otp_value error: unknow chip!\n");
	return -1;
}


static int default_chip_read_vcom(DEV_CHANNEL_E channel, int* p_vcom_value)
{
	printf("default_chip_read_vcom error: unknow chip!\n");
	return -1;
}


static int default_chip_write_vcom(DEV_CHANNEL_E channel, int vcom_value)
{
	printf("default_chip_write_vcom error: unknow chip!\n");
	return -1;
}

static void default_chip_get_and_reset_private_data(DEV_CHANNEL_E channel)
{
	printf("default_chip_get_and_reset_private_data error: unknow chip!\n");
}


static int default_chip_check_vcom_otp_burn_ok(DEV_CHANNEL_E channel, int vcom_value,
													int last_otp_times, int *p_read_vcom)
{
	printf("default_chip_check_vcom_otp_burn_ok error: unknow chip!\n");
	return -1;
}

// gamma
static int default_chip_write_analog_gamma_reg_data(DEV_CHANNEL_E channel, unsigned char *p_analog_gamma_reg_data, int analog_gamma_reg_len)
{
	printf("default_chip_write_analog_gamma_reg_data error: unknow chip!\n");
	return -1;
}

static int default_chip_read_analog_gamma_reg_data(DEV_CHANNEL_E channel, unsigned char *p_analog_gamma_reg_data_buffer, int analog_gamma_reg_len)
{
	printf("default_chip_read_analog_gamma_reg_data error: unknow chip!\n");
	return -1;
}

static int default_chip_write_fix_analog_gamma_reg_data(DEV_CHANNEL_E channel)
{
	printf("default_chip_write_fix_analog_gamma_reg_data error: unknow chip!\n");
	return -1;
}

static int default_chip_write_digital_gamma_reg_data(DEV_CHANNEL_E channel, int ital_gamma_reg_len)
{
	printf("default_chip_write_digital_gamma_reg_data error: unknow chip!\n");
	return -1;
}
				
static int default_chip_write_fix_digital_gamma_reg_data(DEV_CHANNEL_E channel)
{
	printf("default_chip_write_fix_digital_gamma_reg_data error: unknow chip!\n");
	return -1;
}

static int default_chip_write_d3g_control(DEV_CHANNEL_E channel, int enable)
{
	printf("default_chip_write_d3g_control error: unknow chip!\n");
	return -1;
}


static int default_chip_burn_gamma_otp_values(DEV_CHANNEL_E channel, int burn_flag,
											int enable_burn_vcom, int vcom, 
											unsigned char *p_analog_gamma_reg_data, int analog_gamma_reg_data_len,
											unsigned char *p_d3g_r_reg_data, int d3g_r_reg_data_len,
											unsigned char *p_d3g_g_reg_data, int deg_g_reg_data_len,
											unsigned char *p_d3g_b_reg_data, int deg_b_reg_data_len)
{
	printf("default_chip_burn_gamma_otp_values error: unknow chip!\n");
	return -1;
}

static int default_chip_check_gamma_otp_values(DEV_CHANNEL_E channel, int enable_burn_vcom, int new_vcom_value, int last_vcom_otp_times,
											unsigned char *p_analog_gamma_reg_data, int analog_gamma_reg_data_len,
											unsigned char *p_d3g_r_reg_data, int d3g_r_reg_data_len,
											unsigned char *p_d3g_g_reg_data, int deg_g_reg_data_len,
											unsigned char *p_d3g_b_reg_data, int deg_b_reg_data_len)
{
	printf("default_chip_check_gamma_otp_values error: unknow chip!\n");
	return -1;
}

static int default_chip_get_analog_gamma_reg_data(DEV_CHANNEL_E channel, unsigned char *p_analog_gamma_reg_data, int *p_analog_gamma_reg_data_len)
{
	printf("default_chip_get_analog_gamma_reg_data error: unknow chip!\n");
	return -1;
}

static int default_chip_get_digital_gamma_reg_data(DEV_CHANNEL_E channel, unsigned char *p_d3g_r_reg_data, int *p_d3g_r_reg_data_len,
											unsigned char *p_d3g_g_reg_data, int *p_d3g_g_reg_data_len,
											unsigned char *p_d3g_b_reg_data, int *p_d3g_b_reg_data_len)
{
	printf("default_chip_get_digital_gamma_reg_data error: unknow chip!\n");
	return -1;
}

static icm_chip_t default_chip =
{
	.name = "default",

	.reset_private_data = default_chip_get_and_reset_private_data,
	
	.id = default_chip_get_chip_id,
	.check_ok = default_chip_check_chip_ok,
	.read_vcom_otp_times = default_chip_read_chip_vcom_opt_times,
	.read_vcom_otp_info = default_chip_read_chip_vcom_opt_info,
	
	.read_vcom = default_chip_read_vcom,
	.write_vcom = default_chip_write_vcom,
	
	.write_vcom_otp_value = default_chip_write_chip_vcom_otp_value,
	.check_vcom_otp_burn_ok = default_chip_check_vcom_otp_burn_ok,

	// gamma
	.d3g_control = default_chip_write_d3g_control,
	.write_analog_gamma_reg_data = default_chip_write_analog_gamma_reg_data,
	.read_analog_gamma_reg_data = default_chip_read_analog_gamma_reg_data,
	.write_fix_analog_gamma_reg_data = default_chip_write_fix_analog_gamma_reg_data,
	
	.write_digital_gamma_reg_data = default_chip_write_digital_gamma_reg_data,
	.write_fix_digital_gamma_reg_data = default_chip_write_fix_digital_gamma_reg_data,

	.burn_gamma_otp_values = default_chip_burn_gamma_otp_values,
	.check_gamma_otp_values = default_chip_check_gamma_otp_values,
	
	.get_analog_gamma_reg_data = default_chip_get_analog_gamma_reg_data,
	.get_digital_gamma_reg_data = default_chip_get_digital_gamma_reg_data,
};

void defaultChipInit()
{
	registerIcChip(&default_chip);
}

