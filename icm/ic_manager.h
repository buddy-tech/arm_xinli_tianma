#ifndef _IC_MANAGER_H_
#define _IC_MANAGER_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INITCODE_PATH "cfg/initcode/"

#define INIT_CODE_MAX_LEN	(100*1024)
/* default chip */
void defaultChipInit();

/* hi8394 */
void hi8394Init();

/* icn9706 */
void icn9706Init();

/* ili9806e */
void ili9806eInit();

/* jd9367 */
void jd9367Init();

/* otm8019a */
void otm8019aInit();

/* s1d19k08 */
void s1d19k08ChipInit();

/* hx8298c */
void hx8298cChipInit();

struct icm_chip;
/* register the IC chip into the chip list */
void registerIcChip(struct icm_chip* pChip);

enum e_icm_error
{
	E_ICM_OK = 0,
	E_ICM_READ_ID_ERROR = -1,
	E_ICM_VCOM_TIMES_NOT_CHANGE = -2,
	E_ICM_VCOM_VALUE_NOT_CHANGE = -3,
};

typedef int (*_get_id)(DEV_CHANNEL_E channel, unsigned char *p_id_data, int id_data_len);

typedef int (*_check_ok)(DEV_CHANNEL_E channel);

typedef int (*_read_vcom_otp_times)(DEV_CHANNEL_E channel, int* p_otp_vcom_times);

typedef int (*_read_vcom_otp_info)(DEV_CHANNEL_E channel, int* p_otp_vcom_times,int* p_otp_vcom_value);

typedef int (*_write_vcom_otp_value)(DEV_CHANNEL_E channel, int otp_vcom_value);

typedef int (*_read_vcom)(DEV_CHANNEL_E channel, int* p_vcom_value);

typedef int (*_write_vcom)(DEV_CHANNEL_E channel, int vcom_value);

typedef int (*_get_id_otp_info)(DEV_CHANNEL_E channel, unsigned char *p_id_data, int* p_id_data_len, unsigned char* p_otp_times);

typedef int (*_burn_id)(DEV_CHANNEL_E channel, unsigned char *p_id_data,int id_data_len, int ptn_id);

typedef int (*_check_vcom_otp_burn_ok)(DEV_CHANNEL_E channel, int vcom_value, int last_otp_times,int *p_read_vcom);

typedef void (*_reset_private_data)(DEV_CHANNEL_E channel);

typedef int (*_write_analog_gamma_reg_data)(DEV_CHANNEL_E channel, unsigned char *p_analog_gamma_reg_data, int analog_gamma_reg_len);

typedef int (*_read_analog_gamma_reg_data)(DEV_CHANNEL_E channel, unsigned char *p_analog_gamma_reg_data_buffer, int analog_gamma_reg_len);


typedef int (*_write_fix_analog_gamma_reg_data)(DEV_CHANNEL_E channel);

typedef int (*_write_digital_gamma_reg_data)(DEV_CHANNEL_E channel,unsigned char *p_digital_gamma_reg_data,
		int digital_gamma_reg_len);

typedef int (*_write_fix_digital_gamma_reg_data)(DEV_CHANNEL_E channel);

typedef int (*_d3g_control)(DEV_CHANNEL_E channel, int enable);

#define ICM_BURN_SECOND_FLAG		(0x00000001)

typedef int (*_burn_gamma_otp_values)(DEV_CHANNEL_E channel, int burn_flag, int enable_burn_vcom, int vcom,
											unsigned char *p_analog_gamma_reg_data, int analog_gamma_reg_data_len,
											unsigned char *p_d3g_r_reg_data, int d3g_r_reg_data_len,
											unsigned char *p_d3g_g_reg_data, int deg_g_reg_data_len,
											unsigned char *p_d3g_b_reg_data, int deg_b_reg_data_len);

typedef int (*_check_gamma_otp_values)(DEV_CHANNEL_E channel, int enable_burn_vcom, int new_vcom_value, int last_vcom_otp_times,
											unsigned char *p_analog_gamma_reg_data, int analog_gamma_reg_data_len,
											unsigned char *p_d3g_r_reg_data, int d3g_r_reg_data_len,
											unsigned char *p_d3g_g_reg_data, int deg_g_reg_data_len,
											unsigned char *p_d3g_b_reg_data, int deg_b_reg_data_len);

typedef int (*_get_analog_gamma_reg_data)(DEV_CHANNEL_E channel, unsigned char *p_analog_gamma_reg_data, int *p_analog_gamma_reg_data_len);

typedef int (*_get_digital_gamma_reg_data)(DEV_CHANNEL_E channel, unsigned char *p_d3g_r_reg_data, int *p_d3g_r_reg_data_len,
											unsigned char *p_d3g_g_reg_data, int *p_deg_g_reg_data_len,
											unsigned char *p_d3g_b_reg_data, int *p_deg_b_reg_data_len);

typedef void (*_snd_init_code)(DEV_CHANNEL_E channel, const char* pInitCodeName);

#define CHIP_NAME_MAX_LEN	(64)

