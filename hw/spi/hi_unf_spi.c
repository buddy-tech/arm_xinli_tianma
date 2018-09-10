/*
 * SPI testing utility (using spidev driver)
 *
 * Copyright (c) 2007  MontaVista Software, Inc.
 * Copyright (c) 2007  Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * Cross-compile with cross-gcc -I/path/to/cross-kernel/include
 */

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
#include "hi_unf_gpio.h"

static uint32_t mode  = 0;
static uint8_t  bits  = 8;
static uint32_t speed = 1500000;
static uint16_t delay = 0;

static int s_spi_handle = -1;
static int s_spi2_handle = -1;

static virtualSpiCtrlM_t* pSgVirtualSpiM[2] = {0};

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static int __read_write(int fd, int ctrlNum, int index, void* pData)
{
	int ret = -1;

	if (NULL == pData)
		return ret;

	if (ctrlNum == 0)
	{
		gpio_set_output_value(SPI0_CS_PIN(index), 0);
		ret = ioctl(fd, SPI_IOC_MESSAGE(1), pData);
		gpio_set_output_value(SPI0_CS_PIN(index), 1);
	}
	else
	{
		gpio_set_output_value(SPI1_CS_PIN(index), 0);
		ret = ioctl(fd, SPI_IOC_MESSAGE(1), pData);
		gpio_set_output_value(SPI1_CS_PIN(index), 1);
	}

	return ret;
}

static void reinit_cs_state(int spiNo)
{
    if (spiNo >= SPI0_0_CS && spiNo <= SPI0_2_CS)
    {
    	gpio_set_output_value(SPI0_CS_PIN(0), 1);
    	gpio_set_output_value(SPI0_CS_PIN(1), 1);
    	gpio_set_output_value(SPI0_CS_PIN(2), 1);
    }
    else
    {
    	gpio_set_output_value(SPI1_CS_PIN(0), 1);
    	gpio_set_output_value(SPI1_CS_PIN(1), 1);
    	gpio_set_output_value(SPI1_CS_PIN(2), 1);
    	gpio_set_output_value(SPI1_CS_PIN(3), 1);
    }
}

int spiCtrlRead(int spiNo, char *pInData, int dataLen, char *poutData)
{
    int ret = -1;
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)pInData,
        .rx_buf = poutData,
        .len = dataLen,
        .delay_usecs = delay,
        .speed_hz = speed,
        .bits_per_word = bits,
    };

    reinit_cs_state(spiNo);

	switch (spiNo)
	{
	case SPI0_0_CS:
		ret = __read_write(s_spi_handle, 0, 0, &tr);
		break;

	case SPI0_1_CS:
		ret = __read_write(s_spi_handle, 0, 1, &tr);
		break;

	case SPI0_2_CS:
		ret = __read_write(s_spi_handle, 0, 2, &tr);
		break;

	case SPI1_0_CS:
		ret = __read_write(s_spi2_handle, 1, 0, &tr);
		break;

	case SPI1_1_CS:
		ret = __read_write(s_spi2_handle, 1, 1, &tr);
		break;

	case SPI1_2_CS:
		ret = __read_write(s_spi2_handle, 1, 2, &tr);
		break;

	case SPI1_3_CS:
		ret = __read_write(s_spi2_handle, 1, 3, &tr);
		break;

	default:
		printf("HI_UNF_SPI_Read error: Invalid param. channel = %d.\n.", spiNo);
		break;
	}	

    if (ret < 1)
    {
        printf("HI_UNF_SPI_Read error: ioctl failed! ret = %d. \n", ret);
        return -1;
    }
	
    return 0;
}

int spiCtrlWrite(int spiNo, char *pInData, int dataLen)
{
    int ret;
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)pInData,
        .rx_buf = 0,
        .len = dataLen,
        .delay_usecs = delay,
        .speed_hz = speed,
        .bits_per_word = bits,
    };

    reinit_cs_state(spiNo);

	switch (spiNo)
	{
	case SPI0_0_CS:
		ret = __read_write(s_spi_handle, 0, 0, &tr);
		break;

	case SPI0_1_CS:
		ret = __read_write(s_spi_handle, 0, 1, &tr);
		break;

	case SPI0_2_CS:
		ret = __read_write(s_spi_handle, 0, 2, &tr);
		break;

	case SPI1_0_CS:
		ret = __read_write(s_spi2_handle, 1, 0, &tr);
		break;

	case SPI1_1_CS:
		ret = __read_write(s_spi2_handle, 1, 1, &tr);
		break;

	case SPI1_2_CS:
		ret = __read_write(s_spi2_handle, 1, 2, &tr);
		break;
		
	case SPI1_3_CS:
		ret = __read_write(s_spi2_handle, 1, 3, &tr);
		break;

	default:
		printf("HI_UNF_SPI_Write error: Invalid param. channel = %d.\n.", spiNo);
		break;
	}

    if (ret < 1)
    {
        printf("%s error: ioctl failed!\n", __FUNCTION__);
        return -1;
    }
	
    return 0;
}


