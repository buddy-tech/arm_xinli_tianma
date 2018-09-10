#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ic_manager.h"
#include "hi_unf_i2c.h"
#include "hi_unf_gpio.h"
#include "common.h"

static icChipMng_t* pSgIcChipMng = NULL;

void registerIcChip(icm_chip_t* pChip)
{
	if (NULL == pChip)
		return;

	icm_chip_t* pIdleChip = NULL;
	int count = 0;

	do
	{
		pIdleChip = pSgIcChipMng->chips[count];
		if (NULL == pIdleChip)
		{
			pSgIcChipMng->chips[count] = pChip;
			break;
		}

		count++;

	}while (count < IC_CHIP_MAX_NUM);
}

void icMgrSetInitCodeName(char* pName)
{
	strcpy(pSgIcChipMng->initCodeName, pName);
}

static icm_chip_t* currentIcmChipAndCheckChannel(unsigned int channel);

void icMgrSndInitCode(DEV_CHANNEL_E channel)
{
	icm_chip_t* pChip = currentIcmChipAndCheckChannel(channel);
	if (pChip)
	{
		if (pChip->snd_init_code)
			pChip->snd_init_code(channel, pSgIcChipMng->initCodeName);
	}
}

int ic_mgr_set_current_chip_id(char* str_chip_id)
{
	printf("%s\r\n", __FUNCTION__);
	pSgIcChipMng->index = 0;

	int i = 0;
	for (; i < IC_CHIP_MAX_NUM; i++)
	{
		icm_chip_t* pChip = pSgIcChipMng->chips[i];
		if (NULL == pChip)
			continue;

		if (strcmp(str_chip_id, pChip->name) == 0)
		{
			pSgIcChipMng->index = i;
			printf("*** chip: %s ***\n", pChip->name);
			break;
		}
	}

	if (i >= IC_CHIP_MAX_NUM)
		printf("unknown chip: %s\r\n", str_chip_id);

	return 0;
}

static icm_chip_t* currentIcmChipAndCheckChannel(DEV_CHANNEL_E channel)
{
	icm_chip_t* pChip = NULL;

	do
	{
		if (channel < 0 && channel > 3)
		{
			printf("channel<%d> is not in 0-3\n", channel);
			break;
		}

		pChip = pSgIcChipMng->chips[pSgIcChipMng->index];
	}while (0);

	return pChip;
}

int ic_mgr_set_channel_state(DEV_CHANNEL_E channel, unsigned int state)
{
	icm_chip_t* pChip = currentIcmChipAndCheckChannel(channel);
	if (pChip)
	{
		if (pChip->reset_private_data)
			pChip->reset_private_data(channel);
	}

	pSgIcChipMng->state[channel] = state ? CHANNEL_BUSY : CHANNEL_IDLE;
	return 0;
}

int ic_mgr_init()
{
	pSgIcChipMng = (icChipMng_t *)calloc(1, sizeof(icChipMng_t));

	/* default chip */
	defaultChipInit();
	/* hi8394 */
	hi8394Init();
	/* icn9706 */
	icn9706Init();
	/* ili9806e */
	ili9806eInit();
	/* jd9367 */
	jd9367Init();
	/* otm8019a */
	otm8019aInit();
//	/* s1d19k08 */
//	s1d19k08ChipInit();
	/* hx8298c */
	hx8298cChipInit();
	return 0;
}

int ic_mgr_term()
{
	free(pSgIcChipMng);
	return 0;
}

int ic_mgr_read_chip_id(DEV_CHANNEL_E channel, unsigned char* p_id_data, int id_data_len)
{
	icm_chip_t* pChip = currentIcmChipAndCheckChannel(channel);
	if (NULL == pChip)
		return -1;

	int val = -1;
	if (pChip->id)
		val = pChip->id(channel,p_id_data, id_data_len);

	return val;
}

int ic_mgr_check_chip_ok(DEV_CHANNEL_E channel)
{
	icm_chip_t* pChip = currentIcmChipAndCheckChannel(channel);
	if (NULL == pChip)
		return -1;

	int val = -1;

	if (pChip->check_ok)
		val = pChip->check_ok(channel);

	return val;
}

// vcom
int ic_mgr_read_chip_vcom_otp_times(DEV_CHANNEL_E channel, int* p_otp_vcom_times)
{
	icm_chip_t* pChip = currentIcmChipAndCheckChannel(channel);
	if (NULL == pChip)
		return -1;

	int val = -1;

	if (pChip->read_vcom_otp_times)
		val = pChip->read_vcom_otp_times(channel, p_otp_vcom_times);

	return val;
}

