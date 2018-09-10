#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include "fpgaFunc.h"
#include "util/util.h"

#include "comStruct.h"
#include "fpgaCtrlAccess.h"
#include "pgDb.h"
#include "pubmoveptn.h"

void fpgaEnable()
{
	unsigned short regValue = fpga_read_16bits(FPGA_DDR_REG2);
	regValue |= (1 << 15);			// enable LVDS signal
	regValue &= ~0x10;				// 0x2_bit4，1：显示十字光标；0：不显示十字光标
	regValue &= ~(1 << 5);			// 0x2_bit5 图片显示开关。1，显示；0，关闭
	regValue &= ~(1 << 9);			// 0x2_bit9 屏幕时钟触发开关
	fpga_write_16bits(FPGA_DDR_REG2, regValue);

	// fpga 初始化
	fpga_write_16bits(FPGA_DDR_REGA, 0); // 测试模式0
	fpga_write_16bits(FPGA_DDR_REGB, 0); // 图片移动方式：0，不移动，0x8，up，0x10，down，0x1，left，0x2，right，0x18，垂直翻转，0x3，水平翻转，0x1B,中心翻转   bit6~bit13:图片移动速度
	fpga_write_16bits(FPGA_DDR_REG8, 0); // 水平偏置0~1919
	fpga_write_16bits(FPGA_DDR_REG9, 0); // 垂直偏置0~1079

	//十字光标设置
	fpga_write_16bits(FPGA_DDR_REG6, 0);	// 十字光标的水平坐标 0~1919
	fpga_write_16bits(FPGA_DDR_REG7, 0);	// 十字光标的垂直坐标 0~1079
	fpga_write_16bits(FPGA_DDR_REG1C, 0);	// 十字光标字体色默认值
	fpga_write_16bits(FPGA_DDR_REG1D, 0);
	fpga_write_16bits(FPGA_DDR_REG1E, 0x31ff);	// 十字光标背景色默认值
	fpga_write_16bits(FPGA_DDR_REG1F, 0xfff0);
}

fpgaVersionInfo_t fpgaGetVersion()
{
	fpgaVersionInfo_t version = {0};
	unsigned short regValue = fpga_read_16bits(FPGA_DDR_REGD);
	DBG("year = %x\n", regValue);
	version.year = ((regValue >> 12) & 0xf) * 1000 + ((regValue >> 8) & 0xf) * 100 +
			((regValue >> 4) & 0xf) * 10 + ((regValue >> 0) & 0xf) * 1;

	regValue = fpga_read_16bits(FPGA_DDR_REGE);
	DBG("mmdd = %x\n", regValue);
	version.mmdd[0] = ((regValue >> 12) & 0xf) * 10 + ((regValue >> 8) & 0xf) * 1;
	version.mmdd[1] = ((regValue >> 4) & 0xf) * 10 + ((regValue >> 0) & 0xf) * 1;

	regValue = fpga_read_16bits(FPGA_DDR_REGC);
	DBG("version = %x\n", regValue);
	version.version[0] = regValue & 0xff;
	version.version[1] = (regValue >> 8)&0xff;
	DBG("===version = %d, %d\n", version.version[0], version.version[1]);

	version.kernelVersion[0] = FPGA_VERSION_H;
	version.kernelVersion[1] = FPGA_VERSION_M;
	version.kernelVersion[2] = FPGA_VERSION_L;
	return version;
}

void fpgaSetPictureState(bool bShow)
{
	fpgaReset();
    unsigned short regValue = fpga_read_16bits(FPGA_DDR_REG2);

    /* 0x2_bit5 图片显示开关。1，显示；0，关闭  */
	if (bShow)
	    regValue |= (1 << 5);
	else
		regValue &= ~(1 << 5);

	 fpga_write_16bits(FPGA_DDR_REG2, regValue);
}