static int __spi_open(char *devName)
{
	int ret = 0;
	int fd;

	fd = open(devName, O_RDWR);
	if (fd < 0)
	{
		printf("can't open device %s\n", devName);
		return -1;
	}

	/*
	 * spi mode
	 */
	ret = ioctl(fd, SPI_IOC_WR_MODE32, &mode);
	if (ret == -1)
	{
		printf("can't set spi mode\n");
		return -1;
	}

	ret = ioctl(fd, SPI_IOC_RD_MODE32, &mode);
	if (ret == -1)
	{
		printf("can't get spi mode\n");
		return -1;
	}

	/*
	 * bits per word
	 */
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
	{
		printf("can't set bits per word\n");
		return -1;
	}

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
	{
		printf("can't get bits per word\n");
		return -1;
	}

	/*
	 * max speed hz
	 */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
	{
		printf("can't set max speed hz\n");
		return -1;
	}

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
	{
		printf("can't get max speed hz\n");
		return -1;
	}
	
	return fd;
}

static void __spi_close(int fd)
{
	close(fd);
}

void spiCtrlInit()
{
	// spi0
	gpio_set_output_value(SPI0_CS_PIN(0), 1);
	gpio_set_output_value(SPI0_CS_PIN(1), 1);
	gpio_set_output_value(SPI0_CS_PIN(2), 1);

	// spi1
	gpio_set_output_value(SPI1_CS_PIN(0), 1);
	gpio_set_output_value(SPI1_CS_PIN(1), 1);
	gpio_set_output_value(SPI1_CS_PIN(2), 1);
	gpio_set_output_value(SPI1_CS_PIN(3), 1);

	s_spi_handle = __spi_open(MIPI_SPI_DEV_NAME);
	s_spi2_handle = __spi_open(MIPI_SPI2_DEV_NAME);

	pSgVirtualSpiM[0] = virtualSpiCtrlMMalloc();
	pSgVirtualSpiM[0]->setFreq(pSgVirtualSpiM[0], 500*1000);
	pSgVirtualSpiM[0]->setByteFormat(pSgVirtualSpiM[0], BYTE_8_BIT);
	pSgVirtualSpiM[0]->setGpio(pSgVirtualSpiM[0], LCM2_SPI_MOSI, LCM2_SPI_MOSI, LCM2_SPI_CS, LCM2_SPI_SCK, LCM2_SPI_MOSI_DIR);

	pSgVirtualSpiM[1] = virtualSpiCtrlMMalloc();
	pSgVirtualSpiM[1]->setFreq(pSgVirtualSpiM[1], 500*1000);
	pSgVirtualSpiM[1]->setByteFormat(pSgVirtualSpiM[1], BYTE_8_BIT);
	pSgVirtualSpiM[1]->setGpio(pSgVirtualSpiM[1], LCM1_SPI_MOSI, LCM1_SPI_MOSI, LCM1_SPI_CS, LCM1_SPI_SCK, LCM1_SPI_MOSI_DIR);
}

void spiCtrlfree()
{
	__spi_close(s_spi_handle);
	__spi_close(s_spi2_handle);

	pSgVirtualSpiM[0]->free(pSgVirtualSpiM[0]);
	pSgVirtualSpiM[1]->free(pSgVirtualSpiM[1]);
}

#if 0
#define SET_FORMAT_BIT(_data, _val, _mask, _move)	((_data) = (((_data) & (~((_mask) << (_move)))) | (((_val) & (_mask)) << (_move))))
#define SET_WR_BIT(_data, _val)		SET_FORMAT_BIT(_data, _val, 0x1, 15)
#define SET_W_BIT(_data)			SET_WR_BIT(_data, 0)
#define SET_R_BIT(_data)			SET_WR_BIT(_data, 1)
#define SET_SID_BIT(_data, _val)	SET_FORMAT_BIT(_data, _val, 0x3, 13)
#define SET_ADDR_BIT(_data, _val)	SET_FORMAT_BIT(_data, _val, 0x1f, 8)
#define SET_DATA_BIT(_data, _val)	SET_FORMAT_BIT(_data, _val, 0xff, 0)
#else
#define SET_FORMAT_BIT(_data, _val, _mask, _move)	((_data) = (((_data) & (~((_mask) << (_move)))) | (((_val) & (_mask)) << (_move))))
#define SET_WR_BIT(_data, _val)		SET_FORMAT_BIT(_data, _val, 0x1, 7)
#define SET_W_BIT(_data)			SET_WR_BIT(_data, 0)
#define SET_R_BIT(_data)			SET_WR_BIT(_data, 1)
#endif