typedef struct icm_chip
{
	char name[CHIP_NAME_MAX_LEN];
	_snd_init_code snd_init_code;
	// callback
	_reset_private_data reset_private_data;
	_get_id id;
	_check_ok check_ok;
	_read_vcom_otp_times read_vcom_otp_times;
	_read_vcom_otp_info read_vcom_otp_info;
	_write_vcom_otp_value write_vcom_otp_value;
	_read_vcom read_vcom;
	_write_vcom write_vcom;
	_check_vcom_otp_burn_ok check_vcom_otp_burn_ok;

	// gamma 
	_write_analog_gamma_reg_data write_analog_gamma_reg_data;
	_read_analog_gamma_reg_data read_analog_gamma_reg_data;
	_write_fix_analog_gamma_reg_data write_fix_analog_gamma_reg_data;
	
	_write_digital_gamma_reg_data write_digital_gamma_reg_data;
	_write_fix_digital_gamma_reg_data write_fix_digital_gamma_reg_data;

	_d3g_control d3g_control;

	// otp 
	_burn_gamma_otp_values burn_gamma_otp_values;
	_check_gamma_otp_values check_gamma_otp_values;

	// read info
	_get_analog_gamma_reg_data get_analog_gamma_reg_data;
	_get_digital_gamma_reg_data get_digital_gamma_reg_data;

	_get_id_otp_info get_id_otp_info;
	_burn_id burn_id;
}icm_chip_t;

#define IC_CHIP_MAX_NUM	(256)

typedef enum
{
	CHANNEL_IDLE,
	CHANNEL_BUSY,
}channelState_e;

typedef struct icChipMng
{
	int index;	//The current chip index;
	icm_chip_t* chips[IC_CHIP_MAX_NUM];
	channelState_e state[DEV_CHANNEL_NUM];
	char initCodeName[256];
}icChipMng_t;

int ic_mgr_init();

int ic_mgr_term();

void icMgrSetInitCodeName(char* pName);

void icMgrSndInitCode(DEV_CHANNEL_E channel);

int ic_mgr_set_current_chip_id(char* str_chip_id);

int ic_mgr_set_channel_state(DEV_CHANNEL_E channel, unsigned int state);

int ic_mgr_read_chip_id(DEV_CHANNEL_E channel, unsigned char* p_id_data, int id_data_len);

int ic_mgr_check_chip_ok(DEV_CHANNEL_E channel);

int ic_mgr_read_chip_vcom_otp_times(DEV_CHANNEL_E channel, int* p_otp_vcom_times);

int ic_mgr_read_chip_vcom_otp_value(DEV_CHANNEL_E channel, int* p_otp_vcom_times, int* p_otp_vcom_value);

int ic_mgr_write_chip_vcom_otp_value(DEV_CHANNEL_E channel, int otp_vcom_value);

int ic_mgr_read_vcom(DEV_CHANNEL_E channel, int* p_vcom_value);

int ic_mgr_write_vcom(DEV_CHANNEL_E channel, int vcom_value);

int ic_mgr_check_vcom_otp_burn_ok(DEV_CHANNEL_E channel, int vcom_value, int last_otp_times, int *p_read_vcom);

int ic_mgr_write_analog_gamma_data(DEV_CHANNEL_E channel, unsigned char *p_analog_gamma_reg_data,
									int analog_gamma_reg_data_len);

int ic_mgr_read_analog_gamma_data(DEV_CHANNEL_E channel, unsigned char *p_analog_gamma_reg_data,
									int analog_gamma_reg_data_len);

int ic_mgr_write_fix_analog_gamma_data(DEV_CHANNEL_E channel);

int ic_mgr_write_digital_gamma_data(DEV_CHANNEL_E channel, unsigned char *p_digital_gamma_reg_data,
									int digital_gamma_reg_data_len);

int ic_mgr_write_fix_digital_gamma_data(DEV_CHANNEL_E channel);

int ic_mgr_d3g_control(DEV_CHANNEL_E channel, int enable);

int ic_mgr_burn_gamma_otp_values(DEV_CHANNEL_E channel, int burn_flag, int enable_burn_vcom, int vcom,
									unsigned char *p_analog_gamma_reg_data, int analog_gamma_reg_data_len,
									unsigned char *p_d3g_r_reg_data, int d3g_r_reg_data_len,
									unsigned char *p_d3g_g_reg_data, int deg_g_reg_data_len,
									unsigned char *p_d3g_b_reg_data, int deg_b_reg_data_len);

int ic_mgr_check_gamma_otp_values(DEV_CHANNEL_E channel, int enable_burn_vcom, int new_vcom_value, int last_vcom_otp_times,
									unsigned char *p_analog_gamma_reg_data, int analog_gamma_reg_data_len,
									unsigned char *p_d3g_r_reg_data, int d3g_r_reg_data_len,
									unsigned char *p_d3g_g_reg_data, int deg_g_reg_data_len,
									unsigned char *p_d3g_b_reg_data, int deg_b_reg_data_len);

int ic_mgr_get_analog_gamma_reg_data(DEV_CHANNEL_E channel, unsigned char *p_analog_gamma_reg_data,
										int *p_analog_gamma_reg_data_len);

int ic_mgr_get_digital_gamma_reg_data_t(DEV_CHANNEL_E channel, unsigned char *p_d3g_r_reg_data, int *p_d3g_r_reg_data_len,
									unsigned char *p_d3g_g_reg_data, int *p_d3g_g_reg_data_len,
									unsigned char *p_d3g_b_reg_data, int *p_d3g_b_reg_data_len);

int ic_mgr_read_chip_id_otp_info(DEV_CHANNEL_E channel, unsigned char* p_id_data, int *p_id_data_len, unsigned char* p_otp_times);

int ic_mgr_burn_chip_id(DEV_CHANNEL_E channel, unsigned char* p_id_data, int id_data_len, int ptn_id);


void burnVcom(DEV_CHANNEL_E channel, unsigned char value);

unsigned char readVcom(DEV_CHANNEL_E channel);

void writeVcom(DEV_CHANNEL_E channel, unsigned char value);

unsigned int readVcomOtpTimes(DEV_CHANNEL_E channel);

int qc_cabc_test(DEV_CHANNEL_E channel, unsigned short *p_pwm_freq, unsigned short *p_duty);

#ifdef __cplusplus
}
#endif
#endif
