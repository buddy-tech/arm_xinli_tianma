#ifndef _MCU_SPI_H_
#define _MCU_SPI_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

unsigned short mcu_read_data_16(unsigned char addr, unsigned char reg);

unsigned int mcu_write_data_16(unsigned char addr, unsigned char reg, unsigned short value);

int mcu_set_vcom_volt(unsigned short value);

int mcu_set_vcom_max_volt(unsigned short value);

int mcu_set_vcom_enable(bool bEnable);

int mcu_read_vcom_volt(TTL_LVDS_CHANNEL_E channel);

int mcu_read_vcom_status();

unsigned short mcu_read_otp_volt();

unsigned short mcu_read_otp_status();

unsigned short mcu_read_open_short_data(TTL_LVDS_CHANNEL_E channel);

unsigned int mcu_read_ntc_ag_data(TTL_LVDS_CHANNEL_E channel);

int mcu_set_pwm_freq(unsigned short freq);

int mcu_set_pwm_enable(unsigned char duty, bool bEnable);

void mcu_set_pwm(unsigned short freq, unsigned char duty, bool bEnable);

unsigned short mcu_read_ivcom();
#ifdef __cplusplus
}
#endif
#endif
