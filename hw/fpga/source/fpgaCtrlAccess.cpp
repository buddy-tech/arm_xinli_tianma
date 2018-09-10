/*
 * fpgaCtrlAccess.cpp
 *
 *  Created on: 2018-8-19
 *      Author: tys
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "common.h"
#include "fpgaCtrlAccess.h"

static fpgaCtrlAccess_t* pSgFpgaCtrlAccess = NULL;

void fpgaCtrlAccessWrite16Bits(unsigned int addr, unsigned short value)
{
	pSgFpgaCtrlAccess->mutex->semTake(pSgFpgaCtrlAccess->mutex);
	*(volatile unsigned short *)(pSgFpgaCtrlAccess->pBaseAddr + addr) = value;
	pSgFpgaCtrlAccess->mutex->semGive(pSgFpgaCtrlAccess->mutex);
}

unsigned short fpgaCtrlAccessRead16Bits(unsigned int addr)
{
	pSgFpgaCtrlAccess->mutex->semTake(pSgFpgaCtrlAccess->mutex);
	unsigned short value = *(volatile unsigned short *)(pSgFpgaCtrlAccess->pBaseAddr + addr);
	pSgFpgaCtrlAccess->mutex->semGive(pSgFpgaCtrlAccess->mutex);
	return value;
}

void fpgaCtrlAccessWrite(unsigned int addr, unsigned int value)
{
	pSgFpgaCtrlAccess->mutex->semTake(pSgFpgaCtrlAccess->mutex);
	*(volatile unsigned int *)(pSgFpgaCtrlAccess->pBaseAddr + 2*addr) = value;
	pSgFpgaCtrlAccess->mutex->semGive(pSgFpgaCtrlAccess->mutex);
}

unsigned int fpgaCtrlAccessRead(unsigned int addr)
{
	pSgFpgaCtrlAccess->mutex->semTake(pSgFpgaCtrlAccess->mutex);
	unsigned int value = *(volatile unsigned int *)(pSgFpgaCtrlAccess->pBaseAddr + 2*addr);
	pSgFpgaCtrlAccess->mutex->semGive(pSgFpgaCtrlAccess->mutex);
	return value;
}

int fpgaCtrlAccessBurstWrite(void* pData, int length)
{
	int ctrl = 0;
	int writeBytes = write(pSgFpgaCtrlAccess->fd, pData, length);
//	ioctl(pSgFpgaCtrlAccess->fd, 1, &ctrl);
	return writeBytes;
}

int fpgaCtrlAccessInit()
{
	pSgFpgaCtrlAccess = NULL;
	do
	{
		pSgFpgaCtrlAccess = (fpgaCtrlAccess_t *)calloc(1, sizeof(fpgaCtrlAccess_t));
		if (NULL == pSgFpgaCtrlAccess)
			break;

		pSgFpgaCtrlAccess->mutex = semMMalloc();
		if (NULL == pSgFpgaCtrlAccess->mutex)
			break;

		pSgFpgaCtrlAccess->fd = open(FPGA_CTRL_DEV, O_RDWR);
		if (pSgFpgaCtrlAccess->fd < 0)
			break;

		int fd = open("/dev/mem", O_RDWR);
		if (fd < 0)
			break;

		pSgFpgaCtrlAccess->pBaseAddr = (unsigned short *)mmap(0, FPGA_PHY_ADDR_SIZE, PROT_READ | PROT_WRITE,
				MAP_SHARED, fd, FPGA_PHY_ADDR_BASE);
		if (NULL == pSgFpgaCtrlAccess->pBaseAddr)
			break;

		return 0;
	}while (0);

	if (pSgFpgaCtrlAccess)
	{
		if (pSgFpgaCtrlAccess->fd >= 0)
			close(pSgFpgaCtrlAccess->fd);

		if (pSgFpgaCtrlAccess->mutex)
			pSgFpgaCtrlAccess->mutex->semFree(pSgFpgaCtrlAccess->mutex);

		if (pSgFpgaCtrlAccess->pBaseAddr)
			munmap(pSgFpgaCtrlAccess->pBaseAddr, FPGA_PHY_ADDR_SIZE);
	}

	DBG_ERR("%s failed\r\n", __FUNCTION__);
	return -1;
}

void fpgaCtrlAccessDeinit()
{
	if (pSgFpgaCtrlAccess)
	{
		if (pSgFpgaCtrlAccess->fd >= 0)
			close(pSgFpgaCtrlAccess->fd);

		if (pSgFpgaCtrlAccess->mutex)
			pSgFpgaCtrlAccess->mutex->semFree(pSgFpgaCtrlAccess->mutex);

		if (pSgFpgaCtrlAccess->pBaseAddr)
			munmap(pSgFpgaCtrlAccess->pBaseAddr, FPGA_PHY_ADDR_SIZE);

		free(pSgFpgaCtrlAccess);
	}
}
