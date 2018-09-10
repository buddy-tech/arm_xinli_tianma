#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mipi2828.h"
#include "fpgaFunc.h"

#include "hi_unf_spi.h"
#include "mipi_interface.h"
#include "MsOS.h"
#include "client.h"
#include "common.h"
#include "hi_unf_gpio.h"

static mipi2828DevM_t* pSgMipi2828DevM = NULL;
#define MAX_MIPI_CODE_LEN 3072
unsigned char InitCode[MAX_MIPI_CODE_LEN];//3072

/* channel is left to right */
static unsigned int translateChannel2SpiCs(unsigned int channel)
{
	unsigned int cs = 0;
	switch (channel)
	{
	case 3:
		cs = SPI0_0_CS;
		break;

	case 2:
		cs = SPI0_1_CS;
		break;

	case 1:
		cs = SPI1_0_CS;
		break;

	case 0:
		cs = SPI1_1_CS;
		break;
	}

	return cs;
}

void mipi2828Write2IC(int channel, unsigned char regAddr, unsigned char* pData, int length)
{
	unsigned int spiNo = translateChannel2SpiCs(channel);
    WriteSSD2828(spiNo, MIPI_2828_VC_CTRL_REG, 0X0000);

    if (length <= 0 || !pData)
    	WriteCMDSSD2828(spiNo, regAddr);
    else
    {
    	WriteSSD2828(spiNo, MIPI_2828_PARAM_LEN_REG, length);
    	unsigned short regValue = regAddr | *pData << 8;
    	WriteSSD2828(spiNo, MIPI_2828_DST_CACHE_REG, regValue);
    	length -= 1;
    	if (length <= 0)
    		return;

    	pData = pData + 1;
    	unsigned short* pTemp = NULL;
    	int actualLen = length/2;
    	if (length%2 != 0)
    		actualLen = length/2 + 1;

    	pTemp = (unsigned short *)calloc(1, actualLen*sizeof(unsigned short));
    	memcpy(pTemp, pData, length);

    	int i = 0;
    	for (; i < actualLen; i++)
    	{
    		WriteDATASSD2828(spiNo, *pTemp++);
    	}
    }
}

int ReadModReg(unsigned char channel, unsigned char type, unsigned char regAddr, unsigned char len, unsigned char *pRead)
{
    int count = 0;
	int readBytes = 0;
	unsigned int spiNo = translateChannel2SpiCs(channel);

    while(count < 3)
    {
		WriteSSD2828(spiNo, 0XC1, len);

        if(type == 0)   //0:Gerneric
            WriteSSD2828(spiNo, 0XB7, 0x0380);
        else if(type == 1)   //1:DCS
            WriteSSD2828(spiNo, 0XB7, 0x03c0);

        WriteSSD2828(spiNo, 0XC0, 0x0100);	// reset
        WriteSSD2828(spiNo, 0XBC, 0x0001);

        WriteSSD2828(spiNo, 0Xbf, regAddr);	// write packet data

   		_msleep(10);

        unsigned short regValue = ReadSSD2828Reg(spiNo, 0XC6);

        if((regValue & 0x0005) == 0x0005)
        {
        	regValue = ReadSSD2828Reg(spiNo, 0XC2);

        	readBytes = regValue & 0xffff;
			int i = 0;
            for(;i < len; i += 2)
            {
                if(i == 0)
                	regValue = ReadSSD2828Reg(spiNo, 0XFF);
                else
                	regValue = ReadDATASSD2828Reg(spiNo, 0XFF);

                pRead[i]   =  regValue&0xFF;
                if(i + 1 < len)
                {
                    pRead[i + 1] = (regValue & 0xFF00) >> 8;
                }
            }

            break;
        }
        else
            count ++;
    }

	if (readBytes <= 0)
		DBG_ERR("****** ReadModReg error: RetLen = %d. count = %d. ******\n", readBytes, count);

	return readBytes;
}

static void setVsCnt(unsigned short value)
{
	pSgMipi2828DevM->timing.vsyncpulseWidth = value;
}

static void setVBPCnt(unsigned short value)
{
	pSgMipi2828DevM->timing.vBackPorch = value;
}

static void setVFPCnt(unsigned short value)
{
	pSgMipi2828DevM->timing.vFrontPorch = value;
}

static void setVDPCnt(unsigned short value)
{
	pSgMipi2828DevM->timing.vActiveTime = value;
}

static void setVTotal(unsigned short value)
{
	pSgMipi2828DevM->timing.vTotal = value;
}

static void setHsCnt(unsigned short value)
{
	pSgMipi2828DevM->timing.hsyncpulseWidth = value;
}

static void setHBPCnt(unsigned short value)
{
	pSgMipi2828DevM->timing.hBackPorch = value;
}

