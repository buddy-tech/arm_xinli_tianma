/*
 * scanDevApi.h
 *
 *  Created on: 2018-7-6
 *      Author: tys
 */

#ifndef SERIALDEVAPI_H_
#define SERIALDEVAPI_H_

#include <termios.h>
#include <unistd.h>

#ifndef __cplusplus
extern "C" {
#endif

typedef enum
{
	E_B2400,
	E_B4800,
	E_B9600,
	E_B57600,
	E_B115200,
}baudRate_e;

typedef enum
{
	E_S_1_BIT,
	E_S_2_BIT,
}stopBits_e;

typedef enum
{
	E_0_PARITY,	//none
	E_1_PARITY,	//even
	E_2_PARITY,	//odd
}parity_e;

typedef enum
{
	E_D_5_BIT,
	E_D_6_BIT,
	E_D_7_BIT,
	E_D_8_BIT,
}dataBits_e;

struct serialDevM;

typedef unsigned int SERIAL_DEV_ID;

/**************************************************
 *func: It is used to parse one pocket data.
 *
 *@pData: the data of the pocket which may be not equal
 *one pocket data.
 *
 *@length: the data length of the pData.
 *
 *Return: if it is a valid pocket, it will return the
 *length of the actual pocket data; otherwise, it will
 *return 0 or -1.
 *
 * */
typedef int (*_serialPktParse)(void* pData, unsigned int length);	//It is used to parse the data area contain a valid packet
typedef void (*_serialProcess)(void* pData, unsigned int length);	//It is used to process Serial data
typedef void (*_serialDevMFree)(struct serialDevM* pM);
typedef int (*_serialDevMSend)(struct serialDevM* pM, void* pData, unsigned int length);

typedef struct serialDevM
{
	SERIAL_DEV_ID devID;
	_serialDevMFree free;
	_serialDevMSend send;
}serialDevM_t;

#define DEV_NAME_LEN	(64)
typedef struct serialDevMParam
{
	char devName[DEV_NAME_LEN];
	baudRate_e rate;
	dataBits_e dataBits;
	stopBits_e stopBits;
	parity_e parity;
	_serialProcess process;
	_serialPktParse parse;
}serialDevMParam_t;

serialDevM_t* serialDevMMalloc(serialDevMParam_t params);

#ifndef __cplusplus
}
#endif
#endif /* SCANDEVAPI_H_ */