void vspi_read(unsigned int channel, unsigned char addr, unsigned char* pValue)
{
	virtualSpiCtrlM_t* pM = NULL;
	if (channel == 0 || channel == 1)
		pM = pSgVirtualSpiM[0];
	else
		pM = pSgVirtualSpiM[1];

	vspiCtrlSetMode((SPI_CTRL_ID)pM, WIRE_3_LINE);
	vspiCtrlSetByteFormat((SPI_CTRL_ID)pM, BYTE_8_BIT);

	SET_R_BIT(addr);

	pM->setCsState(pM, 1);
	pM->setCsState(pM, 0);
	pM->write(pM, addr);
	pM->read(pM, (unsigned int*)pValue);
	pM->setCsState(pM, 1);
}

void vspi_write(unsigned int channel, unsigned char addr, unsigned char value)
{
	virtualSpiCtrlM_t* pM = NULL;
	if (channel == 0 || channel == 1)
		pM = pSgVirtualSpiM[0];
	else
		pM = pSgVirtualSpiM[1];

	vspiCtrlSetMode((SPI_CTRL_ID)pM, WIRE_3_LINE);
	vspiCtrlSetByteFormat((SPI_CTRL_ID)pM, BYTE_8_BIT);

	SET_W_BIT(addr);

	pM->setCsState(pM, 1);
	pM->setCsState(pM, 0);
	pM->write(pM, addr);
	pM->write(pM, value);
	pM->setCsState(pM, 1);
}

SPI_CTRL_ID vspiCtrlInstance(unsigned int channel)
{
	virtualSpiCtrlM_t* pM = NULL;
	if (channel == 0 || channel == 1)
		pM = pSgVirtualSpiM[0];
	else
		pM = pSgVirtualSpiM[1];

	return (SPI_CTRL_ID) pM;
}

void vspiCtrlSetByteFormat(SPI_CTRL_ID id, spiByteFormat_e format)
{
	virtualSpiCtrlM_t* pM = (virtualSpiCtrlM_t*)id;
	if (NULL == pM)
		return;

	pM->setByteFormat(pM, format);
}

void vspiCtrlSetCsState(SPI_CTRL_ID id, bool bSelected)
{
	virtualSpiCtrlM_t* pM = (virtualSpiCtrlM_t*)id;
	if (NULL == pM)
		return;

	pM->setCsState(pM, !bSelected);
}

void vspiCtrlSetMode(SPI_CTRL_ID id, VSPI_MODE_E mode)
{
	virtualSpiCtrlM_t* pM = (virtualSpiCtrlM_t*)id;
	if (NULL == pM)
		return;

	if (pM == pSgVirtualSpiM[0])
	{
		if (mode == WIRE_4_LINE)
			pM->setGpio(pM, LCM2_SPI_MOSI, LCM2_SPI_MISO, LCM2_SPI_CS, LCM2_SPI_SCK, LCM2_SPI_MOSI_DIR);
		else
			pM->setGpio(pM, LCM2_SPI_MOSI, LCM2_SPI_MOSI, LCM2_SPI_CS, LCM2_SPI_SCK, LCM2_SPI_MOSI_DIR);
	}
	else
	{
		if (mode == WIRE_4_LINE)
			pM->setGpio(pSgVirtualSpiM[1], LCM1_SPI_MOSI, LCM1_SPI_MISO, LCM1_SPI_CS, LCM1_SPI_SCK, LCM1_SPI_MOSI_DIR);
		else
			pM->setGpio(pSgVirtualSpiM[1], LCM1_SPI_MOSI, LCM1_SPI_MOSI, LCM1_SPI_CS, LCM1_SPI_SCK, LCM1_SPI_MOSI_DIR);
	}
}

int vspiCtrlWrite(SPI_CTRL_ID id, unsigned int value)
{
	virtualSpiCtrlM_t* pM = (virtualSpiCtrlM_t*)id;
	if (NULL == pM)
		return 0;

	return pM->write(pM, value);
}

void vspiCtrlRead(SPI_CTRL_ID id, unsigned int* pValue)
{
	virtualSpiCtrlM_t* pM = (virtualSpiCtrlM_t*)id;
	if (NULL == pM)
		return 0;

	pM->read(pM, (unsigned int*)pValue);
}