int ic_mgr_read_chip_vcom_otp_value(DEV_CHANNEL_E channel, int* p_otp_vcom_times, int* p_otp_vcom_value)
{
	icm_chip_t* pChip = currentIcmChipAndCheckChannel(channel);
	if (NULL == pChip)
		return -1;

	int val = -1;
	if (pChip->read_vcom_otp_info)
		val = pChip->read_vcom_otp_info(channel, p_otp_vcom_times, p_otp_vcom_value);
	
	return val;
}

int ic_mgr_write_chip_vcom_otp_value(DEV_CHANNEL_E channel, int otp_vcom_value)
{
	icm_chip_t* pChip = currentIcmChipAndCheckChannel(channel);
	if (NULL == pChip)
		return -1;

	int val = -1;

	if (pChip->write_vcom_otp_value)
		val = pChip->write_vcom_otp_value(channel, otp_vcom_value);
	
	return val;
}

int ic_mgr_read_vcom(DEV_CHANNEL_E channel, int* p_vcom_value)
{
	icm_chip_t* pChip = currentIcmChipAndCheckChannel(channel);
	if (NULL == pChip)
		return -1;

	int val = -1;
	if (pChip->read_vcom)
		val = pChip->read_vcom(channel, p_vcom_value);

	return val;
}

int ic_mgr_write_vcom(DEV_CHANNEL_E channel, int vcom_value)
{
	icm_chip_t* pChip = currentIcmChipAndCheckChannel(channel);
	if (NULL == pChip)
		return -1;

	int val = -1;

	if (pChip->write_vcom)
		val = pChip->write_vcom(channel, vcom_value);
	
	return val;
}

int ic_mgr_check_vcom_otp_burn_ok(DEV_CHANNEL_E channel, int vcom_value, int last_otp_times, int *p_read_vcom)
{
	icm_chip_t* pChip = currentIcmChipAndCheckChannel(channel);
	if (NULL == pChip)
		return -1;

	int val = -1;
	if (pChip->check_vcom_otp_burn_ok)
		val = pChip->check_vcom_otp_burn_ok(channel, vcom_value, last_otp_times,p_read_vcom);

	
	return val;
}

// gamma
int ic_mgr_write_analog_gamma_data(DEV_CHANNEL_E channel, unsigned char *p_analog_gamma_reg_data,
									int analog_gamma_reg_data_len)
{
	icm_chip_t* pChip = currentIcmChipAndCheckChannel(channel);
	if (NULL == pChip)
		return -1;

	int val = -1;
	if (pChip->write_analog_gamma_reg_data)
		val = pChip->write_analog_gamma_reg_data(channel, p_analog_gamma_reg_data, analog_gamma_reg_data_len);
	return val;
}
									
int ic_mgr_read_analog_gamma_data(DEV_CHANNEL_E channel, unsigned char *p_analog_gamma_reg_data,
									int analog_gamma_reg_data_len)
{
	icm_chip_t* pChip = currentIcmChipAndCheckChannel(channel);
	if (NULL == pChip)
		return -1;

	int val = -1;
	if (pChip->read_analog_gamma_reg_data)
		val = pChip->read_analog_gamma_reg_data(channel,p_analog_gamma_reg_data, analog_gamma_reg_data_len);
	
	return val;
}

int ic_mgr_write_fix_analog_gamma_data(DEV_CHANNEL_E channel)
{
	icm_chip_t* pChip = currentIcmChipAndCheckChannel(channel);
	if (NULL == pChip)
		return -1;

	int val = -1;
	if (pChip->write_fix_analog_gamma_reg_data)
		val = pChip->write_fix_analog_gamma_reg_data(channel);

	return val;
}

int ic_mgr_write_digital_gamma_data(DEV_CHANNEL_E channel, unsigned char *p_digital_gamma_reg_data,
									int digital_gamma_reg_data_len)
{
	icm_chip_t* pChip = currentIcmChipAndCheckChannel(channel);
	if (NULL == pChip)
		return -1;

	int val = -1;
	if (pChip->write_digital_gamma_reg_data)
		val = pChip->write_digital_gamma_reg_data(channel,p_digital_gamma_reg_data, digital_gamma_reg_data_len);
	
	return val;
}
									
