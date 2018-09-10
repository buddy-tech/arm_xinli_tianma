#ifndef __HI_UNF_SPI_H__
#define __HI_UNF_SPI_H__

#include "virtualSpiCtrlApi.h"
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int SPI_CTRL_ID;

#define SPI_CONTROL(_no)			((_no) << 4)

#define SPI_CS(_ctrl, _index)		(SPI_CONTROL(_ctrl) | (0x1 << (_index)))

#define SPI0_0_CS				SPI_CS(1, 0)
#define SPI0_1_CS				SPI_CS(1, 1)
#define SPI0_2_CS				SPI_CS(1, 2)

#define SPI1_0_CS				SPI_CS(2, 0)
#define SPI1_1_CS				SPI_CS(2, 1)
#define SPI1_2_CS				SPI_CS(2, 2)
#define SPI1_3_CS				SPI_CS(2, 3)


#define SPI0_CS_PIN(_index) (90 + (_index))
#define SPI1_CS_PIN(_index) (78 + (_index))

#define MIPI_SPI_DEV_NAME	"/dev/spidev32766.0"
#define MIPI_SPI2_DEV_NAME	"/dev/spidev32765.0"

void spiCtrlInit();
void spiCtrlfree();
int spiCtrlRead(int spiNo,char *pInData,int dataLen,char *poutData);
int spiCtrlWrite(int spiNo,char *pInData,int dataLen);

typedef enum
{
	WIRE_3_LINE,
	WIRE_4_LINE,
}VSPI_MODE_E;

SPI_CTRL_ID vspiCtrlInstance(unsigned int channel);
void vspiCtrlSetByteFormat(SPI_CTRL_ID id, spiByteFormat_e format);
void vspiCtrlSetCsState(SPI_CTRL_ID id, bool bSelected);
void vspiCtrlSetMode(SPI_CTRL_ID id, VSPI_MODE_E mode);
int vspiCtrlWrite(SPI_CTRL_ID id, unsigned int value);
void vspiCtrlRead(SPI_CTRL_ID id, unsigned int* pValue);

void setVspiMode(unsigned int channel, VSPI_MODE_E mode);
void vspi_read(unsigned int channel, unsigned char addr, unsigned char* pValue);
void vspi_write(unsigned int channel, unsigned char addr, unsigned char value);
#ifdef __cplusplus
}
#endif
#endif
