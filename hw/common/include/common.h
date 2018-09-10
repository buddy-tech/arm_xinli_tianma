
#ifndef __COMMON_H
#define __COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <syslog.h>

#ifdef __cplusplus
extern "C" {
#else
typedef enum
{
	false,
	true,
}bool;
#endif

#ifdef _DEBUG
#	define	DBG(fmt, arg...) printf(fmt, ##arg)
#else
#	define	DBG(fmt, arg...)
#endif

#define	DBG_WARNING(fmt, arg...) printf("Warning: "fmt, ##arg)
#define	DBG_ERR(fmt, arg...) printf("Error: "fmt, ##arg);

typedef unsigned short      WORD16; 	  /* 16位无符号整型  */
typedef unsigned int        WORD32; 	  /* 32位无符号整型  */

typedef unsigned char       BYTE;         /* 8位无符号整型   */
typedef signed int          INT;          /* 32位有符号整型   */
typedef signed char         CHAR;         /* 8位有符号字符型  */		
typedef signed long         LONG;         /* 32位有符号长整型 */
typedef signed short	    SHORT;        /* 16位有符号整型  */
typedef void                *LPVOID;      /* 空类型           */
typedef unsigned char       BOOLEAN;      /* 布尔类型        */

#define SUCCESS (0)
#define FAILED   (-1)
#define BREAK_OUT (-2)

#define MAX_FILE_NAME_LEN               64
#define MAX_STRING_LEN                  64
#define MAX_PATH_LEN			        128

#define MAX_CMD_NUM        				64
#define PWR_MAX_CMD_NUM        			32
#define EXT_MAX_CMD_NUM        			32

#define PWR_MSG_FROM_ETH                0x01
#define PWR_MSG_FROM_BOX                0x02
#define PWR_MSG_FROM_LOCAL              0x03
#define PWR_MSG_FROM_UART				0x04	//it is need modify

typedef enum
{
	PWR_CHANNEL_ALL = 0x1,
	PWR_CHANNEL_LEFT,
	PWR_CHANNEL_RIGHT,
}PWR_CHANNEL_E;

typedef enum
{
	TTL_LVDS_CHANNEL_LEFT,
	TTL_LVDS_CHANNEL_RIGHT
}TTL_LVDS_CHANNEL_E;

/* device interface channel which from left to right */
typedef enum
{
	DEV_CHANNEL_0,
	DEV_CHANNEL_1,
	DEV_CHANNEL_2,
	DEV_CHANNEL_3,
	DEV_CHANNEL_NUM,
}DEV_CHANNEL_E;

#define FPGA_CMD_SET_REG             0X0B
#define FPGA_CMD_SET_3D_PIN          0X18
#define FPGA_CMD_SET_TPMODE          0X19
#define FPGA_CMD_ADJUST_LOGICPHOTO   0X22
#define FPGA_CMD_HANDO				 0x23
#define FPGA_CMD_SUDOKU				 0x24
#define FPGA_CMD_LVDS_TEST           0x25
#define FPGA_CMD_UPDATE              0x70
#define FPGA_CMD_GET_BY_PTN_INFO     0x72

#define PWR_CMD_POWER_ON             0X01
#define PWR_CMD_POWER_OFF            0X02
#define PWR_CMD_CTRL_SWITCH          0X03		// 打开单个电源项。
#define PWR_CMD_GET_VOL              0X04
#define PWR_CMD_GET_POWER_VER        0X08
#define PWR_CMD_CFG_POWER_STRUCT     0X09
#define PWR_CMD_CFG_POWER_FILE       0X19
#define PWR_CMD_ADJUST_VDIM          0X0B
#define PWR_CMD_ADJUST_PWR_FREQ      0X0C
#define PWR_CMD_ADJUST_INVERT        0X0D
#define PWR_CMD_ON_OFF               0X0D
#define PWR_CMD_ADJUST_BY_PTN        0X0F
#define PWR_CMD_SET_VOL              0X12
#define PWR_CMD_SET_FLY_TIME         0X14
#define PWR_CMD_GET_CABC_PWM		 0x25
#define PWR_CMD_SET_BOE_FREQ_ONOFF   0X30
#define PWR_CMD_POWER_CALIBRATION	 0x80
#define PWR_CMD_UPGRADE              0x12345679
#define PWR_CMD_SET_SINGLE_POWER	 0x50

#define MSG_TYPE_POWER               0x05
#define MSG_LCD_CMD_FLICK_START          0X0C12
#define MSG_LCD_CMD_SET_FLICK            0X0C13
#define MSG_LCD_CMD_FLICK_END            0X0C14
#define MSG_LCD_CMD_TTYUSB_ADD           0x0C15
#define MSG_LCD_CMD_TTYUSB_REMOVE        0x0C16

#define MSG_LCD_CMD_XYLV_START			0x0C17
#define MSG_LCD_CMD_XYLV_READ			0x0C18
#define MSG_LCD_CMD_XYLV_STOP			0x0C19

#define NOTIFY_ERROR_OK					(0)
#define NOTIFY_ERROR_POWER_CHECK		(-1)
#define NOTIFY_ERROR_OPEN_SHORT_CHECK	(-2)
#define NOTIFY_ERROR_NTC_CHECK			(-3)

typedef enum
{
	E_PWR_CHECK_ERR_OV,
	E_PWR_CHECK_ERR_OC,
	E_PWR_CHECK_ERR_UV,
	E_PWR_CHECK_ERR_UC,
};

enum 
{
	E_PWR_CHECK_OK = 0,
	// VDD
	E_PWR_CHECK_ERR_VDD_OV,
	E_PWR_CHECK_ERR_VDD_OC,
	E_PWR_CHECK_ERR_VDD_UV,
	E_PWR_CHECK_ERR_VDD_UC,

	// VDDIO => VINT
	E_PWR_CHECK_ERR_VINT_OV,
	E_PWR_CHECK_ERR_VINT_OC,
	E_PWR_CHECK_ERR_VINT_UV,
	E_PWR_CHECK_ERR_VINT_UC,

	// VSP
	E_PWR_CHECK_ERR_VSP_OV,
	E_PWR_CHECK_ERR_VSP_OC,
	E_PWR_CHECK_ERR_VSP_UV,
	E_PWR_CHECK_ERR_VSP_UC,

	// VSN
	E_PWR_CHECK_ERR_VSN_OV,
	E_PWR_CHECK_ERR_VSN_OC,
	E_PWR_CHECK_ERR_VSN_UV,
	E_PWR_CHECK_ERR_VSN_UC,

	// VGH: elvdd
	E_PWR_CHECK_ERR_VGH_OV,
	E_PWR_CHECK_ERR_VGH_OC,
	E_PWR_CHECK_ERR_VGH_UV,
	E_PWR_CHECK_ERR_VGH_UC,

	// VGL: elvss.
	E_PWR_CHECK_ERR_VGL_OV,
	E_PWR_CHECK_ERR_VGL_OC,
	E_PWR_CHECK_ERR_VGL_UV,
	E_PWR_CHECK_ERR_VGL_UC,

	// VCOM
	E_PWR_CHECK_ERR_VCOM_OV,
	E_PWR_CHECK_ERR_VCOM_OC,
	E_PWR_CHECK_ERR_VCOM_UV,
	E_PWR_CHECK_ERR_VCOM_UC,

	// VBL
	E_PWR_CHECK_ERR_VBL_OV,
	E_PWR_CHECK_ERR_VBL_OC,
	E_PWR_CHECK_ERR_VBL_UV,
	E_PWR_CHECK_ERR_VBL_UC,

	E_PWR_CHECK_ERR_VBL_OPEN, // open
	E_PWR_CHECK_ERR_VBL_SHORT_LIGHT,// pn short
	E_PWR_CHECK_ERR_VBL_SHORT_PN, // pn short
	E_PWR_CHECK_ERR_VBL_SHORT_NN, // nn short
	E_PWR_CHECK_ERR_VBL_SHORT_N_GND, // n gnd short
	E_PWR_CHECK_ERR_VBL_SHORT_P_GND,
	E_PWR_CHECK_OTHER,
};

#define SIZE_1_MB	(1024*1024) //1MByte
#define BMP_PIC_HEAD_LEN	(64)

#define CRCPOLY_LE 0xedb88320
#define PKT_SERIAL_CRC_LEN		(4)
#define PKT_SERIAL_HEADER_LEN	(12)

void dump_data1(unsigned char* p_data, int data_len);

void dump_data1Ex(unsigned char* p_data, int data_len);

void _usleep(unsigned long usecond);

void _msleep(unsigned long msecond);

void _sleep(unsigned long second);

unsigned int doCrc32(const unsigned char* pData, unsigned int len);

unsigned int doCom2H(const unsigned char* pIn, unsigned int length, unsigned char** pOut);

unsigned int doH2Com(const unsigned char* pIn, unsigned int length, unsigned char** pOut);

int fromUtf8ToGBK(const char* pSrc, char** pDst);

int fromGBK2Utf8(const char* pSrc, char** pDst);
#ifdef __cplusplus
}
#endif

#endif