int ic_mgr_write_fix_digital_gamma_data(DEV_CHANNEL_E channel)
{
	icm_chip_t* pChip = currentIcmChipAndCheckChannel(channel);
	if (NULL == pChip)
		return -1;

	int val = -1;
	if (pChip->write_fix_digital_gamma_reg_data)
		val = pChip->write_fix_digital_gamma_reg_data(channel);
	
	return val;
}

int ic_mgr_d3g_control(DEV_CHANNEL_E channel, int enable)
{
	icm_chip_t* pChip = currentIcmChipAndCheckChannel(channel);
	if (NULL == pChip)
		return -1;

	int val = -1;
	if (pChip->d3g_control)
		val = pChip->d3g_control(channel, enable);

	return val;
}

int ic_mgr_burn_gamma_otp_values(DEV_CHANNEL_E channel, int burn_flag, int enable_burn_vcom, int vcom,
									unsigned char *p_analog_gamma_reg_data, int analog_gamma_reg_data_len,
									unsigned char *p_d3g_r_reg_data, int d3g_r_reg_data_len,
									unsigned char *p_d3g_g_reg_data, int deg_g_reg_data_len,
									unsigned char *p_d3g_b_reg_data, int deg_b_reg_data_len)
{
	icm_chip_t* pChip = currentIcmChipAndCheckChannel(channel);
	if (NULL == pChip)
		return -1;

	int val = -1;
	if (pChip->burn_gamma_otp_values)
		val = pChip->burn_gamma_otp_values(channel, burn_flag,enable_burn_vcom, vcom,
											p_analog_gamma_reg_data, analog_gamma_reg_data_len,
											p_d3g_r_reg_data, d3g_r_reg_data_len,
											p_d3g_g_reg_data, deg_g_reg_data_len,
											p_d3g_b_reg_data, deg_b_reg_data_len);

	return val;
}

int ic_mgr_check_gamma_otp_values(DEV_CHANNEL_E channel, int enable_burn_vcom, int new_vcom_value, int last_vcom_otp_times,
									unsigned char *p_analog_gamma_reg_data, int analog_gamma_reg_data_len,
									unsigned char *p_d3g_r_reg_data, int d3g_r_reg_data_len,
									unsigned char *p_d3g_g_reg_data, int deg_g_reg_data_len,
									unsigned char *p_d3g_b_reg_data, int deg_b_reg_data_len)
{
	icm_chip_t* pChip = currentIcmChipAndCheckChannel(channel);
	if (NULL == pChip)
		return -1;

	int val = -1;
	if (pChip->check_gamma_otp_values)
		val = pChip->check_gamma_otp_values(channel, enable_burn_vcom, new_vcom_value, last_vcom_otp_times,
											p_analog_gamma_reg_data, analog_gamma_reg_data_len,
											p_d3g_r_reg_data, d3g_r_reg_data_len,
											p_d3g_g_reg_data, deg_g_reg_data_len,
											p_d3g_b_reg_data, deg_b_reg_data_len);
	return val;
}

int ic_mgr_get_analog_gamma_reg_data(DEV_CHANNEL_E channel, unsigned char *p_analog_gamma_reg_data,
										int *p_analog_gamma_reg_data_len)
{
	icm_chip_t* pChip = currentIcmChipAndCheckChannel(channel);
	if (NULL == pChip)
		return -1;

	int val = -1;
	if (pChip->get_analog_gamma_reg_data)
		val = pChip->get_analog_gamma_reg_data(channel, p_analog_gamma_reg_data, p_analog_gamma_reg_data_len);

	return val;
}

int ic_mgr_get_digital_gamma_reg_data_t(DEV_CHANNEL_E channel, unsigned char *p_d3g_r_reg_data, int *p_d3g_r_reg_data_len,
									unsigned char *p_d3g_g_reg_data, int *p_d3g_g_reg_data_len,
									unsigned char *p_d3g_b_reg_data, int *p_d3g_b_reg_data_len)
{
	icm_chip_t* pChip = currentIcmChipAndCheckChannel(channel);
	if (NULL == pChip)
		return -1;

	int val = -1;
	if (pChip->get_digital_gamma_reg_data)
		val = pChip->get_digital_gamma_reg_data(channel, p_d3g_r_reg_data, p_d3g_r_reg_data_len,
																p_d3g_g_reg_data, p_d3g_g_reg_data_len,
																p_d3g_b_reg_data, p_d3g_b_reg_data_len);
	
	return val;
}

