
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include "hi_unf_spi.h"
#include "mcu_spi.h"

static int mcu_spi_read(char *pInData, int dataLen, char *poutData)
{
    return spiCtrlRead(SPI1_2_CS, pInData, dataLen, poutData);
}

static int mcu_spi_write(char *pInData, int dataLen)
{
	return spiCtrlWrite(SPI1_2_CS, pInData, dataLen);
}

static short __read_16bits_data(unsigned char addr, unsigned char reg)
{
	unsigned char tx[2] = { 0 };
	unsigned char rx[2] = { 0 };
	tx[0] = addr;
	tx[1] = reg;

	unsigned int ret = 0;

	do
	{
		ret = mcu_spi_read(tx, sizeof(tx), rx);
		if (ret < 0)
			break;

		ret = (rx[0] & 0xff) << 8 | (rx[1] & 0xff);
	} while (0);

	return ret;
}

static int __write_16bits_data(unsigned char addr, unsigned char reg)
{
	unsigned char tx[2] = { 0 };
	tx[0] = addr;
	tx[1] = reg;

	return mcu_spi_write(tx, sizeof(tx));
}

unsigned short mcu_read_data_16(unsigned char addr, unsigned char reg)
{
	__write_16bits_data(addr, reg);
	return __read_16bits_data(addr, reg);
}

unsigned int mcu_write_data_16(unsigned char addr, unsigned char reg, unsigned short value)
{
	__write_16bits_data(addr, reg);
	return __write_16bits_data((value >> 8) & 0xff, value & 0xff);
}

int mcu_set_vcom_volt(unsigned short value)
{
	return mcu_write_data_16(0x01, 0x01, value);
}

int mcu_set_vcom_max_volt(unsigned short value)
{
	return mcu_write_data_16(0x01, 0x05, value);
}

int mcu_set_vcom_enable(bool bEnable)
{
	if (bEnable)
		return mcu_write_data_16(0x01, 0x02, 0x01);
	else
		return mcu_write_data_16(0x01, 0x02, 0x00);
}

int mcu_read_vcom_volt(TTL_LVDS_CHANNEL_E channel)
{
	if (channel == TTL_LVDS_CHANNEL_LEFT) // left
		return mcu_read_data_16(0x02, 0x01);
	else // right
		return mcu_read_data_16(0x02, 0x11);
}

int mcu_read_vcom_status()
{
	return mcu_read_data_16(0x02, 0x02) & 0xff;
}

unsigned short mcu_read_otp_volt()
{
	return mcu_read_data_16(0x02, 0x03);
}

unsigned short mcu_read_otp_status()
{
	return mcu_read_data_16(0x02, 0x04);
}

// Open/Short: 0.1ma;
unsigned short mcu_read_open_short_data(unsigned int channel)
{
	if (channel == TTL_LVDS_CHANNEL_LEFT)	// left
		return mcu_read_data_16(0x02, 0x05);
	else // right
		return mcu_read_data_16(0x02, 0x15);
}

// NTC/AG: omg.
unsigned int mcu_read_ntc_ag_data(TTL_LVDS_CHANNEL_E channel)
{
	if (channel == TTL_LVDS_CHANNEL_LEFT)
		return (mcu_read_data_16(0x02, 0x06) & 0xffff) << 16 | (mcu_read_data_16(0x02, 0x07) & 0xffff);
	else	// right
		return (mcu_read_data_16(0x02, 0x16) & 0xffff) << 16 | (mcu_read_data_16(0x02, 0x17) & 0xffff);
}

int mcu_set_pwm_freq(unsigned short freq)
{
	return mcu_write_data_16(0x01, 0x08, freq);
}

int mcu_set_pwm_enable(unsigned char duty, bool bEnable)
{
	if (bEnable)
		return mcu_write_data_16(0x01, 0x09, (duty << 8) && 0x1);
	else
		return mcu_write_data_16(0x01, 0x09, (duty << 8) && 0x0);
}

void mcu_set_pwm(unsigned short freq, unsigned char duty, bool bEnable)
{
	mcu_set_pwm_freq(freq);
	_msleep(5);
	mcu_set_pwm_enable(duty, bEnable);
}

unsigned short mcu_read_ivcom()
{
	return mcu_read_data_16(0x02, 0x0A);
}