void fpgaShowPicture(unsigned short position, unsigned short size)
{
	unsigned short regValue = position&0xff | ((size&0xff) << 8);

	DBG( "position = %d\n", position);
	DBG( "size = %d\n", size);

	fpga_write_16bits(FPGA_DDR_REG4, regValue);		//显示图片编号寄存器
    regValue = fpga_read_16bits(FPGA_DDR_REG2);
    regValue |= (1 << 5);			// 0x2_bit5 图片显示开关。1，显示；0，关闭
    regValue &= ~(1 << 10);			// 取消RGB调节信号

    regValue |= (1 << 15);             //enable LVDS signal
    fpga_write_16bits(FPGA_DDR_REG2, regValue);		//置高

	regValue = fpga_read_16bits(FPGA_DDR_REG13);
	regValue &= ~(0x07 << 5);		// 清除RGB消色
	regValue &= ~(0x01 << 12);		// 清除RGB 调节
	fpga_write_16bits(FPGA_DDR_REG13, regValue);

	regValue = fpga_read_16bits(FPGA_DDR_REG3B);       //取消逻辑画面状态
	regValue &= ~(1 << 1);
	fpga_write_16bits(FPGA_DDR_REG3B, regValue);

	regValue = fpga_read_16bits(FPGA_DDR_REG2);
	regValue |= (1 << 11);
	fpga_write_16bits(FPGA_DDR_REG2, regValue);

    regValue = fpga_read_16bits(FPGA_DDR_REG2);
    regValue &= ~(1 << 11);
    fpga_write_16bits(FPGA_DDR_REG2, regValue);
}

//FPGA的复位，寄存器2的bit3位先置高再置低
void fpgaReset()
{
    unsigned int regValue = fpga_read_16bits(FPGA_DDR_REG2);
    regValue |= (1 << 3);			//0x2_bit3

    fpga_write_16bits(FPGA_DDR_REG2, regValue);		//0x2_bit3 置高
	_msleep(5);
	regValue &= ~(1 << 3);
	fpga_write_16bits(FPGA_DDR_REG2, regValue);		//0x2_bit3 置低
	return 0;
}

void fpgaCrossCursorSet(crossCursorInfo_t* pInfo)
{
	unsigned short wordColorHigh, wordColorLow, cursorColorHigh, cursorColorLow;
	DBG("crossCursor.x = %d, crossCursor.y = %d\n", pInfo->x, pInfo->y);

	// 设置十字光标坐标
	fpga_write_16bits(FPGA_DDR_REG6, pInfo->x);
	fpga_write_16bits(FPGA_DDR_REG7, pInfo->y);
	fpga_write_16bits(FPGA_DDR_REGF, pInfo->x);

	cursorColorHigh = (unsigned short)((pInfo->crossCursorColor) >> 16)&0xFFFF;
	cursorColorLow = (unsigned short)(pInfo->crossCursorColor)&0xFFFF;
	wordColorHigh = (unsigned short)(pInfo->wordColor >> 16)&0xFFFF;
	wordColorLow = (unsigned short)(pInfo->wordColor)&0xFFFF;

	DBG("cursorColorHigh = 0x%x, cursorColorLow = 0x%x, wordColorHigh = 0x%x, wordColorLow = 0x%x\n",
		cursorColorHigh, cursorColorLow, wordColorHigh, wordColorLow);

	// 设置十字光标 颜色
	// 光标和字背景颜色
	fpga_write_16bits(FPGA_DDR_REG1E, cursorColorHigh);
	fpga_write_16bits(FPGA_DDR_REG1F, cursorColorLow);

	// 字颜色
	fpga_write_16bits(FPGA_DDR_REG1C, wordColorHigh);
    // 设置字体颜色，及设置当前的RGB的值
	unsigned short regValue = (wordColorLow & 0xfffc) | (pInfo->RGBchang & 0x03);
    DBG("tmp16 = 0x%x, RGBchang = 0x%x\r\n", regValue, pInfo->RGBchang);
    fpga_write_16bits(FPGA_DDR_REG1D, regValue);// 低2bit作为当前RGB的选择，0表示全部。

    regValue = fpga_read_16bits(FPGA_DDR_REG2);
	if (pInfo->enable)
	{
		regValue |= (1 << 8) | (1 << 4);	// 4 开始十字光标 8 显示像素信息
		if (pInfo->startCoordinate == 0)
			regValue &= ~(1 << 14);
		else
			regValue |= (1 << 14);
	}
	else
		regValue &= ~(1 << 8) & ~(1 << 4);

	fpga_write_16bits(FPGA_DDR_REG2, regValue);
	DBG( "FPGA_DDR_REG2 = 0x%x\n", regValue);

	return 0;
}