void setHFPCnt(unsigned short value)
{
	pSgMipi2828DevM->timing.hFrontPorch = value;
}

static void setHDPCnt(unsigned short value)
{
	pSgMipi2828DevM->timing.hActiveTime = value;
}

static void setHTotal(unsigned short value)
{
	pSgMipi2828DevM->timing.hTotal = value;
}

static void setLaneCnt(unsigned char value)
{
	pSgMipi2828DevM->timing.laneCnt = value;
}

static void setSyncMode(unsigned char value)
{
	pSgMipi2828DevM->timing.syncMode = value;
}

static void setDSIFRE(unsigned int value)
{
	pSgMipi2828DevM->timing.dsifre = value;
}

static void setBitNum(unsigned int value)
{
	pSgMipi2828DevM->timing.bit = value;
}

static void calculateConfigParams()
{
    unsigned int i,ByteClk,ByteClkMhz,LPClk,LPClkKHz;
    // unsigned char send_buffer[100];
    unsigned char HsPrepareDelay,HsZeroDelay,ClkZeroDelay,ClkPrepareDelay,ClkPreDelay,ClkPostDelay,HsTrailDelay,TAGoDelay,TAGetDelay;
    unsigned short WakeUpDelay;

    picture_timing_t* pPictureTiming = &pSgMipi2828DevM->timing;
    configure2828Info_t* pInfo = &pSgMipi2828DevM->configure;

    ByteClk = pPictureTiming->dsifre / 8;
    ByteClkMhz = ByteClk / 1000000;

    pInfo->reg_ba = 0;
    pInfo->reg_b1 = pPictureTiming->vsyncpulseWidth & 0xff;
    pInfo->reg_b1 = pPictureTiming->hsyncpulseWidth & 0xff + (pInfo->reg_b1<<8);
	
    if(pPictureTiming->syncMode == 0 )  //pulse
    {
    	pInfo->reg_b2 = pPictureTiming->vBackPorch;
    	pInfo->reg_b2 = pPictureTiming->hBackPorch & 0xff + (pInfo->reg_b2<<8);
    }
	
    if(pPictureTiming->syncMode == 1 || pPictureTiming->syncMode == 2)  //burst or event
    {
    	pInfo->reg_b2 = pPictureTiming->vBackPorch+pPictureTiming->vsyncpulseWidth;
    	pInfo->reg_b2 = (pPictureTiming->hBackPorch+pPictureTiming->hsyncpulseWidth) & 0xff + (pInfo->reg_b2<<8);
    }
	
    pInfo->reg_b3 = pPictureTiming->vFrontPorch & 0xff;
    pInfo->reg_b3 = pPictureTiming->hFrontPorch & 0xff + (pInfo->reg_b3<<8);

    pInfo->reg_b4 = pPictureTiming->hActiveTime;
    pInfo->reg_b5 = pPictureTiming->vActiveTime;
	
    //25M
    if(pPictureTiming->dsifre >= 62500000 && pPictureTiming->dsifre < 125000000)   //25M   4分频
    	pInfo->reg_ba = 0x0400;

    else if(pPictureTiming->dsifre >= 125000000 && pPictureTiming->dsifre < 250000000)
    	pInfo->reg_ba = 0x4400;

    else if(pPictureTiming->dsifre >= 250000000 && pPictureTiming->dsifre < 500000000)
    	pInfo->reg_ba = 0x8400;

    else if(pPictureTiming->dsifre >= 500000000 && pPictureTiming->dsifre <=1000000000)
    	pInfo->reg_ba = 0xc400;

    pInfo->reg_ba = pInfo->reg_ba + pPictureTiming->dsifre/6250000; //25M/4 = 6.25M
    pInfo->reg_bb = (pPictureTiming->dsifre/80000000) - 1;  //lp

    LPClk = ByteClk / (pInfo->reg_bb + 1);
    LPClkKHz = LPClk / 1000;
	
    HsPrepareDelay = (600+48*ByteClkMhz)/500 + 1;
    if (HsPrepareDelay >= 256 || HsPrepareDelay < 4)
    	HsPrepareDelay = 0;
    else
    	HsPrepareDelay -= 4;

    HsZeroDelay = (1500 + 150*ByteClkMhz)/500 + 1;
    if (HsZeroDelay >= 256)
    	HsZeroDelay = 0;
	
    pInfo->reg_c9 = (HsZeroDelay << 8) + HsPrepareDelay;
    
    ClkPrepareDelay = 44*ByteClkMhz/500 + 1;
    if (ClkPrepareDelay >= 256 || ClkPrepareDelay < 3)
    	ClkPrepareDelay = 0;
    else
    	ClkPrepareDelay -= 3;

    ClkZeroDelay = 280*ByteClkMhz/500 + 1;
    if (ClkZeroDelay >= 256)
    	ClkPrepareDelay = 0;
	
    pInfo->reg_ca = (ClkZeroDelay << 8) + ClkPrepareDelay;

    ClkPostDelay = (7000 + 65*ByteClkMhz)/500 + 1;
    if (ClkPostDelay >= 256)
    	ClkPostDelay = 0;

    ClkPreDelay = (650 + 69*ByteClkMhz)/500 + 1;
    if (ClkPreDelay >= 256)
    	ClkPreDelay = 0;
	
    pInfo->reg_cb = (ClkPreDelay << 8) + ClkPostDelay;

    HsTrailDelay = (1250 + 100*ByteClkMhz)/500 + 1;
    if (HsTrailDelay >= 256)
    	HsTrailDelay = 0;

    pInfo->reg_cc = (HsTrailDelay << 8) + HsTrailDelay;

    WakeUpDelay = 1050*LPClkKHz/1000 + 1;
    if (WakeUpDelay >= 256)
    	WakeUpDelay = 0;

    pInfo->reg_cd = WakeUpDelay;

    WakeUpDelay = 275*LPClkKHz/500000 + 1;
    if (WakeUpDelay >= 16)
    	WakeUpDelay = 0;

    TAGoDelay = 220*LPClkKHz/500000 + 1;
    if (TAGoDelay >= 16)
    	TAGoDelay = 0;
	
    pInfo->reg_ce = (TAGoDelay << 8) + TAGetDelay;
    
    if (pPictureTiming->bit == 6)
    	pInfo->reg_b6 = 0x02 + 0x04*pPictureTiming->syncMode;
    else if (pPictureTiming->bit == 8)
    	pInfo->reg_b6 = 0x03 + 0x04*pPictureTiming->syncMode;

    pInfo->reg_de = pPictureTiming->laneCnt - 1;
    pInfo->reg_d6 = 4;//5;  //05BGR  04RGB
}

