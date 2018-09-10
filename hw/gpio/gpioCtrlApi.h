/*
 * gpioCtrlApi.h
 *
 *  Created on: 2018-7-10
 *      Author: tys
 */

#ifndef GPIOCTRLAPI_H_
#define GPIOCTRLAPI_H_

#ifdef __cplusplus
extern "C" {
#endif

struct gpioCtrlM;
typedef unsigned int GPIO_CTRL_ID;

typedef int (*_gpioCtrlMSetDir)(struct gpioCtrlM* pCtrl, unsigned int pin, unsigned int dir);
typedef int (*_gpioCtrlMEnable)(struct gpioCtrlM* pCtrl, unsigned int pin, unsigned int enable);
typedef int (*_gpioCtrlMWrite)(struct gpioCtrlM* pCtrl, unsigned int pin, unsigned int value);
typedef int (*_gpioCtrlMRead)(struct gpioCtrlM* pCtrl, unsigned int pin, unsigned int* pValue);
typedef void (*_gpioCtrlMFree)(struct gpioCtrlM* pCtrl);

typedef struct gpioCtrlM
{
	GPIO_CTRL_ID id;
	_gpioCtrlMSetDir setDir;
	_gpioCtrlMEnable enable;
	_gpioCtrlMWrite write;
	_gpioCtrlMRead read;
	_gpioCtrlMFree free;
}gpioCtrlM_t;

gpioCtrlM_t* gpioCtrlMMalloc();

#ifdef __cplusplus
}
#endif

#endif /* GPIOCTRLAPI_H_ */