static int fpgaReg3ResetCheck(void)
{
	int flag = 0;

	while (1)
	{
		unsigned short regValue = fpga_read_16bits(FPGA_DDR_REG3);
		regValue = regValue & 1;
		if (0 != regValue)
			break;

		if (flag++ > 300)
			break;

		_usleep(100);
	}

    if (flag >= 300)
		return -1;

	return 0;
}

static void fpgaReg2Reset(void)
{
    unsigned short regValue = fpga_read_16bits(FPGA_DDR_REG2);
    regValue |= 0x1;				//0x2_bit0置1，表示对FPGA写复位
    fpga_write_16bits(FPGA_DDR_REG2, regValue);
    _msleep(1);
    regValue &= ~0x1;				//0x2_bit0置0，表示对FPGA写正常
    fpga_write_16bits(FPGA_DDR_REG2, regValue);
}

static void fpgaDdrChipSelection(bool bSelected)
{
	//FPGA_DDR设置   0x2_bit7写设置,0x2bit8、9读设置
	unsigned short regValue = fpga_read_16bits(FPGA_DDR_REG2);
	if (bSelected)
		regValue &= ~(1 << 7);	//0x2_bit7,write DDR select
	else
		regValue |= (1 << 7);

	fpga_write_16bits(FPGA_DDR_REG2, regValue);
}

int fpgaPictureDownloadConfig(fpgaDdrBufferInfo_t* pInfo)
{
    unsigned short regValue = 0;

	if (0 != fpgaReg3ResetCheck())
	{
		DBG_ERR( "ddrCs = %d, ddrCs Reg3 bit 3 can't write.\r\n", pInfo->ddrCs);
		return -1;
	}

    if (PICTURE_TYPE_JPG == pInfo->pictureType)		// JPG图片
    {
        DBG( "JPG Picture.\n");
        regValue = fpga_read_16bits(FPGA_DDR_REG3B);
        regValue = regValue | (1 << 12);
        fpga_write_16bits(FPGA_DDR_REG3B, regValue);
    }
    else		// BMP或逻辑画面
    {
        regValue = fpga_read_16bits(FPGA_DDR_REG3B);
        regValue = regValue | (1 << 13);
        fpga_write_16bits(FPGA_DDR_REG3B, regValue);
    }

	if (0 == pInfo->ddrCs)
    {
		// 当X坐标不足以整除32，则补全32， 显示时用
		unsigned short completion = 32 - (pInfo->hResolution % 32);
		if (32 == completion)
			completion = 0;

		fpga_write_16bits(FPGA_DDR_REG13, completion);
		fpgaDdrChipSelection(true);

		fpga_write_16bits(FPGA_DDR_REG5, pInfo->pictureNo|(pInfo->pictureSize << 8));

		fpgaReg2Reset();			// 写复位
        DBG("DDR0 %x %x\n",pInfo->pictureNo, pInfo->pictureSize << 8);
	}
	else
	{
		fpgaReg2Reset();
		fpgaDdrChipSelection(false);
		DBG( "DDR1\n");
	}

	return 0;
}

void fpgaLinkSet(unsigned int mode)
{
	unsigned short regValue = fpga_read_16bits(FPGA_DDR_REG1);

	switch (mode)
	{
		case 1:
			regValue &= ~(1 << 2);
			regValue &= ~(1 << 3);
            DBG( "LINK1\r\n");
			break;

		case 2:
			regValue |= (1 << 2);
			regValue &= ~(1 << 3);
			DBG( "LINK2\r\n");
			break;

		case 4:
			regValue |= (1 << 2);
			regValue |= (1 << 3);
			DBG( "LINK4\r\n");
			break;

		default:
			regValue |= (1 << 2);
			regValue &= ~(1 << 3);
			DBG( "default LINK2\r\n");
			break;
	}

	fpga_write_16bits(FPGA_DDR_REG1, regValue);
	DBG( "Link reg = %d\r\n", regValue);
}