static void ConfigSSD2828(int channel)
{
	unsigned int spiNo = translateChannel2SpiCs(channel);
	configure2828Info_t* pInfo = &pSgMipi2828DevM->configure;

    WriteSSD2828(spiNo, 0XB1, pInfo->reg_b1);//Vertical Sync Active Period    Horizontal Sync Active Period
    WriteSSD2828(spiNo, 0XB2, pInfo->reg_b2);//VBP   HBP
    WriteSSD2828(spiNo, 0XB3, pInfo->reg_b3);//VFP   HFP
    WriteSSD2828(spiNo, 0XB4, pInfo->reg_b4);//x 540
    WriteSSD2828(spiNo, 0XB5, pInfo->reg_b5);//y 960
    WriteSSD2828(spiNo, 0XB6, pInfo->reg_b6);

    WriteSSD2828(spiNo, 0XDE, pInfo->reg_de);//4lane
    WriteSSD2828(spiNo, 0XD6, pInfo->reg_d6);//Packet Number in Blanking Period  BGR   05BGR  04RGB
    WriteSSD2828(spiNo, 0XB9, 0X0000);//pll powerdown
    WriteSSD2828(spiNo, 0XC4, 0X0000);//Automatically perform BTA after the next write packe

    WriteSSD2828(spiNo, 0XC9, pInfo->reg_c9);   //htc 4.7(pa68)
    WriteSSD2828(spiNo, 0XCA, pInfo->reg_ca);
    WriteSSD2828(spiNo, 0XCB, pInfo->reg_cb);
    WriteSSD2828(spiNo, 0XCC, pInfo->reg_cc);
    WriteSSD2828(spiNo, 0XCD, pInfo->reg_cd);
    WriteSSD2828(spiNo, 0XCE, pInfo->reg_ce);

    WriteSSD2828(spiNo, 0XBA, pInfo->reg_ba);//pll config     /5   *70
    WriteSSD2828(spiNo, 0XBB, pInfo->reg_bb);//LP Clock Control Register
    WriteSSD2828(spiNo, 0XB9, 0X0001);//PLL Control Register
    WriteSSD2828(spiNo, 0XB8, 0X0000);//VC Control Register
}

void setMipi2828HighSpeedMode(int channel, bool bHSMode)
{
	unsigned int spiNo = translateChannel2SpiCs(channel);
	if (bHSMode)
		WriteSSD2828(spiNo, MIPI_2828_MODE_REG, 0X030b);
	else
		WriteSSD2828(spiNo, MIPI_2828_MODE_REG, 0X030a);

	pSgMipi2828DevM->isHiSpeedMode = bHSMode;
}

static void config_mipi_channel(unsigned int channel)
{
	unsigned int spiNo = translateChannel2SpiCs(channel);
	ConfigSSD2828(channel);
}

static void initChannel(unsigned int channel)
{
	calculateConfigParams();

	setMipi2828HighSpeedMode(channel, false);
	_msleep(10);
	config_mipi_channel(channel);
	_msleep(10);
	setMipi2828HighSpeedMode(channel, true);
}

