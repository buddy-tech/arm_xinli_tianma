/*********************************************************************************
*
*  Copyright (C) 2014 Hisilicon Technologies Co., Ltd.  All rights reserved. 
*
*  This program is confidential and proprietary to Hisilicon Technologies Co., Ltd.
*  (Hisilicon), and may not be copied, reproduced, modified, disclosed to
*  others, published or used, in whole or in part, without the express prior
*  written permission of Hisilicon.
*
***********************************************************************************/

#ifndef __HI_UNF_I2C_H__
#define __HI_UNF_I2C_H__

#include "MsTypes.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*************************** Structure Definition ****************************/
/** \addtogroup      I2C */
/** @{*/  /** <!-[I2C]*/

#define HI_UNF_I2C_Open  HI_UNF_I2C_Init
#define HI_UNF_I2C_Close HI_UNF_I2C_DeInit

/**Maximum I2C channel ID*/ /**CNcomment:最大I2C通道号*/
#define HI_I2C_MAX_NUM_USER (15)

/**Rate type of the I2C module*/
/**CNcomment:I2C的速率类型 */
typedef enum hiUNF_I2C_RATE_E
{
    HI_UNF_I2C_RATE_10K = 0, /**<Standard rate: 10 kbit/s*/         /**<CNcomment:标准速率：10kbit/s*/
    HI_UNF_I2C_RATE_50K, /**<Standard rate: 50 kbit/s*/   /**<CNcomment:标准速率：50kbit/s*/
    HI_UNF_I2C_RATE_100K, /**<Standard rate: 100 kbit/s*/  /**<CNcomment:标准速率：100kbit/s*/
    HI_UNF_I2C_RATE_200K, /**<Standard rate: 200 kbit/s*/  /**<CNcomment:标准速率：200kbit/s*/
    HI_UNF_I2C_RATE_300K, /**<Standard rate: 300 kbit/s*/  /**<CNcomment:标准速率：300kbit/s*/
    HI_UNF_I2C_RATE_400K, /**<Fast rate: 400 kbit/s*/      /**<CNcomment:快速速率：400kbit/s*/
    HI_UNF_I2C_RATE_BUTT
} HI_UNF_I2C_RATE_E;

/** @}*/  /** <!-- ==== Structure Definition End ====*/

/******************************* API Declaration *****************************/
/** \addtogroup      I2C*/
/** @{*/  /** <!-- -I2C=*/

/**
 \brief Init the I2C device.
CNcomment:\brief 初始化I2C（the Inter-Integrated Circuit）设备。CNend

 \param N/A                                                               CNcomment:无。CNend
 \retval 0 Success                                                        CNcomment:成功。CNend
 \retval ::HI_ERR_I2C_OPEN_ERR  Open I2c Error				  CNcomment:I2C无效参数。CNend
 \see \n
N/A
 */
MS_S32 HI_UNF_I2C_Init (MS_VOID);

/**
 \brief  DeInit the I2C device.
CNcomment:\brief 去初始化I2C设备。CNend

 \attention \n
This API is called after I2C operations are completed.\n
CNcomment:在I2C操作结束后调用此接口\n CNend

 \param N/A                                                        CNcomment:无。CNend
 \retval 0 Success                                                 CNcomment: 成功。CNend
 \retval ::HI_ERR_I2C_CLOSE_ERR  Close I2c Error.	 	   CNcomment:I2C设备未初始化。CNend
 \see \n
N/A
 */