int ic_mgr_read_chip_id_otp_info(DEV_CHANNEL_E channel, unsigned char* p_id_data, int *p_id_data_len, unsigned char* p_otp_times)
{
	icm_chip_t* pChip = currentIcmChipAndCheckChannel(channel);
	if (NULL == pChip)
		return -1;

	int val = -1;
	if (pChip->get_id_otp_info)
		val = pChip->get_id_otp_info(channel, p_id_data, p_id_data_len, p_otp_times);

	return val;
}

int ic_mgr_burn_chip_id(DEV_CHANNEL_E channel, unsigned char* p_id_data, int id_data_len, int ptn_id)
{
	icm_chip_t* pChip = currentIcmChipAndCheckChannel(channel);
	if (NULL == pChip)
		return -1;

	int val = -1;
	if (pChip->get_id_otp_info)
		val = pChip->get_id_otp_info(channel, p_id_data, id_data_len, ptn_id);

	return val;
}

static void setIcWriteState(DEV_CHANNEL_E channel, int enable)
{
	if (channel == 0 || channel == 1)
	{
		if (enable)
			gpio_set_output_value(TTL_GPIO_PIN_UD_L, 0);
		else
			gpio_set_output_value(TTL_GPIO_PIN_UD_L, 1);
	}
	else
	{
		if (enable)
			gpio_set_output_value(TTL_GPIO_PIN_UD_R, 0);
		else
			gpio_set_output_value(TTL_GPIO_PIN_UD_R, 1);
	}
}

void burnVcom(DEV_CHANNEL_E channel, unsigned char value)
{
	/* enable IC register write */
	setIcWriteState(channel, 1);

	/* select IC register page 1 */
	vi2c_write(channel, 0x52, 0x0, 0x1);

	/* Enable IC vcom */
	unsigned char tmpValue = 0;
	vi2c_read(channel, 0x52, 0x10, &tmpValue);

	tmpValue |= (0x1 << 4);
	vi2c_write(channel, 0x52, 0x10, tmpValue);

	/* set vcom register value of the IC */
	vi2c_write(channel, 0x52, 0x16, value);

	/* select IC register page 9 */
	vi2c_write(channel, 0x52, 0x00, 0x09);

	/* Enable the OTP program password */
	vi2c_write(channel, 0x52, 0x02, 0x66);

	/* Set Vcom group number */
	vi2c_write(channel, 0x52, 0x01, 0x08);

	/* Set OTP write bit */
	vi2c_write(channel, 0x52, 0x03, 0x01);

	/* sleep 12 ms */
	usleep(50*1000);

	/* Read OTP_TRIM_FLAG */
	vi2c_read(channel, 0x52, 0x0F, &tmpValue);
	if (0 != tmpValue)
		printf("Error: vcom burn error<0x%x>\r\n", tmpValue);

	vi2c_write(channel, 0x52, 0x02, 0);

	/* disable IC register write */
	setIcWriteState(channel, 0);
}

unsigned char readVcom(DEV_CHANNEL_E channel)
{
	unsigned char value = 0;

	/* enable IC register write */
	setIcWriteState(channel, 1);

	/* select IC register page 1 */
	vi2c_write(channel, 0x52, 0x0, 0x1);

	/* read vcom from IC */
	vi2c_read(channel, 0x52, 0x16, &value);

	/* disable IC register write */
	setIcWriteState(channel, 0);

	return value;
}

void writeVcom(DEV_CHANNEL_E channel, unsigned char value)
{
	/* enable IC register write */
	setIcWriteState(channel, 1);

	/* select IC register page 1 */
	vi2c_write(channel, 0x52, 0x0, 0x1);

	/* Enable IC vcom */
	unsigned char tmpValue = 0;
	vi2c_read(channel, 0x52, 0x10, &tmpValue);

	tmpValue |= (0x1 << 4);
	vi2c_write(channel, 0x52, 0x10, tmpValue);

	/* write vcom into IC */
	vi2c_write(channel, 0x52, 0x16, value);
}

unsigned int readVcomOtpTimes(DEV_CHANNEL_E channel)
{
	unsigned short value = 0;
	/* select IC register page 1 */
	vi2c_write(channel, 0x52, 0x0, 0x1);

	/* read vcom otp times high bits from IC */
	unsigned short highBits = 0;
	vi2c_read(channel, 0x52, 0x1E, &highBits);

	/* read vcom otp times low bits from IC */
	unsigned short lowBits = 0;
	vi2c_read(channel, 0x52, 0x1D, &lowBits);

	value = (highBits & 0x3) << 8 | lowBits;

	int count = 0;
	while (value)
	{
		count++;
		value = value >> 1;
	}

	return count;
}