static void deinitChannel(unsigned int channel)
{
	mipi2828Write2IC(channel, 0x28, NULL, 0);
	mipi2828Write2IC(channel, 0x10, NULL, 0);

	unsigned int spiNo = translateChannel2SpiCs(channel);
	WriteSSD2828(spiNo, MIPI_2828_MODE_REG,0x0340);
	WriteSSD2828(spiNo, 0XC4,0x0004);
}

void mipi2828ModuleInit()
{
	pSgMipi2828DevM = calloc(1, sizeof(mipi2828DevM_t));
}

void mipiIcReset(int channel)
{
	if(channel == 0 || channel == 1) //left
	{
		gpio_set_output_value(GPIO_PIN_RST_L,0);
		_msleep(10);
		gpio_set_output_value(GPIO_PIN_RST_L,1);
	}
	else //right
	{
		gpio_set_output_value(GPIO_PIN_RST_R,0);
		_msleep(10);
		gpio_set_output_value(GPIO_PIN_RST_R,1);
	}
}

static void mipiInterfaceReset(unsigned int channel)
{
    if(channel == 0 || channel == 1)	//left
    {
        gpio_set_output_value(RST_2828_GPIO,0);
        _msleep(10);
        gpio_set_output_value(RST_2828_GPIO,1);

        gpio_set_output_value(GPIO_PIN_RST_L,0);
        _msleep(10);
        gpio_set_output_value(GPIO_PIN_RST_L,1);
    }
    else	//right
    {
        gpio_set_output_value(RST_2828_GPIO,0);
        _msleep(10);
        gpio_set_output_value(RST_2828_GPIO,1);

        gpio_set_output_value(GPIO_PIN_RST_R,0);
        _msleep(10);
        gpio_set_output_value(GPIO_PIN_RST_R,1);
    }
}

static void mipiInterfaceTimingInit(timing_info_t *pInfo)
{
	setVsCnt(pInfo->timvsyncpulsewidth);
	setVBPCnt(pInfo->timvbackporch);
	setVFPCnt(pInfo->timvfrontporch);
	setVDPCnt(pInfo->timvactivetime);
	setVTotal(pInfo->timvtotaltime);
	setHsCnt(pInfo->timhsyncpulsewidth);
	setHBPCnt(pInfo->timhbackporch);
	setHFPCnt(pInfo->timhfrontporch);
	setHDPCnt(pInfo->timhactivetime);
	setHTotal(pInfo->timhtotaltime);
	setLaneCnt(pInfo->timmipilanenum);
	setSyncMode(pInfo->timmipisyncmode);
	setDSIFRE(pInfo->mdlmipidsiclock);
	setBitNum(pInfo->timbit);
}

int mipi2828Open(timing_info_t *timing_info, int channel)
{
	mipiInterfaceTimingInit(timing_info);
    mipiInterfaceReset(0);
    mipiInterfaceReset(1);
    mipiInterfaceReset(2);
    mipiInterfaceReset(3);

	unsigned short sB0Value2 = ReadSSD2828Reg(SPI0_0_CS, 0xb0);
	printf("MIPI Channel 1 B0: %#x.\n", sB0Value2);

	unsigned short sB0Value = ReadSSD2828Reg(SPI0_1_CS, 0xb0);
	printf("MIPI Channel 2 B0: %#x.\n", sB0Value);

	unsigned short sB0Value3 = ReadSSD2828Reg(SPI1_0_CS, 0xb0);
	printf("MIPI Channel 3 B0: %#x.\n", sB0Value3);

	unsigned short sB0Value4 = ReadSSD2828Reg(SPI1_1_CS, 0xb0);
	printf("MIPI Channel 4 B0: %#x.\n", sB0Value4);

	initChannel(DEV_CHANNEL_0);
	initChannel(DEV_CHANNEL_1);
	initChannel(DEV_CHANNEL_2);
	initChannel(DEV_CHANNEL_3);

    ic_mgr_set_channel_state(DEV_CHANNEL_0, 1);
    ic_mgr_set_channel_state(DEV_CHANNEL_1, 1);
    ic_mgr_set_channel_state(DEV_CHANNEL_2, 1);
    ic_mgr_set_channel_state(DEV_CHANNEL_3, 1);
}

void mipi2828Close()
{
	ic_mgr_set_channel_state(0, 0);
	ic_mgr_set_channel_state(1, 0);
	ic_mgr_set_channel_state(2, 0);
	ic_mgr_set_channel_state(3, 0);

	deinitChannel(DEV_CHANNEL_0);
	deinitChannel(DEV_CHANNEL_1);
	deinitChannel(DEV_CHANNEL_2);
	deinitChannel(DEV_CHANNEL_3);
}
