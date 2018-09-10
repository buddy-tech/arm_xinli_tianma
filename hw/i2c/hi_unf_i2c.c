#include "hi_unf_i2c.h"
#include <stdio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

#include "i2cCtrlApi.h"
#include "virtualI2cCtrlApi.h"
#include "hi_unf_gpio.h"

static i2cCtrlM_t* pSgI2cCtrlM[2] = {0};	//I2C control Devices Manager

static virtualI2cCtrlM_t* pSgVirutalI2cM[2] = {0};	//Virtual I2C control Devices Manager

MS_S32 HI_UNF_I2C_Init(MS_VOID)
{
    pSgI2cCtrlM[0] = i2cCtrlMMalloc("/dev/i2c-0");
    pSgI2cCtrlM[1] = i2cCtrlMMalloc("/dev/i2c-1");

    pSgVirutalI2cM[0] = virtualI2cCtrlMMalloc();
    pSgVirutalI2cM[0]->setGpio(pSgVirutalI2cM[0], LCM2_I2C_SDA, LCM2_I2C_SCK, LCM2_I2C_SDA_DIR);

    pSgVirutalI2cM[1] = virtualI2cCtrlMMalloc();
    pSgVirutalI2cM[1]->setGpio(pSgVirutalI2cM[1], LCM1_I2C_SDA, LCM1_I2C_SCK, LCM1_I2C_SDA_DIR);

	return 0;
}

MS_S32 HI_UNF_I2C_DeInit(MS_VOID)
{
	pSgI2cCtrlM[0]->free(pSgI2cCtrlM[0]);
	pSgI2cCtrlM[1]->free(pSgI2cCtrlM[1]);
	pSgVirutalI2cM[0]->free(pSgVirutalI2cM[0]);
	pSgVirutalI2cM[1]->free(pSgVirutalI2cM[1]);

	return 0;
}

MS_S32 HI_UNF_I2C_Read(MS_U8 u8DevAddress, MS_U32 u32RegAddr, MS_U8 *pu8Buf)
{
	i2cCtrlM_t* pCtrlM = pSgI2cCtrlM[0];
	*pu8Buf = pCtrlM->read(pCtrlM, u8DevAddress, u32RegAddr);
    return 0;
}

MS_S32 HI_UNF_I2C_Write(MS_U8 u8DevAddress, MS_U32 u32RegAddr, MS_U8 value)
{
	i2cCtrlM_t* pCtrlM = pSgI2cCtrlM[0];
    return pCtrlM->write(pCtrlM, u8DevAddress, u32RegAddr, value);
}

#define SET_FORMAT_BIT(_data, _val, _mask, _move)	((_data) = (((_data) & (~((_mask) << (_move)))) | (((_val) & (_mask)) << (_move))))
#define SET_SID_BIT(_data, _val) SET_FORMAT_BIT(_data, _val, 0x3, 5)
#define SET_SLAVE_ADDR(_data) ((_data) = ((_data) << 1)&0xff)

MS_S32 HI_UNF_I2C_Read2(MS_U8 u8DevAddress, MS_U32 u32RegAddr, MS_U8 *pu8Buf)
{
	i2cCtrlM_t* pCtrlM = pSgI2cCtrlM[1];
	SET_SID_BIT(u32RegAddr, 1);
	*pu8Buf = pCtrlM->read(pCtrlM, u8DevAddress, u32RegAddr);
    return 0;
}

MS_S32 HI_UNF_I2C_Write2(MS_U8 u8DevAddress, MS_U32 u32RegAddr, MS_U8 value)
{
	i2cCtrlM_t* pCtrlM = pSgI2cCtrlM[1];
	SET_SID_BIT(u32RegAddr, 1);
    return pCtrlM->write(pCtrlM, u8DevAddress, u32RegAddr, value);
}

/* read from device of virtual I2C control*/
void vi2c_read(unsigned int channel, unsigned char slaveAddr, unsigned char regAddr, unsigned char* pValue)
{
	virtualI2cCtrlM_t* pM = NULL;

	if (channel == 0 || channel == 1)
		pM = pSgVirutalI2cM[0];
	else
		pM = pSgVirutalI2cM[1];

	pM->read(pM, slaveAddr, regAddr, pValue);
}

/* write from device of virtual I2C control*/
void vi2c_write(unsigned int channel, unsigned char slaveAddr, unsigned char regAddr, unsigned char value)
{
	virtualI2cCtrlM_t* pM = NULL;

	if (channel == 0 || channel == 1)
		pM = pSgVirutalI2cM[0];
	else
		pM = pSgVirutalI2cM[1];

	pM->write(pM, slaveAddr, regAddr, value);
}
