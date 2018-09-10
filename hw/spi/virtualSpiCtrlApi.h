/*
 * virtualSpiCtrlApi.h
 *
 * description: It is the virtual spi control object which
 * is implemented with Gpio bins. It supports four-wire and
 * three-wire mode. In three-wire mode, the read gpio should
 * be set as write gpio, and the direction gpio is used to
 * changed the hardware driver direction, which will need use
 * setGpios functions. And in four-wire mode, you need to
 * input the three gpio pin, which will need use setGpios
 * functions.
 *
 *  Created on: 2018-7-9
 *      Author: tys
 */

#ifndef VIRTUALSPICTRLAPI_H_
#define VIRTUALSPICTRLAPI_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	EDGE_RISE,
	EDGE_DOWN,
}spiEdge_e;

typedef enum
{
	BYTE_1_BIT = 1,	//one bit for byte
	BYTE_2_BIT,
	BYTE_3_BIT,
	BYTE_4_BIT,
	BYTE_5_BIT,
	BYTE_6_BIT,
	BYTE_7_BIT,
	BYTE_8_BIT,
	BYTE_9_BIT,
	BYTE_10_BIT,
	BYTE_11_BIT,
	BYTE_12_BIT,
	BYTE_13_BIT,
	BYTE_14_BIT,
	BYTE_15_BIT,
	BYTE_16_BIT,
	BYTE_17_BIT,
	BYTE_18_BIT,
	BYTE_19_BIT,
	BYTE_20_BIT,
	BYTE_21_BIT,
	BYTE_22_BIT,
	BYTE_23_BIT,
	BYTE_24_BIT,
	BYTE_25_BIT,
	BYTE_26_BIT,
	BYTE_27_BIT,
	BYTE_28_BIT,
	BYTE_29_BIT,
	BYTE_30_BIT,
	BYTE_31_BIT,
	BYTE_32_BIT,
}spiByteFormat_e;

typedef unsigned int VIR_SPI_CTRL_ID;
struct virtualSpiCtrlM;

typedef int (*_virtualSpiCtrlMWrite)(struct virtualSpiCtrlM* pCtrl, unsigned int value);	//spi control write data
typedef int (*_virtualSpiCtrlMRead)(struct virtualSpiCtrlM* pCtrl, unsigned int* pValue);	//spi control read data
typedef void (*_virtualSpiCtrlMFree)(struct virtualSpiCtrlM* pCtrl);	//free the control
typedef void (*_virtualSpiCtrlMSetFreq)(struct virtualSpiCtrlM* pCtrl, unsigned int freq);
typedef void (*_virtualSpiCtrlMSetEdge)(struct virtualSpiCtrlM* pCtrl, spiEdge_e edge);
typedef void (*_virtualSpiCtrlMSetCsState)(struct virtualSpiCtrlM* pCtrl, unsigned int value);
typedef void (*_virtualSpiCtrlMSetGpio)(struct virtualSpiCtrlM* pCtrl, unsigned int writeGpio, unsigned int readGpio,
		unsigned int csGpio, unsigned int clkGpio, unsigned int writeDirGpio);
typedef void (*_virtualSpiCtrlMSetByteFormat)(struct virtualSpiCtrlM* pCtrl, spiByteFormat_e format);

typedef struct virtualSpiCtrlM
{
	VIR_SPI_CTRL_ID id;
	_virtualSpiCtrlMSetFreq setFreq;	//The max Freq is 500KHz whick will be 400KHz
	_virtualSpiCtrlMSetEdge setEdge;
	_virtualSpiCtrlMSetGpio setGpio;
	_virtualSpiCtrlMSetCsState setCsState;
	_virtualSpiCtrlMSetByteFormat setByteFormat;
	_virtualSpiCtrlMWrite write;
	_virtualSpiCtrlMRead read;
	_virtualSpiCtrlMFree free;
}virtualSpiCtrlM_t;

virtualSpiCtrlM_t* virtualSpiCtrlMMalloc();
#ifdef __cplusplus
}
#endif

#endif /* VIRTUALSPICTRLAPI_H_ */
