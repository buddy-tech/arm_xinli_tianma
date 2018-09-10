/*
 * virtualI2cCtrl.h
 *
 *  Created on: 2018-7-13
 *      Author: tys
 */

#ifndef VIRTUALI2CCTRL_H_
#define VIRTUALI2CCTRL_H_
#ifdef __cplusplus
extern "C" {
#endif

#define I2C_WRITE_MODE	(0)
#define I2C_READ_MDOE	(1)
#define I2C_NOACK		(1)
#define I2C_ACK			(0)

struct virtualI2cCtrl;

typedef int (*_virtualI2cCtrlWrite)(struct virtualI2cCtrl* pCtrl, unsigned char slaveAddr, unsigned char regAddr, unsigned char value);	//i2c control write data
typedef int (*_virtualI2cCtrlRead)(struct virtualI2cCtrl* pCtrl, unsigned char slaveAddr, unsigned char regAddr, unsigned char* pValue);	//i2c control read data
typedef void (*_virtualI2cCtrlFree)(struct virtualI2cCtrl* pCtrl);	//free the control
typedef void (*_virtualI2cCtrlSetFreq)(struct virtualI2cCtrl* pCtrl, unsigned int freq);
typedef void (*_virtualI2cCtrlSetGpio)(struct virtualI2cCtrl* pCtrl, unsigned int sdaGpio, unsigned int sclGpio, unsigned int sdaDirGpio);

typedef struct virtualI2cCtrl
{
	unsigned int period;	// frequency for the i2c controller
	unsigned int sdaGpio;
	unsigned int sdaDirGpio;	//it is used to control the output driver direction
	unsigned int clkGpio;

	_virtualI2cCtrlSetFreq setFreq;	//The max Freq is 400KHz
	_virtualI2cCtrlSetGpio setGpio;
	_virtualI2cCtrlWrite write;
	_virtualI2cCtrlRead read;
	_virtualI2cCtrlFree free;
}virtualI2cCtrl_t;

virtualI2cCtrl_t* virtualI2cCtrlMalloc();

#ifdef __cplusplus
}
#endif
#endif /* VIRTUALI2CCTRL_H_ */
