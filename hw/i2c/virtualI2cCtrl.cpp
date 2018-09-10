/*
 * virtualI2cCtrl.cpp
 *
 *  Created on: 2018-7-13
 *      Author: tys
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#include "virtualI2cCtrl.h"
#include "virtualI2cCtrlApi.h"
#include "hi_unf_gpio.h"
#include "common.h"

static void _writeOneBit(unsigned int gpioPin, unsigned int value)
{
	/* set Gpio pin as output */
	HI_UNF_GPIO_SetDirBit(gpioPin, 0);
	HI_UNF_GPIO_Enable(gpioPin, 1);
	HI_UNF_GPIO_WriteBit(gpioPin, value);
}

static unsigned int _readOneBit(unsigned int gpioPin)
{
	/* set Gpio pin as input */
	HI_UNF_GPIO_SetDirBit(gpioPin, 1);
	unsigned int value = 0;
	HI_UNF_GPIO_ReadBit(gpioPin, (int *)&value);
	return value;
}

static void virtualI2cCtrlGpioInit(struct virtualI2cCtrl* pCtrl)
{
	HI_UNF_GPIO_SetDirBit(pCtrl->sdaGpio, 0);
	HI_UNF_GPIO_SetDirBit(pCtrl->clkGpio, 0);
	HI_UNF_GPIO_SetDirBit(pCtrl->sdaDirGpio, 0);
	gpio_set_output_value(pCtrl->clkGpio, 1);
	gpio_set_output_value(pCtrl->sdaGpio, 0);
}

static void virtualI2cCtrlGpioDeinit(struct virtualI2cCtrl* pCtrl)
{
}

static void virtualI2cCtrlStart(struct virtualI2cCtrl* pCtrl)
{
	_writeOneBit(pCtrl->sdaDirGpio, 1);	// set direction as output
	_writeOneBit(pCtrl->clkGpio, 1);
	_writeOneBit(pCtrl->sdaGpio, 1);
	usleep(pCtrl->period/4);
	_writeOneBit(pCtrl->sdaGpio, 0);
	usleep(pCtrl->period/4);
}

static void virtualI2cCtrlSend(struct virtualI2cCtrl* pCtrl, unsigned char value)
{
	_writeOneBit(pCtrl->sdaDirGpio, 1);	// set direction as output

	for (int i = 0; i < 8; i++)
	{
		_writeOneBit(pCtrl->clkGpio, 0);
		usleep(pCtrl->period/4);
		_writeOneBit(pCtrl->sdaGpio, (value >> (8 - i - 1))&0x1);
		usleep(pCtrl->period/4);
		_writeOneBit(pCtrl->clkGpio, 1);
		usleep(pCtrl->period/4);
		_writeOneBit(pCtrl->clkGpio, 0);
		usleep(pCtrl->period/4);
	}
}

static void virtualI2cCtrlRcv(struct virtualI2cCtrl* pCtrl, unsigned char* pValue)
{
	*pValue = 0;
	_writeOneBit(pCtrl->sdaDirGpio, 0);	// set direction as input

	for (int i = 0; i < 8; i++)
	{
		_writeOneBit(pCtrl->clkGpio, 0);
		usleep(pCtrl->period/4);
		_writeOneBit(pCtrl->clkGpio, 1);
		usleep(pCtrl->period/4);
		*pValue |= _readOneBit(pCtrl->sdaGpio) << (8 - i - 1);
		usleep(pCtrl->period/4);
		_writeOneBit(pCtrl->clkGpio, 0);
		usleep(pCtrl->period/4);
	}
}

static void virtualI2cCtrlStop(struct virtualI2cCtrl* pCtrl)
{
	_writeOneBit(pCtrl->sdaDirGpio, 1);	// set direction as output
	_writeOneBit(pCtrl->sdaGpio, 0);
	usleep(pCtrl->period/4);
	_writeOneBit(pCtrl->clkGpio, 1);
	usleep(pCtrl->period/4);
	_writeOneBit(pCtrl->sdaGpio, 1);
}

