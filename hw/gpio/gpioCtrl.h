/*
 * gpio.h
 *
 *  Created on: 2018-7-10
 *      Author: tys
 */

#ifndef GPIO_CTRL_H_
#define GPIO_CTRL_H_

#ifdef __cplusplus
extern "C" {
#endif

#define GPIO_PHY_BASE_ADDR	(0xE000A000)
#define GPIO_PHY_PAGE_SIZE	(0x2E4)

#define GPIO_MASK_LSW_OFFSET(_num)		(0x00 + (_num)*0x08)	//0, 1, 2, 3
#define GPIO_MASK_MSW_OFFSET(_num)		(0x04 + (_num)*0x08)	//0, 1, 2, 3

#define GPIO_BANK_WR_OFFSET(_num)	(0x40 + (_num)*0x04)	//0, 1, 2, 3
#define GPIO_BANK_RO_OFFSET(_num)	(0x60 + (_num)*0x04)	//0, 1, 2, 3

#define GPIO_BANK_DIR_OFFSET(_num)	(0x204 + (_num)*0x40)	//0, 1, 2, 3
#define GPIO_BANK_EN_OFFSET(_num)	(0x208 + (_num)*0x40)	//0, 1, 2, 3

#define GPIO_SET_MASK_BIT_EN(_offset)	(((~(0x1 << (_offset))) & 0xFFFF) << 16)
#define GPIO_SET_MASK_BIT_VALUE(_offset, _val)	((_val) << (_offset))
#define GPIO_SET_MASK_VALUE(_offset, _val) (GPIO_SET_MASK_BIT_EN(_offset) | GPIO_SET_MASK_BIT_VALUE(_offset, _val))

typedef struct gpioInfo
{
	int bank;
	int offset;
}gpioInfo_t;

struct gpioCtrl;

typedef int (*_gpioCtrlSetDir)(struct gpioCtrl* pCtrl, unsigned int pin, unsigned int dir);
typedef int (*_gpioCtrlEnable)(struct gpioCtrl* pCtrl, unsigned int pin, unsigned int enable);
typedef int (*_gpioCtrlWrite)(struct gpioCtrl* pCtrl, unsigned int pin, unsigned int value);
typedef int (*_gpioCtrlRead)(struct gpioCtrl* pCtrl, unsigned int pin, unsigned int* pValue);
typedef void (*_gpioCtrlFree)(struct gpioCtrl* pCtrl);

typedef struct gpioCtrl
{
	int fd;
	void* pBaseAddr;
	_gpioCtrlSetDir setDir;
	_gpioCtrlEnable enable;
	_gpioCtrlWrite write;
	_gpioCtrlRead read;
	_gpioCtrlFree free;
}gpioCtrl_t;

gpioCtrl_t* gpioCtrlMalloc();

#ifdef __cplusplus
}
#endif

#endif /* GPIO_CTRL_H_ */