void fpgaSeqSet(fpgaSeqInfo_t* pInfo)
{
	DBG( "fpgaSeq.T0 = %d\n", pInfo->T0);
	DBG( "fpgaSeq.T1 = %d\n", pInfo->T1);
	DBG( "fpgaSeq.T2 = %d\n", pInfo->T2);
	DBG( "fpgaSeq.T3 = %d\n", pInfo->T3);
	DBG( "fpgaSeq.T4 = %d\n", pInfo->T4);
	DBG( "fpgaSeq.T5 = %d\n", pInfo->T5);
	DBG( "fpgaSeq.T6 = %d\n", pInfo->T6);
	DBG( "fpgaSeq.T7 = %d\n", pInfo->T7);

	fpga_write_16bits(HSD_BACK_PORCH, pInfo->T0);
	fpga_write_16bits(HSD_PLUSE, pInfo->T1);
	fpga_write_16bits(HSD_FRONT, pInfo->T2);
	fpga_write_16bits(HSD_DISPLAY, pInfo->T3);
	fpga_write_16bits(VSD_BACK_PORCH, pInfo->T4);
	fpga_write_16bits(VSD_PLUSE, pInfo->T5);
	fpga_write_16bits(VSD_FRONT, pInfo->T6);
	fpga_write_16bits(VSD_DISPLAY, pInfo->T7);
}

void fpgaBitSet(int bits)
{
	unsigned short regValue = fpga_read_16bits(FPGA_DDR_REG1);
	DBG( "setFpgaBit reg = %d\n", regValue);

	switch (bits)
	{
		case 6:
			regValue |= 1;
			regValue &= ~(1 << 1);
			DBG( "Bit6\r\n");
			break;

		case 8:
			regValue &= ~(1 << 0);
			regValue |= (1 << 1);
			DBG( "Bit8\r\n");
			break;

		case 10:
			regValue |= VBIT10;
            DBG( "Bit10\r\n");
			break;

		default:
			regValue |= VBIT10;
            DBG( "default Bit10\r\n");
			break;
	}

	fpga_write_16bits(FPGA_DDR_REG1, regValue);
	DBG( "setFpgaBit reg = %d\n", regValue);
}

void fpgaImageMove(imageMoveInfo_t* pInfo)
{
	unsigned short regB, reg2B;
	unsigned char xstep, ystep;

	xstep = pInfo->Xmsg >> 16;
	ystep = pInfo->Ymsg >> 16;
	regB = (pInfo->Xmsg & 0x7) | ((pInfo->Ymsg & 0x7) << 3) | (xstep << 6);
	reg2B = ystep << 6;

	fpga_write_16bits(FPGA_DDR_REGB, regB);
	fpga_write_16bits(FPGA_DDR_REG2B, reg2B);
	return 0;
}

void fpgaSignalModeSet(signalModeInfo_t* pInfo)
{
	unsigned short regValue = fpga_read_16bits(FPGA_DDR_REG1);

	regValue &= 0xFF;
	regValue |= (pInfo->link0 & 0x3) << 8;
	regValue |= (pInfo->link1 & 0x3) << 10;
	regValue |= (pInfo->link2 & 0x3) << 12;
	regValue |= (pInfo->link3 & 0x3) << 14;

	fpga_write_16bits(FPGA_DDR_REG1, regValue);
}

int fpgaFreqSet (freqConfigInfo_t* pInfo)
{
	unsigned short regValue = ((pInfo->d1&0xff) << 8) | (pInfo->m & 0xff);
	fpga_write_16bits(FPGA_DDR_REG26, regValue);
	fpga_write_16bits(FPGA_DDR_REG27, (pInfo->d0 & 0xff) << 8);

	_msleep(1);

	regValue = fpga_read_16bits(FPGA_DDR_REG3);

	unsigned int tmp = 0;
	while (!((regValue >> 4)&0x1))
	{
		regValue = fpga_read_16bits(FPGA_DDR_REG3);
		tmp++;
		if (tmp > 0xffff)
		{
			DBG_ERR("freq timeout\n");
			return 1;
		}
    }

	regValue = fpga_read_16bits(FPGA_DDR_REG2);
	regValue |= ((1 << 9) | (1 << 11));			//0x2_bit9位先置高再置低，高电平保持>200ns
	fpga_write_16bits(FPGA_DDR_REG2, regValue);

	_usleep(100);

	regValue = fpga_read_16bits(FPGA_DDR_REG2);
	regValue &= ~((1 << 9) | (1 << 11));
	fpga_write_16bits(FPGA_DDR_REG2, regValue);

	return 0;
}