static bool virtualI2cWaitAck(struct virtualI2cCtrl* pCtrl)
{
	_writeOneBit(pCtrl->sdaDirGpio, 0);	// set direction as input
	_writeOneBit(pCtrl->clkGpio, 1);
	usleep(pCtrl->period/4);
	unsigned char ack = _readOneBit(pCtrl->sdaGpio);
	usleep(pCtrl->period/4);
	_writeOneBit(pCtrl->clkGpio, 0);
	usleep(pCtrl->period/4);

	return I2C_ACK == ack;
}

static int virtualI2cCtrlWrite(struct virtualI2cCtrl* pCtrl, unsigned char slaveAddr,
		unsigned char regAddr, unsigned char value)
{
	int ret = -1;

	do
	{
		/* 1st step: i2c start */
		virtualI2cCtrlStart(pCtrl);

		/* 2nd step: write slave address and RW bit (MSB to LSB)*/
		unsigned char msbData = ((slaveAddr << 1) & 0xff) | I2C_WRITE_MODE;
		virtualI2cCtrlSend(pCtrl, msbData);

		/* 3rd step: wait Ack */
		if (!virtualI2cWaitAck(pCtrl))
			break;

		/* 4th step: write register address */
		virtualI2cCtrlSend(pCtrl, regAddr);

		/* 5th step: wait Ack */
		if (!virtualI2cWaitAck(pCtrl))
			break;

		/* 6th step: write register value */
		virtualI2cCtrlSend(pCtrl, value);

		/* 7th step: wait Ack */
		if (!virtualI2cWaitAck(pCtrl))
			break;

		ret = 0;
	}while (0);

	/* final step: stop */
	virtualI2cCtrlStop(pCtrl);

	if (ret < 0)
		printf("i2c write failed. slaveAddr:0x%x, "
				"regAddr:0x%x, value:0x%x\r\n", slaveAddr, regAddr, value);

	return ret;
}

static int virtualI2cCtrlRead(struct virtualI2cCtrl* pCtrl, unsigned char slaveAddr,
		unsigned char regAddr, unsigned char* pValue)
{
	int ret = -1;

	do
	{
		if (NULL == pValue)
			break;

		/* 1st step: i2c start */
		virtualI2cCtrlStart(pCtrl);

		/* 2nd step: write slave address and RW bit (MSB to LSB)*/
		unsigned char msbData = ((slaveAddr << 1) & 0xff) | I2C_WRITE_MODE;
		virtualI2cCtrlSend(pCtrl, msbData);

		/* 3rd step: wait Ack */
		if (!virtualI2cWaitAck(pCtrl))
			break;

		/* 4th step: write register address */
		virtualI2cCtrlSend(pCtrl, regAddr);

		/* 5th step: wait Ack */
		if (!virtualI2cWaitAck(pCtrl))
			break;

		/* 7st step: i2c restart */
		virtualI2cCtrlStart(pCtrl);

		/* 8nd step: write slave address and RW bit (MSB to LSB)*/
		msbData = ((slaveAddr << 1) & 0xff) | I2C_READ_MDOE;
		virtualI2cCtrlSend(pCtrl, msbData);

		/* 9rd step: wait Ack */
		if (!virtualI2cWaitAck(pCtrl))
			break;

		/* 10th step: read register value */
		virtualI2cCtrlRcv(pCtrl, pValue);

		/* 11th step: wait NoAck */
		if (virtualI2cWaitAck(pCtrl))
			break;

		ret = 0;
	}while (0);

	/* final step: stop */
	virtualI2cCtrlStop(pCtrl);

	if (ret < 0)
		printf("i2c read failed. slaveAddr:0x%x, regAddr:0x%x\r\n", slaveAddr, regAddr);

	return ret;
}

static void virtualI2cCtrlSetFreq(struct virtualI2cCtrl* pCtrl, unsigned int freq)
{
	switch (freq)
	{
	case I2C_FREQ_10K:
		pCtrl->period = 100;
		break;

	case I2C_FREQ_25K:
		pCtrl->period = 40;
		break;

	case I2C_FREQ_50K:
		pCtrl->period = 20;
		break;

	case I2C_FREQ_100K:
		pCtrl->period = 10;
		break;

	case I2C_FREQ_250K:
		pCtrl->period = 4;
		break;

	default:
		break;
	}
}

