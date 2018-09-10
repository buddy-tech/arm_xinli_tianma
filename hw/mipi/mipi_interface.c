#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "hi_unf_spi.h"
#include "hi_unf_gpio.h"
#include "mipi_interface.h"

void WriteSSD2828(int spiNo, char addr, unsigned short data)
{
	unsigned char tx[3] = {0x70,0x00,0x00};
	tx[2] = addr;
	spiCtrlWrite(spiNo, tx, 3);
	tx[0] = 0x72;
	tx[1] = (data&0xFF00) >> 8;
	tx[2] = data&0xFF;
	spiCtrlWrite(spiNo, tx, 3);
}

void WriteCMDSSD2828(int spiNo, unsigned char command)
{
	unsigned char tx[3] = {0x70, 0x00, 0x00};
	tx[2] = command;
	spiCtrlWrite(spiNo, tx, 3);
}

void WriteDATASSD2828(int spiNo, unsigned short data)
{
	unsigned char tx[3] = {0x72,0x00,0x00};
	tx[1] = (data&0xFF00) >> 8;
	tx[2] = data&0xFF;
	spiCtrlWrite(spiNo, tx, 3);
}

unsigned short ReadSSD2828Reg(int spiNo, char addr)
{
    unsigned char tx[3] = {0x70,0x00,0x00};
    unsigned char rx[3] = {0x00,0x00,0x00};
    tx[2] = addr;
    spiCtrlWrite(spiNo, tx, 3);

    memset(tx, 0, 3);
    tx[0] = 0x73;
    spiCtrlRead(spiNo, tx, 3, rx);

    return (rx[1]<<8)|rx[2];
}

unsigned short ReadDATASSD2828Reg(int spiNo, char addr)
{
    unsigned char tx[3] = {0x73,0x00,0x00};
    unsigned char rx[3] = {0x00,0x00,0x00};

    spiCtrlRead(spiNo, tx, 3, rx);

    return (rx[1]<<8)|rx[2];
}


void WriteFPGATo2828_8bit(int spiNo,char addr,unsigned short data)
{
	unsigned char tx[3] = {0x70,0x00,0x00};
	tx[2] = addr;
	spiCtrlWrite(spiNo, tx, 3);
	tx[0] = 0x72;
	tx[1] = (data&0xFF00) >> 8;
	tx[2] = data&0xFF;
	spiCtrlWrite(spiNo, tx, 3);
}

void WriteCMDFPGATo2828_8bit(int spiNo, char command)
{
	unsigned char tx[3] = {0x70,0x00,0x00};
	tx[2] = command;
	spiCtrlWrite(spiNo, tx, 3);
}

void WriteDATAFPGATo2828_8bit(int spiNo,unsigned short data)
{
	unsigned char tx[3] = {0x72,0x00,0x00};
	tx[1] = (data&0xFF00) >> 8;
	tx[2] = data&0xFF;
	spiCtrlWrite(spiNo, tx, 3);
}

void SSD2828_read_reg(int spiNo, char reg)
{
	unsigned short id = ReadSSD2828Reg(spiNo, reg);
	printf("2828 read reg: 0x%02x, value: 0x%04x.\n", reg, id);
}

void SSD2828_write_reg(int spiNo, char reg, unsigned short data)
{
	WriteSSD2828(spiNo, reg, data);
	printf("2828 write reg: 0x%02x, value: 0x%04x.\n", reg, data);
}

void mtp_power_on(int channel)
{
	if(channel == 0 || channel == 1)	//left
		gpio_set_output_value(MTP_CHANNEL_ENABLE_L, 1); //output
	else
		gpio_set_output_value(MTP_CHANNEL_ENABLE_R, 1); //output
}

void mtp_power_off(int channel)
{
	if(channel == 0 || channel == 1)	//left
		gpio_set_output_value(MTP_CHANNEL_ENABLE_L, 0); //output
	else
		gpio_set_output_value(MTP_CHANNEL_ENABLE_R, 0); //output
}