void fpgaShowRgb(showRgbInfo_t* pInfo)
{
	unsigned short regValue = 0;
	// 设定颜色
	regValue = (pInfo->rgb_r << 2) + (pInfo->rgb_g << 12);
	fpga_write_16bits(FPGA_DDR_REG11, regValue);

	regValue = (pInfo->rgb_b << 6) + (pInfo->rgb_g >> 4);
	fpga_write_16bits(FPGA_DDR_REG10, regValue);
    // 使能RGB调节
	regValue = fpga_read_16bits(FPGA_DDR_REG2);
	regValue |= (1 << 10);
	fpga_write_16bits(FPGA_DDR_REG2, regValue);
}

void fpgaVesaJedaSwitch(vesaJedaSwitchInfo_t* pInfo)
{
	unsigned short regValue = fpga_read_16bits(FPGA_DDR_REG1);
	switch (pInfo->lvdsChange)
	{
		case VESA:
			regValue &= ~(1 << 7);
            DBG( "VESA mode \r\n");
			break;

		case JEDA:
			regValue |= (1 << 7);
            DBG( "JEDA mode \r\n");
			break;

		default:
			regValue &= ~(1 << 7);
            DBG( "default VESA mode \r\n");
			break;
	}

	fpga_write_16bits(FPGA_DDR_REG1, regValue);
}

void fpgaSignalSyncLevel(syncSingalLevelInfo_t* pInfo)
{
	pInfo->sync_pri_v = pInfo->sync_pri_h;
	unsigned short regValue = fpga_read_16bits(FPGA_DDR_REG1);
	if (0 == pInfo->sync_pri_h)
		regValue &= ~(1 << 5);
	else
		regValue |= (1 << 5);

	if (0 == pInfo->sync_pri_v)
		regValue &= ~(1 << 6);
	else
		regValue |= (1 << 6);

	if (0 == pInfo->de)
		regValue &= ~(1 << 4);
	else
		regValue |= (1 << 4);

	fpga_write_16bits(FPGA_DDR_REG1, regValue);
}

//#define USE_NEW_DATA_TRNA
#ifndef USE_NEW_DATA_TRAN
#define MAX_SIZE	(320*1024)
#endif

int fpgaTransPicData(const char *pName, unsigned int dataSize)
{
    char *pTemp = 0;
    int  fileSize  = 0;
    int needBytes = 0;

    loadPtnToMem(pName,&pTemp,&fileSize);

#ifndef USE_NEW_DATA_TRNA
    if (fileSize > dataSize*SIZE_1_MB)
    	needBytes = fileSize;
    else
    	needBytes = dataSize*SIZE_1_MB;

    char* pNeedData = (char *)calloc(1, needBytes);
    memcpy(pNeedData, pTemp, fileSize);
#else
    needBytes = fileSize - BMP_PIC_HEAD_LEN;
    char* pNeedData = (char *)calloc(1, needBytes);
    memcpy(pNeedData, pTemp+BMP_PIC_HEAD_LEN, needBytes);
#endif
    free(pTemp);

    printf("fileSize: %d, needBytes: %d\r\n", fileSize, needBytes);
    pTemp = pNeedData;

#ifdef USE_NEW_DATA_TRNA
    fpgaCtrlAccessBurstWrite(pTemp, needBytes);
#else
    while(needBytes>0)
     {
         int s32WriteSize,actWriteSize = 0;

         int onceTransmitSize = needBytes > MAX_SIZE ? MAX_SIZE: needBytes;
         while(onceTransmitSize > 0)
         {
             int sndBytes = fpgaCtrlAccessBurstWrite(pTemp, onceTransmitSize);
             if(sndBytes < 0)
             {
                 free(pNeedData);
                 return -2;
             }

             onceTransmitSize -= sndBytes;
             pTemp += sndBytes;
         }
     }
#endif

    free(pNeedData);
	return 0;
}

void fpga_write(unsigned int reg, unsigned int val)
{
	fpgaCtrlAccessWrite(reg, val);
}

void fpga_write_16bits(unsigned short reg, unsigned short val)
{
	fpgaCtrlAccessWrite16Bits(reg, val);
}

unsigned short fpga_read(unsigned int reg)
{
	return fpgaCtrlAccessRead(reg);
}

unsigned short fpga_read_16bits(unsigned short reg)
{
	return fpgaCtrlAccessRead16Bits(reg);
}

