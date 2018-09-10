/*
 * virtualI2cCtrlApi.h
 * description: It is the virtual spi control object which
 * is implemented with Gpio bins. It supports four-wire and
 * three-wire mode. In three-wire mode, the read gpio should
 * be set as write gpio, and the direction gpio is used to
 * changed the hardware driver direction, which will need use
 * setGpios functions. And in four-wire mode, you need to
 * input the three gpio pin, which will need use setGpios
 * functions.
 *  Created on: 2018-7-13
 *      Author: tys
 */

#ifndef VIRTUALI2CCTRLAPI_H_
#define VIRTUALI2CCTRLAPI_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    I2C_FREQ_10K = 0,
    I2C_FREQ_25K,
    I2C_FREQ_50K,
    I2C_FREQ_100K,
    I2C_FREQ_200K,
    I2C_FREQ_250K,
    I2C_FREQ_BUTT
}i2cFreq_e;

typedef unsigned int VIR_I2C_CTRL_ID;
struct virtualI2cCtrlM;

typedef int (*_virtualI2cCtrlMWrite)(struct virtualI2cCtrlM* pCtrl, unsigned char slaveAddr, unsigned char regAddr, unsigned char value);	//i2c control write data
typedef int (*_virtualI2cCtrlMRead)(struct virtualI2cCtrlM* pCtrl, unsigned char slaveAddr, unsigned char regAddr, unsigned char* pValue);	//i2c control read data
typedef void (*_virtualI2cCtrlMFree)(struct virtualI2cCtrlM* pCtrl);	//free the control
typedef void (*_virtualI2cCtrlMSetFreq)(struct virtualI2cCtrlM* pCtrl, i2cFreq_e freq);
typedef void (*_virtualI2cCtrlMSetGpio)(struct virtualI2cCtrlM* pCtrl, unsigned int sdaGpio, unsigned int sclGpio, unsigned int sdaDirGpio);

typedef struct virtualI2cCtrlM
{
	VIR_I2C_CTRL_ID id;
	_virtualI2cCtrlMSetFreq setFreq;	//The max Freq is 400KHz
	_virtualI2cCtrlMSetGpio setGpio;
	_virtualI2cCtrlMWrite write;
	_virtualI2cCtrlMRead read;
	_virtualI2cCtrlMFree free;
}virtualI2cCtrlM_t;

virtualI2cCtrlM_t* virtualI2cCtrlMMalloc();
#ifdef __cplusplus
}
#endif

#endif /* VIRTUALI2CCTRLAPI_H_ */