MS_S32 HI_UNF_I2C_DeInit(MS_VOID);
/**
 \brief Reads data by using the I2C bus.
CNcomment:\brief 通过I2C读数据。CNend

 \attention \n
N/A
 \param[in] u32I2cNum  I2C bus of the device to be read           CNcomment:所读取设备使用的I2C总线。CNend
 \param[in] u8DevAddress  Address of a device on the I2C bus      CNcomment:设备在I2C总线上的地址。CNend
 \param[in] u32RegAddr  On-chip offset address of a device        CNcomment:设备的片内偏移地址。CNend
 \param[in] u32RegAddrCount  Length of an on-chip offset address. CNcomment:片内偏移地址的长度单位。CNend
                      1: 8-bit sub address                       CNcomment:1：表示8bit子地址；CNend
                      2: 16-bit sub address                      CNcomment:2：表示16bit子地址；CNend
                      3: 24-bit sub address                      CNcomment:3：表示24bit子地址；CNend
                      4: 32-bit sub address                      CNcomment:4：表示32bit子地址。CNend

 \param[out] pu8Buf   Buffer for storing the data to be read                            CNcomment:读Buffer，存放读取数据。CNend
 \param[in] u32Length  Length of the data to be read                                    CNcomment:要读取的数据长度。CNend
 \retval 0 Success                                                                      CNcomment:成功。CNend
 \retval ::HI_FAILURE	Read data failed					  	CNcomment:失败。CNend
 \retval ::HI_ERR_I2C_NOT_INIT  The I2C device is not initialized.                      CNcomment:I2C设备未初始化。CNend
 \retval ::HI_ERR_I2C_NULL_PTR  The I2C pointer is invalid.                        	   CNcomment:I2C无效指针。CNend
 \retval ::HI_ERR_I2C_INVALID_PARA  The I2C parameter is invalid.                       CNcomment:I2C无效参数。CNend
 \retval ::HI_ERR_I2C_FAILED_READ  Data fails to be read by using the I2C bus.          CNcomment:I2C读数据失败。CNend
 \see \n
N/A
 */
MS_S32 HI_UNF_I2C_Read(MS_U8 u8DevAddress, MS_U32 u32RegAddr, MS_U8 *pu8Buf);

/**
 \brief Writes data by using the I2C bus. That is, you can call this API to write data to the device mounted on the I2C bus through the I2C channel.
CNcomment:\brief 通过I2C写数据。通过I2C通道，向I2C总线上挂接的设备进行写操作。CNend

 \attention \n
N/A
 \param[in] u32I2cNum  I2C bus of the device to be written         CNcomment:待写设备使用的I2C总线。CNend
 \param[in] u8DevAddress  Address of a device on the I2C bus       CNcomment:设备在I2C总线上的地址。CNend
 \param[in] u32RegAddr  On-chip offset address of a device         CNcomment:设备的片内偏移地址。CNend
 \param[in] u32RegAddrCount Length of an on-chip offset address.   CNcomment:片内偏移地址的长度单位。CNend
                    1: 8-bit sub address                          CNcomment:1：表示8bit子地址；CNend
                    2: 16-bit sub address                         CNcomment:2：表示16bit子地址；CNend
                    3: 24-bit sub address                         CNcomment:3：表示24bit子地址；CNend
                    4: 32-bit sub address                         CNcomment:4：表示32bit子地址。CNend

 \param[in]  pu8Buf   Buffer for storing the data to be written                         CNcomment:写Buffer，存放待写入数据。CNend
 \param[in] u32Length  Length of the data to be written                                 CNcomment:要写入的数据的长度。CNend
 \retval 0  Success                                                                     CNcomment:成功。CNend
 \retval ::HI_FAILURE	Write data failed					  	CNcomment:失败。CNend
 \retval ::HI_ERR_I2C_NOT_INIT  The I2C device is not initialized.                      CNcomment:I2C设备未初始化。CNend
 \retval ::HI_ERR_I2C_NULL_PTR  The I2C pointer is invalid.                        	   CNcomment:I2C无效指针。CNend
 \retval ::HI_ERR_I2C_INVALID_PARA  The I2C parameter is invalid.                       CNcomment:I2C无效参数。CNend
 \retval ::HI_ERR_I2C_FAILED_WRITE  Data fails to be written by using the I2C bus.      CNcomment:I2C写数据失败。CNend
 \see \n
N/A
 */
MS_S32 HI_UNF_I2C_Write(MS_U8 u8DevAddress, MS_U32 u32RegAddr,MS_U8 value);


/** @} */  /** <!-- ==== API Declaration End ==== */

MS_S32 HI_UNF_I2C_Read2(MS_U8 u8DevAddress, MS_U32 u32RegAddr, MS_U8 *pu8Buf);

MS_S32 HI_UNF_I2C_Write2(MS_U8 u8DevAddress, MS_U32 u32RegAddr, MS_U8 value);

void vi2c_read(unsigned int channel, unsigned char slaveAddr, unsigned char regAddr, unsigned char* pValue);

void vi2c_write(unsigned int channel, unsigned char slaveAddr, unsigned char regAddr, unsigned char value);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __HI_UNF_ECS_TYPE_H__ */
