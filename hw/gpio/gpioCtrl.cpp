/*
 * gpioCtrl.cpp
 *
 *  Created on: 2018-7-10
 *      Author: tys
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "gpioCtrl.h"
#include "gpioCtrlApi.h"

static gpioInfo_t gpioCtrlInfo(unsigned int pin)
{
	gpioInfo_t info;
	info.bank = -1;
	info.offset = -1;

	if (pin <= 31) //bank 0
	{
		info.bank = 0;
		info.offset = pin;
	}
	if (pin > 31 && pin <= 53)	//bank 1
	{
		info.bank = 1;
		info.offset = pin - 32;
	}
	else if (pin > 53 && pin <= 85) //bank 2
	{
		info.bank = 2;
		info.offset = pin - 54;
	}
	else if (pin > 85 && pin <= 117) //bank 3
	{
		info.bank = 3;
		info.offset = pin - 86;
	}

	return info;
}

static int gpioCtrlWrite(struct gpioCtrl* pCtrl, unsigned int pin, unsigned int value)
{
	gpioInfo_t  info = gpioCtrlInfo(pin);
	if (info.bank < 0)
		return -1;

	volatile unsigned int* pMaskAddr = NULL;
	unsigned int offset = 0;

	if (info.offset < 16)
	{
		offset = info.offset;
		pMaskAddr = (volatile unsigned int *)((char *)pCtrl->pBaseAddr + GPIO_MASK_LSW_OFFSET(info.bank));
	}
	else
	{
		offset = info.offset - 16;
		pMaskAddr = (volatile unsigned int *)((char *)pCtrl->pBaseAddr + GPIO_MASK_MSW_OFFSET(info.bank));
	}

	*(volatile unsigned int *)pMaskAddr = GPIO_SET_MASK_VALUE(offset, value&0x1);
	return 0;
}

static int gpioCtrlRead(struct gpioCtrl* pCtrl, unsigned int pin, unsigned int* pValue)
{
	gpioInfo_t info = gpioCtrlInfo(pin);
	if (info.bank < 0)
		return -1;

	unsigned int value = *(volatile unsigned int *)((char *)pCtrl->pBaseAddr + GPIO_BANK_RO_OFFSET(info.bank));
	*pValue = (value >> info.offset) & 0x1;

	return 0;
}

static int gpioCtrlSetDir(struct gpioCtrl* pCtrl, unsigned int pin, unsigned int dir)
{
	gpioInfo_t  info = gpioCtrlInfo(pin);
	if (info.bank < 0)
		return -1;

	volatile unsigned int* pRegAddr = (volatile unsigned int *)((char *)pCtrl->pBaseAddr + GPIO_BANK_DIR_OFFSET(info.bank));
	unsigned int regValue = *(volatile unsigned int *)pRegAddr;
	regValue = (regValue & (~(0x1 << info.offset))) | ((dir & 0x1) << info.offset);

	*(volatile unsigned int *)pRegAddr = regValue;
	return 0;
}

static int gpioCtrlEnable(struct gpioCtrl* pCtrl, unsigned int pin, unsigned int enable)
{
	gpioInfo_t  info = gpioCtrlInfo(pin);
	if (info.bank < 0)
		return -1;

	volatile unsigned int* pRegAddr = (volatile unsigned int *)((char *)pCtrl->pBaseAddr + GPIO_BANK_EN_OFFSET(info.bank));
	unsigned int regValue = *(volatile unsigned int *)pRegAddr;
	regValue = (regValue & (~(0x1 << info.offset))) | ((enable & 0x1) << info.offset);

	*(volatile unsigned int *)pRegAddr = regValue;
	return 0;
}

static void gpioCtrlFree(struct gpioCtrl* pCtrl)
{
	if (pCtrl->pBaseAddr)
		munmap(pCtrl->pBaseAddr, GPIO_PHY_PAGE_SIZE);

	close(pCtrl->fd);
	free(pCtrl);
}

gpioCtrl_t* gpioCtrlMalloc()
{
	gpioCtrl_t* pCtrl = NULL;
	do
	{
		pCtrl = (gpioCtrl_t *)calloc(1, sizeof(gpioCtrl_t));
		if (NULL == pCtrl)
			break;

		pCtrl->fd = open("/dev/mem", O_RDWR);
		if (pCtrl->fd < 0)
			break;

		pCtrl->pBaseAddr = mmap(NULL, GPIO_PHY_PAGE_SIZE, PROT_READ | PROT_WRITE,
				MAP_SHARED, pCtrl->fd, GPIO_PHY_BASE_ADDR);
		if (NULL == pCtrl->pBaseAddr)
			break;

		pCtrl->setDir = gpioCtrlSetDir;
		pCtrl->enable = gpioCtrlEnable;
		pCtrl->write = gpioCtrlWrite;
		pCtrl->read = gpioCtrlRead;
		pCtrl->free = gpioCtrlFree;
		return pCtrl;
	}while (0);

	if (pCtrl)
	{
		if (pCtrl->fd >= 0)
			close(pCtrl->fd);

		free(pCtrl);
	}

	return NULL;
}

static int gpioCtrlMWrite(struct gpioCtrlM* pM, unsigned int pin, unsigned int value)
{
	gpioCtrl_t* pCtrl = (gpioCtrl_t *)pM->id;
	return pCtrl->write(pCtrl, pin, value);
}

static int gpioCtrlMRead(struct gpioCtrlM* pM, unsigned int pin, unsigned int* pValue)
{
	gpioCtrl_t* pCtrl = (gpioCtrl_t *)pM->id;
	return pCtrl->read(pCtrl, pin, pValue);
}

static int gpioCtrlMSetDir(struct gpioCtrlM* pM, unsigned int pin, unsigned int dir)
{
	gpioCtrl_t* pCtrl = (gpioCtrl_t *)pM->id;
	return pCtrl->setDir(pCtrl, pin, dir);
}

static int gpioCtrlMEnable(struct gpioCtrlM* pM, unsigned int pin, unsigned int enable)
{
	gpioCtrl_t* pCtrl = (gpioCtrl_t *)pM->id;
	return pCtrl->enable(pCtrl, pin, enable);
}

static void gpioCtrlMFree(struct gpioCtrlM* pM)
{
	gpioCtrl_t* pCtrl = (gpioCtrl_t *)pM->id;
	pCtrl->free(pCtrl);

	free(pM);
}

gpioCtrlM_t* gpioCtrlMMalloc()
{
	gpioCtrlM_t* pM = NULL;
	do
	{
		pM = (gpioCtrlM_t *)calloc(1, sizeof(gpioCtrlM_t));
		if (NULL == pM)
			break;

		pM->id = (GPIO_CTRL_ID)gpioCtrlMalloc();
		if (pM->id == 0)
		{
			printf("************gpioCtrlMalloc failed***********\r\n");
			break;
		}

		pM->setDir = gpioCtrlMSetDir;
		pM->enable = gpioCtrlMEnable;
		pM->write = gpioCtrlMWrite;
		pM->read = gpioCtrlMRead;
		pM->free = gpioCtrlMFree;
		return pM;
	}while (0);

	if (pM)
		free(pM);

	return NULL;
}