void setCurrentPicture(int index)
{
	unsigned int width,height;
	char imageName[256];
	module_info_t *pModuleInfo = dbModuleGetPatternById(index);
	if(!pModuleInfo)
	{
		printf("!!!!!!!!!!pls notice err ptnId is %d\n",index);
		return -1;
	}

	sprintf(imageName,"./cfg/imageOrigin/%s/%s.bmp",pModuleInfo->displayX,pModuleInfo->ptdpatternname);

	if(access(imageName , F_OK)!=0)
	{
		printf("!image is not exist:%s\n",imageName);
		return -1;
	}

	sscanf(pModuleInfo->displayX,"%dX%d",&width,&height);
//	fpgaSetPictureState(false);
	int ucPosition = move_ptn_download(imageName,width,height,0);
#if 0
	fpgaSetPictureState(true);
#else
	int totalSize = width*height*4;
	if (totalSize%SIZE_1_MB != 0)
		totalSize = totalSize/SIZE_1_MB + 1;
	else
		totalSize= totalSize/SIZE_1_MB;

	fpgaShowPicture(ucPosition, totalSize);
#endif
}

void setCrossCursorState(CrossCursorStateStr* pCrossCurSor)
{
	crossCursorInfo_t info = {0};
	switch (pCrossCurSor->RGBchang)
	{
		case 1: //userCrossCursorR
			pCrossCurSor->crossCursorColorGreen = (~pCrossCurSor->crossCursorColorGreen) & 0xFF;
			pCrossCurSor->crossCursorColorBlue = (~pCrossCurSor->crossCursorColorBlue) & 0xFF;
			break;

		case 2: //userCrossCursorG
		{
			pCrossCurSor->crossCursorColorRed = (~pCrossCurSor->crossCursorColorRed) & 0xFF;
			pCrossCurSor->crossCursorColorGreen = (~pCrossCurSor->crossCursorColorGreen) & 0xFF;
			pCrossCurSor->crossCursorColorBlue = (~pCrossCurSor->crossCursorColorBlue) & 0xFF;
		}
		break;

		case 3: //userCrossCursorB
		{
			pCrossCurSor->crossCursorColorRed = (~pCrossCurSor->crossCursorColorRed) & 0xFF;
			pCrossCurSor->crossCursorColorGreen = (~pCrossCurSor->crossCursorColorGreen) & 0xFF;
		}
		break;
	}

	info.x = pCrossCurSor->x;
	info.y = pCrossCurSor->y;
	info.enable = pCrossCurSor->enable;
	info.HVflag = pCrossCurSor->HVflag;
	info.RGBchang = pCrossCurSor->RGBchang;
	info.startCoordinate = pCrossCurSor->startCoordinate;
	info.crossCursorColor = (pCrossCurSor->crossCursorColorRed << 2) | (pCrossCurSor->crossCursorColorGreen << 12) | (pCrossCurSor->crossCursorColorBlue << 22);
	info.wordColor = ((~pCrossCurSor->crossCursorColorRed)&0xff << 2) | ((~pCrossCurSor->crossCursorColorGreen)&0xff << 12) | ((~pCrossCurSor->crossCursorColorBlue)&0xff << 22);
	fpgaCrossCursorSet(&info);
}

void setFpgaSignalMode(lvdsSignalModuleStr* pLvdsSignal)
{
	signalModeInfo_t signalMode = {0};

	switch (pLvdsSignal->linkCount)
	{
		case 2:
			signalMode.link0 = pLvdsSignal->module[0] - 1;
			signalMode.link1 = pLvdsSignal->module[1] - 1;
			signalMode.link2 = pLvdsSignal->module[0] - 1;
			signalMode.link3 = pLvdsSignal->module[1] - 1;
			break;
		default:
			signalMode.link0 = pLvdsSignal->module[0] - 1;
			signalMode.link1 = pLvdsSignal->module[1] - 1;
			signalMode.link2 = pLvdsSignal->module[2] - 1;
			signalMode.link3 = pLvdsSignal->module[3] - 1;
			break;
	}

	fpgaSignalModeSet(&signalMode);
}

int initFpga(void)
{
    int ret = fpgaCtrlAccessInit();
    if (ret < 0)
    {
    	DBG_ERR("fpgaCtrlAccessInit failed.\r\n");
    	return ret;
    }

    fpgaEnable();
}
