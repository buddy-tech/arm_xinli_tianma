#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include "spi_sp6.h"
#include "hi_unf_spi.h"
#include "common.h"

static int spi_read(char *pInData, int dataLen, char *poutData)
{
	return spiCtrlRead(SPI1_3_CS, pInData, dataLen, poutData);
}

static int spi_write(char *pInData, int dataLen)
{
	return spiCtrlWrite(SPI1_3_CS, pInData, dataLen);
}

typedef struct data8Bit{
	unsigned char addr;
	unsigned char reg;
	unsigned char value;
}data8Bit_t;

int sp6_write_8bit_data(unsigned char reg, unsigned char value)
{
	data8Bit_t data = {0x70, 0x0, 0x0};

	data.reg = reg;
	data.value = value;
	return spi_write(&data, sizeof(data));
}

unsigned char sp6_read_8bit_data(unsigned char reg)
{
	data8Bit_t data = {0x78, 0x0, 0x0};
	data.reg = reg;
	unsigned char read_buf[3] = "";

	spi_read(&data, sizeof(data), read_buf);
	return read_buf[2];
}

//寄存器名称：lvds_ttl_ctrl地址：20h 类型：-RW
// bit7~bit6:ttl模式选择(00:normal；01: hs+vs mode；10: DE only mode)
// Bit5为通道2的打开ttl使能信号，为低时打开使能。
// Bit4为通道1的打开ttl使能信号，为低时打开使能。
// Bit3为通道2的打开lvds预加重使能信号，为高时打开使能。
// Bit2为通道1的打开lvds预加重使能信号，为高时打开使能。
// Bit1为通道2的打开lvds使能信号，为高时打开使能。
// Bit0为通道1的打开lvds使能信号，为高时打开使能。

int sp6_open_lvds_signal()
{
	return sp6_write_8bit_data(0x20, 0x3f);
}

int sp6_close_lvds_signal()
{
	return sp6_write_8bit_data(0x20, 0x30);
}

static int sp6_set_ttl_timing_mode(int mode)
{
	unsigned char value = 0;
	value = sp6_read_8bit_data(0x20);

	value &= (~0x30);
	value |= (mode&0x3 << 6);

	return sp6_write_8bit_data(0x20, value);
}

int sp6_open_ttl_signal()
{
	return sp6_set_ttl_timing_mode(TTL_LVDS_NORMAL_MODE);
}

int sp6_open_ttl_buffer()
{
	return sp6_write_8bit_data(0x20, 0x00);
}

int sp6_close_ttl_signal()
{
	return sp6_write_8bit_data(0x20, 0x30);
}

// reg29:
// bit 0: open/short
// bit 1: ttl clock pol. channel 1; [right]
// bit 2: ttl clock pol. channel 2; [left]

void sp6_entry_open_short_mode()
{
	sp6_write_8bit_data(0x29, 0x01);
}

void sp6_leave_open_short_mode()
{
	sp6_write_8bit_data(0x29, 0x00);
}

unsigned char sp6_read_open_short_mode()
{
	return sp6_read_8bit_data(0x29);
}

int sp6_set_clock_pol_mode(int is_on)
{
	unsigned char value = 0;
	if (is_on)
		value = 0x02;

	return sp6_write_8bit_data(0x29, value);
}

int sp6_set_open_short_data(TTL_LVDS_CHANNEL_E channel, unsigned int value)
{
	if (TTL_LVDS_CHANNEL_LEFT == channel)
	{
		sp6_write_8bit_data(0x25, value&0xff);
		sp6_write_8bit_data(0x26, (value >> 8)&0xff);
		sp6_write_8bit_data(0x27, (value >> 16)&0xff);
		sp6_write_8bit_data(0x28, (value >> 24)&0xff);
	}
	else
	{
		sp6_write_8bit_data(0x21, value&0xff);
		sp6_write_8bit_data(0x22, (value >> 8)&0xff);
		sp6_write_8bit_data(0x23, (value >> 16)&0xff);
		sp6_write_8bit_data(0x24, (value >> 24)&0xff);
	}
}

int sp6_read(unsigned char reg)
{
	return sp6_read_8bit_data(reg);
}

int sp6_write(unsigned char reg, unsigned char data)
{
	return sp6_write_8bit_data(reg, data);
}

// bit1~bit0:数据位宽(01:6bit；10:8bit；00:10bit)。
// bit2 0:vasa 1:jeida
int sp6_set_lcd_bit_and_vesa_jeida(int bit_mode, int is_vesa_mode)
{
	unsigned value = 0;

	switch(bit_mode)
	{
		case 6:
			value |= 0x01;
			break;

		case 8:
			value |= 0x02;
			break;

		case 10:
			value |= 0x03;
			break;
	}

	if (!is_vesa_mode)
		value &= ~0x04;
	else
		value |= 0x04;

	return sp6_write_8bit_data(0x2a, value);
}
