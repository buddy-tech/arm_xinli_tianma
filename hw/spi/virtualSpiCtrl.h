/*
 * virtualSpiCtrl.h
 *
 *  Created on: 2018-7-9
 *      Author: tys
 */

#ifndef VIRTUALSPICTRL_H_
#define VIRTUALSPICTRL_H_

#ifdef __cplusplus
extern "C" {
#endif

struct virtualSpiCtrl;

typedef int (*_virtualSpiCtrlWrite)(struct virtualSpiCtrl* pCtrl, unsigned int value);	//spi control write data
typedef int (*_virtualSpiCtrlRead)(struct virtualSpiCtrl* pCtrl, unsigned int* pValue);	//spi control read data
typedef void (*_virtualSpiCtrlFree)(struct virtualSpiCtrl* pCtrl);	//free the control
typedef void (*_virtualSpiCtrlSetFreq)(struct virtualSpiCtrl* pCtrl, unsigned int freq);
typedef void (*_virtualSpiCtrlSetEdge)(struct virtualSpiCtrl* pCtrl, unsigned int edge);
typedef void (*_virtualSpiCtrlSetBits)(struct virtualSpiCtrl* pCtrl, unsigned int bits);
typedef void (*_virtualSpiCtrlSetGpio)(struct virtualSpiCtrl* pCtrl, unsigned int writeGpio, unsigned int readGpio,
		unsigned int csGpio, unsigned int clkGpio, unsigned int writeDirGpio);
typedef void (*_virtualSpiCtrlSetCsState)(struct virtualSpiCtrl* pCtrl, unsigned int value);
typedef void (*_virtualSpiCtrlSetByteFormat)(struct virtualSpiCtrl* pCtrl, unsigned int value);

typedef struct virtualSpiCtrl
{
	unsigned int freq;	// frequency for the spi controller
	unsigned int edge;	// the sample data edge
	unsigned int writeGpio;
	unsigned int writeDirGpio;	//it is used to control the output driver direction
	unsigned int readGpio;
	unsigned int csGpio;
	unsigned int clkGpio;

	unsigned int byteFormat;	// the bits data for writing at once time.
	_virtualSpiCtrlSetFreq setFreq;	//The max Freq is 1MHz
	_virtualSpiCtrlSetEdge setEdge;
	_virtualSpiCtrlSetGpio setGpio;
	_virtualSpiCtrlWrite write;
	_virtualSpiCtrlRead read;
	_virtualSpiCtrlSetCsState setCsState;
	_virtualSpiCtrlSetByteFormat setByteFormat;
	_virtualSpiCtrlFree free;
}virtualSpiCtrl_t;

virtualSpiCtrl_t* virtualSpiCtrlMalloc();

#ifdef __cplusplus
}
#endif
#endif /* VIRTUALSPICTRL_H_ */
