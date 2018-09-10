/*
 * i2cCtrl.h
 *
 *  Created on: 2018-7-12
 *      Author: tys
 */

#ifndef I2CCTRL_H_
#define I2CCTRL_H_

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SET_FORMAT_BIT(_data, _val, _mask, _move)	((_data) = (((_data) & (~((_mask) << (_move)))) | (((_val) & (_mask)) << (_move))))
#define SET_REG_ADDR(_data, _reg) SET_FORMAT_BIT(_data, _reg, 0xFF, 0)
#define SET_REG_VAL(_data, _val) SET_FORMAT_BIT(_data, _val, 0xFF, 8)

struct i2cCtrl;

typedef int (*_i2cCtrlWrite)(struct i2cCtrl* pCtrl, unsigned char slaveAddr,
		unsigned char regAddr, unsigned char data);
typedef int (*_i2cCtrlRead)(struct i2cCtrl* pCtrl, unsigned char slaveAddr, unsigned char regAddr);
typedef int (*_i2cCtrlBurstWrite)(struct i2cCtrl* pCtrl, unsigned char slaveAddr,
		unsigned char regAddr, void* pData, unsigned int length);
typedef int (*_i2cCtrlBurstRead)(struct i2cCtrl* pCtrl, unsigned char slaveAddr,
		unsigned char regAddr, void* pData, unsigned int length);
typedef void (*_i2cCtrlFree)(struct i2cCtrl* pCtrl);

typedef struct i2cCtrl
{
	int fd;	//i2c device descriptor
	pthread_mutex_t mutex;
	_i2cCtrlWrite write;
	_i2cCtrlRead read;
	_i2cCtrlBurstWrite burstWrite;
	_i2cCtrlBurstRead burstRead;
	_i2cCtrlFree free;
}i2cCtrl_t;

i2cCtrl_t* i2cCtrlMalloc(const char* devName);

#ifdef __cplusplus
}
#endif

#endif /* I2CCTRL_H_ */
