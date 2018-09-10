/*
 * virtualSpiCtrl.cpp
 *
 *  Created on: 2018-7-9
 *      Author: tys
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#include "virtualSpiCtrl.h"
#include "virtualSpiCtrlApi.h"
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

static void virtualSpiCtrlGpioInit(struct virtualSpiCtrl* pCtrl)
{
	HI_UNF_GPIO_SetDirBit(pCtrl->writeGpio, 0);
	HI_UNF_GPIO_SetDirBit(pCtrl->readGpio, 1);
	HI_UNF_GPIO_SetDirBit(pCtrl->clkGpio, 0);
	HI_UNF_GPIO_SetDirBit(pCtrl->csGpio, 0);

	gpio_set_output_value(pCtrl->csGpio, 1);
	gpio_set_output_value(pCtrl->clkGpio, 1);
	gpio_set_output_value(pCtrl->writeGpio, 1);
}

static void virtualSpiCtrlGpioDeinit(struct virtualSpiCtrl* pCtrl)
{
}

static int virtualSpiCtrlWrite(struct virtualSpiCtrl* pCtrl, unsigned int value)
{
	unsigned long timeGap = 1000*1000/pCtrl->freq;	//usecond

	_writeOneBit(pCtrl->writeDirGpio, 1);	// set direction as output

	for (int i = 0; i < pCtrl->byteFormat; i++)
	{
		unsigned int offset = pCtrl->byteFormat - i - 1;
		if (pCtrl->edge == EDGE_RISE)
		{
			_writeOneBit(pCtrl->clkGpio, 0);
			_writeOneBit(pCtrl->writeGpio, (value >> offset)&0x1);
			_usleep(timeGap/2);
			_writeOneBit(pCtrl->clkGpio, 1);
			_usleep(timeGap/2);
		}
		else
		{
			_writeOneBit(pCtrl->clkGpio, 1);
			_usleep(1);
			_writeOneBit(pCtrl->writeGpio, (value >> offset)&0x1);
			_writeOneBit(pCtrl->clkGpio, 0);
			_usleep(timeGap/2);
		}
	}

	return 0;
}

static int virtualSpiCtrlRead(struct virtualSpiCtrl* pCtrl, unsigned int* pValue)
{
	if (NULL == pValue)
		return -1;

	unsigned long timeGap = 1000*1000/pCtrl->freq;	//usecond

	if (pCtrl->writeGpio == pCtrl->readGpio)
	{
		_writeOneBit(pCtrl->writeGpio, 0);
		_writeOneBit(pCtrl->writeDirGpio, 0);	// set direction as input
	}

	for (int i = 0; i < pCtrl->byteFormat; i++)
	{
		unsigned int offset = (pCtrl->byteFormat - i - 1);
		_writeOneBit(pCtrl->clkGpio, 0);
		*pValue |= ((_readOneBit(pCtrl->readGpio) & 0x1 ) << offset);
		_usleep(timeGap/2);
		_writeOneBit(pCtrl->clkGpio, 1);
		_usleep(timeGap/2);
	}

	return 0;
}

static void virtualSpiCtrlSetFreq(struct virtualSpiCtrl* pCtrl, unsigned int freq)
{
	pCtrl->freq = freq;
}

static void virtualSpiCtrlSetEdge(struct virtualSpiCtrl* pCtrl, unsigned int edge)
{
	pCtrl->edge = edge;
}

static void virtualSpiCtrlSetGpio(struct virtualSpiCtrl* pCtrl, unsigned int writeGpio, unsigned int readGpio,
		unsigned int csGpio, unsigned int clkGpio, unsigned int writeDirGpio)
{
	pCtrl->writeGpio = writeGpio;
	pCtrl->readGpio = readGpio;
	pCtrl->csGpio = csGpio;
	pCtrl->clkGpio = clkGpio;
	pCtrl->writeDirGpio = writeDirGpio;
	virtualSpiCtrlGpioInit(pCtrl);
}

static void virtualSpiCtrlSetCsState(struct virtualSpiCtrl* pCtrl, unsigned int value)
{
	unsigned long timeGap = 1000*1000/pCtrl->freq;	//usecond
	_usleep(timeGap);
	_writeOneBit(pCtrl->csGpio, value&0x1);
	_usleep(timeGap);
}

void virtualSpiCtrlSetByteFormat(struct virtualSpiCtrl* pCtrl, unsigned int byteFormat)
{
	if (byteFormat < BYTE_1_BIT || byteFormat > BYTE_32_BIT)
		return;

	pCtrl->byteFormat = byteFormat;
}

static void virtualSpiCtrlFree(struct virtualSpiCtrl* pCtrl)
{
	virtualSpiCtrlGpioDeinit(pCtrl);
	free(pCtrl);
}

virtualSpiCtrl_t* virtualSpiCtrlMalloc()
{
	virtualSpiCtrl_t* pCtrl = NULL;

	do
	{
		pCtrl = (virtualSpiCtrl_t *)calloc(1, sizeof(virtualSpiCtrl_t));
		if (NULL == pCtrl)
			break;

		pCtrl->setFreq = virtualSpiCtrlSetFreq;
		pCtrl->setEdge = virtualSpiCtrlSetEdge;
		pCtrl->setGpio = virtualSpiCtrlSetGpio;
		pCtrl->setCsState = virtualSpiCtrlSetCsState;
		pCtrl->setByteFormat = virtualSpiCtrlSetByteFormat;
		pCtrl->write = virtualSpiCtrlWrite;
		pCtrl->read = virtualSpiCtrlRead;
		pCtrl->free = virtualSpiCtrlFree;

		pCtrl->freq = 1000*1000;
		pCtrl->edge = EDGE_RISE;
		pCtrl->byteFormat = BYTE_8_BIT;
	}while (0);

	return pCtrl;
}

static int virtualSpiCtrlMWrite(struct virtualSpiCtrlM* pM, unsigned int value)
{
	virtualSpiCtrl_t* pCtrl = (virtualSpiCtrl_t *)pM->id;

	return pCtrl->write(pCtrl, value);
}

static int virtualSpiCtrlMRead(struct virtualSpiCtrlM* pM, unsigned int* pValue)
{
	virtualSpiCtrl_t* pCtrl = (virtualSpiCtrl_t *)pM->id;

	return pCtrl->read(pCtrl, pValue);
}

static void virtualSpiCtrlMSetFreq(struct virtualSpiCtrlM* pM, unsigned int freq)
{
	virtualSpiCtrl_t* pCtrl = (virtualSpiCtrl_t *)pM->id;

	pCtrl->setFreq(pCtrl, freq);
}

static void virtualSpiCtrlMSetEdge(struct virtualSpiCtrlM* pM, spiEdge_e edge)
{
	virtualSpiCtrl_t* pCtrl = (virtualSpiCtrl_t *)pM->id;

	pCtrl->setEdge(pCtrl, edge);
}

static void virtualSpiCtrlMSetCsState(struct virtualSpiCtrlM* pM, unsigned int value)
{
	virtualSpiCtrl_t* pCtrl = (virtualSpiCtrl_t *)pM->id;

	pCtrl->setCsState(pCtrl, value);
}

static void virtualSpiCtrlMSetByteFormat(struct virtualSpiCtrlM* pM, spiByteFormat_e format)
{
	virtualSpiCtrl_t* pCtrl = (virtualSpiCtrl_t *)pM->id;
	pCtrl->setByteFormat(pCtrl, format);
}

static void virtualSpiCtrlMSetGpio(struct virtualSpiCtrlM* pM, unsigned int writeGpio, unsigned int readGpio,
		unsigned int csGpio, unsigned int clkGpio, unsigned int writeDirGpio)
{
	virtualSpiCtrl_t* pCtrl = (virtualSpiCtrl_t *)pM->id;

	pCtrl->setGpio(pCtrl, writeGpio, readGpio, csGpio, clkGpio, writeDirGpio);
}

static void virtualSpiCtrlMFree(struct virtualSpiCtrlM* pM)
{
	virtualSpiCtrl_t* pCtrl = (virtualSpiCtrl_t *)pM->id;
	if (pCtrl)
		pCtrl->free(pCtrl);

	free(pM);
}

virtualSpiCtrlM_t* virtualSpiCtrlMMalloc()
{
	virtualSpiCtrlM_t* pM = NULL;

	do
	{
		pM = (virtualSpiCtrlM_t *)calloc(1, sizeof(virtualSpiCtrlM_t));
		if (NULL == pM)
			break;

		pM->id = (VIR_SPI_CTRL_ID)virtualSpiCtrlMalloc();
		if (0 == pM->id)
			break;

		pM->setFreq = virtualSpiCtrlMSetFreq;
		pM->setEdge = virtualSpiCtrlMSetEdge;
		pM->setGpio = virtualSpiCtrlMSetGpio;
		pM->setCsState = virtualSpiCtrlMSetCsState;
		pM->setByteFormat = virtualSpiCtrlMSetByteFormat;
		pM->write = virtualSpiCtrlMWrite;
		pM->read = virtualSpiCtrlMRead;
		pM->free = virtualSpiCtrlMFree;

		return pM;
	}while (0);

	if (pM)
		free(pM);
	return NULL;
}