static void virtualI2cCtrlSetGpio(struct virtualI2cCtrl* pCtrl, unsigned int sdaGpio,
		unsigned int sclGpio, unsigned int sdaDirGpio)
{
	pCtrl->sdaGpio = sdaGpio;
	pCtrl->clkGpio = sclGpio;
	pCtrl->sdaDirGpio = sdaDirGpio;
	virtualI2cCtrlGpioInit(pCtrl);
}

static void virtualI2cCtrlFree(struct virtualI2cCtrl* pCtrl)
{
	virtualI2cCtrlGpioDeinit(pCtrl);
	free(pCtrl);
}

virtualI2cCtrl_t* virtualI2cCtrlMalloc()
{
	virtualI2cCtrl_t* pCtrl = NULL;

	do
	{
		pCtrl = (virtualI2cCtrl_t *)calloc(1, sizeof(virtualI2cCtrl_t));
		if (NULL == pCtrl)
			break;

		pCtrl->setFreq = virtualI2cCtrlSetFreq;
		pCtrl->setGpio = virtualI2cCtrlSetGpio;
		pCtrl->write = virtualI2cCtrlWrite;
		pCtrl->read = virtualI2cCtrlRead;
		pCtrl->free = virtualI2cCtrlFree;
		pCtrl->period = 100;	//I2C_FREQ_10K

	}while (0);

	return pCtrl;
}

static int virtualI2cCtrlMWrite(struct virtualI2cCtrlM* pM, unsigned char slaveAddr,
		unsigned char regAddr, unsigned char value)
{
	virtualI2cCtrl_t* pCtrl = (virtualI2cCtrl_t *)pM->id;
	return pCtrl->write(pCtrl, slaveAddr, regAddr, value);
}

static int virtualI2cCtrlMRead(struct virtualI2cCtrlM* pM, unsigned char slaveAddr,
		unsigned char regAddr, unsigned char* pValue)
{
	virtualI2cCtrl_t* pCtrl = (virtualI2cCtrl_t *)pM->id;
	return pCtrl->read(pCtrl, slaveAddr, regAddr, pValue);
}

static void virtualI2cCtrlMSetFreq(struct virtualI2cCtrlM* pM, i2cFreq_e freq)
{
	virtualI2cCtrl_t* pCtrl = (virtualI2cCtrl_t *)pM->id;

	pCtrl->setFreq(pCtrl, freq);
}

static void virtualI2cCtrlMSetGpio(struct virtualI2cCtrlM* pM, unsigned int sdaGpio,
		unsigned int sclGpio, unsigned int sdaDirGpio)
{
	virtualI2cCtrl_t* pCtrl = (virtualI2cCtrl_t *)pM->id;
	pCtrl->setGpio(pCtrl, sdaGpio, sclGpio, sdaDirGpio);
}

static void virtualI2cCtrlMFree(struct virtualI2cCtrlM* pM)
{
	virtualI2cCtrl_t* pCtrl = (virtualI2cCtrl_t *)pM->id;
	if (pCtrl)
		pCtrl->free(pCtrl);

	free(pM);
}

virtualI2cCtrlM_t* virtualI2cCtrlMMalloc()
{
	virtualI2cCtrlM_t* pM = NULL;

	do
	{
		pM = (virtualI2cCtrlM_t *)calloc(1, sizeof(virtualI2cCtrlM_t));
		if (NULL == pM)
			break;

		pM->id = (VIR_I2C_CTRL_ID)virtualI2cCtrlMalloc();
		if (0 == pM->id)
			break;

		pM->setFreq = virtualI2cCtrlMSetFreq;
		pM->setGpio = virtualI2cCtrlMSetGpio;
		pM->write = virtualI2cCtrlMWrite;
		pM->read = virtualI2cCtrlMRead;
		pM->free = virtualI2cCtrlMFree;

		return pM;
	}while (0);

	if (pM)
		free(pM);
	return NULL;
}
