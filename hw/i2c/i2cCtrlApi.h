/*
 * i2cCtrlApi.h
 *
 *  Created on: 2018-7-12
 *      Author: tys
 */

#ifndef I2CCTRLAPI_H_
#define I2CCTRLAPI_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int I2C_CTRL_ID;

struct i2cCtrlM;

typedef int (*_i2cCtrlMWrite)(struct i2cCtrlM* pM, unsigned char slaveAddr,
		unsigned char regAddr, unsigned char data);
typedef int (*_i2cCtrlMRead)(struct i2cCtrlM* pM, unsigned char slaveAddr, unsigned char regAddr);
typedef int (*_i2cCtrlMBurstWrite)(struct i2cCtrlM* pM, unsigned char slaveAddr,
		unsigned char regAddr, void* pData, unsigned int length);
typedef int (*_i2cCtrlMBurstRead)(struct i2cCtrlM* pM, unsigned char slaveAddr,
		unsigned char regAddr, void* pData, unsigned int length);
typedef void (*_i2cCtrlMFree)(struct i2cCtrlM* pM);

typedef struct i2cCtrlM
{
	I2C_CTRL_ID id;
	_i2cCtrlMWrite write;
	_i2cCtrlMRead read;
	_i2cCtrlMBurstWrite burstWrite;
	_i2cCtrlMBurstRead burstRead;
	_i2cCtrlMFree free;
}i2cCtrlM_t;

i2cCtrlM_t* i2cCtrlMMalloc(const char* devName);

#ifdef __cplusplus
}
#endif

#endif /* I2CCTRLAPI_H_ */
