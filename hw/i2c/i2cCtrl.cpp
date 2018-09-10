/*
 * i2cCtrl.cpp
 *
 *  Created on: 2018-7-12
 *      Author: tys
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "i2cCtrl.h"
#include "i2cCtrlApi.h"

static int i2cCtrlWrite(struct i2cCtrl* pCtrl, unsigned char slaveAddr,
		unsigned char regAddr, unsigned char data)
{
	pthread_mutex_lock(&pCtrl->mutex);
    unsigned short outData = 0;
    SET_REG_ADDR(outData, regAddr);
    SET_REG_VAL(outData, data);

    ioctl(pCtrl->fd, I2C_SLAVE, slaveAddr);	// set the slave address
    ioctl(pCtrl->fd, I2C_TIMEOUT, 2);
    ioctl(pCtrl->fd, I2C_RETRIES, 1);

    int writeBytes = write(pCtrl->fd, &outData, sizeof(outData));
    pthread_mutex_unlock(&pCtrl->mutex);
    return writeBytes;
}

static int i2cCtrlRead(struct i2cCtrl* pCtrl, unsigned char slaveAddr, unsigned char regAddr)
{
	pthread_mutex_lock(&pCtrl->mutex);

	unsigned char value = regAddr;
	struct i2c_rdwr_ioctl_data data;
	data.nmsgs = 2;
	data.msgs = (struct i2c_msg *)calloc(1, data.nmsgs * sizeof(struct i2c_msg));

	data.msgs[0].len = 1;
	data.msgs[0].addr  = slaveAddr;
	data.msgs[0].flags = 0;
	data.msgs[0].buf = &value;

	data.msgs[1].len = 1;
	data.msgs[1].addr = slaveAddr;
	data.msgs[1].flags = I2C_M_RD;
	data.msgs[1].buf = &value;

    ioctl(pCtrl->fd, I2C_TIMEOUT, 2);
    ioctl(pCtrl->fd, I2C_RETRIES, 1);
	ioctl (pCtrl->fd, I2C_RDWR, (unsigned long)&data);
	free(data.msgs);

	pthread_mutex_unlock(&pCtrl->mutex);
	return value;
}

static int i2cCtrlBurstWrite(struct i2cCtrl* pCtrl, unsigned char slaveAddr,
		unsigned char regAddr, void* pData, unsigned int length)
{
	if (NULL == pData || length == 0)
		return -1;

	unsigned int outLen = length + 1;
	unsigned char* pOut = (unsigned char*)calloc(1, outLen);
	if (NULL == pOut)
		return -1;

	pthread_mutex_lock(&pCtrl->mutex);
	*pOut = regAddr;
	memcpy(pOut+1, pData, length);

    ioctl(pCtrl->fd, I2C_TIMEOUT, 2);
    ioctl(pCtrl->fd, I2C_RETRIES, 1);

    ioctl(pCtrl->fd, I2C_SLAVE, slaveAddr);	// set the slave address
    int writeBytes = write(pCtrl->fd, pOut, outLen);
    free(pOut);

    pthread_mutex_unlock(&pCtrl->mutex);
    return writeBytes;
}

static int i2cCtrlBurstRead(struct i2cCtrl* pCtrl, unsigned char slaveAddr,
		unsigned char regAddr, void* pData, unsigned int length)
{
	return 0;
}

static void i2cCtrlFree(struct i2cCtrl* pCtrl)
{
	pthread_mutex_unlock(&pCtrl->mutex);
	pthread_mutex_lock(&pCtrl->mutex);
	pthread_mutex_destroy(&pCtrl->mutex);
	close(pCtrl->fd);
	free(pCtrl);
}

i2cCtrl_t* i2cCtrlMalloc(const char* devName)
{
	i2cCtrl_t* pCtrl = NULL;

	do
	{
		if (NULL == devName)
			break;

		pCtrl = (i2cCtrl_t *)calloc(1, sizeof(i2cCtrl_t));
		if (NULL == pCtrl)
			break;

		pCtrl->fd = open(devName, O_RDWR);
		if (pCtrl->fd < 0)
			break;

		pthread_mutex_init(&pCtrl->mutex, NULL);

		pCtrl->free = i2cCtrlFree;
		pCtrl->write = i2cCtrlWrite;
		pCtrl->read = i2cCtrlRead;
		pCtrl->burstWrite = i2cCtrlBurstWrite;
		pCtrl->burstRead = i2cCtrlBurstRead;
		return pCtrl;
	}while (0);

	if (pCtrl)
		free(pCtrl);

	return pCtrl;
}

static int i2cCtrlMWrite(struct i2cCtrlM* pM, unsigned char slaveAddr,
		unsigned char regAddr, unsigned char data)
{
	i2cCtrl_t* pCtrl = (i2cCtrl_t*)pM->id;
	return pCtrl->write(pCtrl, slaveAddr, regAddr, data);
}

static int i2cCtrlMRead(struct i2cCtrlM* pM, unsigned char slaveAddr, unsigned char regAddr)
{
	i2cCtrl_t* pCtrl = (i2cCtrl_t*)pM->id;
	return pCtrl->read(pCtrl, slaveAddr, regAddr);
}

static int i2cCtrlMBurstWrite(struct i2cCtrlM* pM, unsigned char slaveAddr,
		unsigned char regAddr, void* pData, unsigned int length)
{
	i2cCtrl_t* pCtrl = (i2cCtrl_t*)pM->id;
	return pCtrl->burstWrite(pCtrl, slaveAddr, regAddr, pData, length);
}

static int i2cCtrlMBurstRead(struct i2cCtrlM* pM, unsigned char slaveAddr,
		unsigned char regAddr, void* pData, unsigned int length)
{
	i2cCtrl_t* pCtrl = (i2cCtrl_t*)pM->id;
	return pCtrl->burstRead(pCtrl, slaveAddr, regAddr, pData, length);
}

static void i2cCtrlMFree(struct i2cCtrlM* pM)
{
	i2cCtrl_t* pCtrl = (i2cCtrl_t*)pM->id;
	pCtrl->free(pCtrl);
	free(pM);
}

i2cCtrlM_t* i2cCtrlMMalloc(const char* devName)
{
	i2cCtrlM_t* pM = NULL;
	do
	{
		if (NULL == devName)
			break;

		pM = (i2cCtrlM_t *)calloc(1, sizeof(i2cCtrlM_t));
		if (NULL == pM)
			break;

		pM->id = (I2C_CTRL_ID)i2cCtrlMalloc(devName);
		if (0 == pM->id)
			break;

		pM->free = i2cCtrlMFree;
		pM->write = i2cCtrlMWrite;
		pM->read = i2cCtrlMRead;
		pM->burstWrite = i2cCtrlMBurstWrite;
		pM->burstRead = i2cCtrlMBurstRead;
		return pM;
	}while (0);

	if (pM)
		free(pM);

	return NULL;
}

