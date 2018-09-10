
#ifndef _FPGA_FUN_H_
#define _FPGA_FUN_H_

#ifdef __cplusplus
extern "C" {
#endif

#define FPGA_VERSION_H			4
#define FPGA_VERSION_M			1
#define FPGA_VERSION_L			41

#define LINK1						(~0x03)
#define LINK2						(0x01 << 2)
#define LINK4						(0x03 << 2)
#define VBIT6						(0x01)
#define VBIT8						(0x02)
#define VBIT10						(0x03)


enum fpgaReturnValue
{	// 返回值
	fpgaOK,						// 正常的返回值
	fpgaEnableReadyError,		// 预备下载FPGA错误
	fpgaEnableRightError,		// 下载FPGA失败
	fpgaSetShowModeError,		// FPGA模式设置
	fpgaResetError,				// fpga复位操作
	fpgaResetTestError,			// fpga复位测试
	fpgaSetCrossCursorError,	// 设置十字光标
	fpgaDownPictureError,		// 下载图片
};

enum e_FpgaCommand
{	// fpga操作命令字
	eFPGA_SUPER_CMD,		// 0x00.保留
	eSET_CLOCK,				// 0x0e.设置FPGA 时钟
    eSET_FPGA_REG_BIT,		// 0x1d.设置FPGA寄存器位操作
};


#pragma pack(push)
#pragma pack(1)

typedef struct
{
	unsigned short offset;	// 寄存器偏移地址
	unsigned short value;	// 寄存器值
}fpgaRegisterInfo_t;

typedef struct
{
	unsigned short year;		// xxxx
	unsigned char mmdd[2];		// xx-xx
	unsigned char version[2];	// x.x.xx

	unsigned char userVersion[3];		// FPGA用户版本
	unsigned char kernelVersion[3];		// FPGA内核版本
}fpgaVersionInfo_t;

typedef struct
{
	unsigned char enable;			// 十字光标使能
	unsigned short x;				//
	unsigned short y;				//
	unsigned int wordColor;			// 字颜色
	unsigned int crossCursorColor;	// 光标颜色
	unsigned char RGBchang;			// RGB步进时用的颜色,0:ALL 1 R; 2 G; 3 B.
	unsigned char HVflag;			// 选择是横向还是纵向的RGB点
	unsigned char startCoordinate;	// 保留位
}crossCursorInfo_t;

typedef struct
{
    unsigned char enable;
    unsigned short x;
    unsigned short y;
    unsigned short crossCursorColorRed;
    unsigned short crossCursorColorGreen;
    unsigned short crossCursorColorBlue;
    unsigned char RGBchang;		    // RGB步进时用的颜色,0:ALL 1 R; 2 G; 3 B.
    unsigned char HVflag;			// 选择是横向还是纵向的RGB点
    unsigned char startCoordinate;	// 选择起始坐标 0开始还是1开始
}CrossCursorStateStr;

// 设置模组链接信息
typedef struct
{
    unsigned char linkCount;	// link数
    unsigned char module[4];	// 链接状态
}lvdsSignalModuleStr;

typedef struct
{
#define PICTURE_TYPE_BMP			0
#define PICTURE_TYPE_LOGIC			0
#define PICTURE_TYPE_JPG			1

	unsigned char ddrCs;			// DDR片选
	unsigned char last;				// 图分次下载标志
	unsigned char *buffer;			// 图信息缓冲区
	int fileLength;					// 图分次下载长度
	unsigned short hResolution; 	// 图水平分辨率
	unsigned short vResolution;		// 图垂直分辨率
	unsigned char pictureNo;		// 图序号
	unsigned char pictureSize;		// 图大小
	unsigned char pictureType;		// 图类型
	unsigned char reserve[3];		// 保留
}fpgaDdrBufferInfo_t;

typedef struct
{
	unsigned short T0;		// hsdForntPorch
	unsigned short T1;		// hsdForntPorch + hsdSyncPures
	unsigned short T2;		// hsdForntPorch + hsdSyncPures + hsdBackPorch
	unsigned short T3;		// hsdForntPorch + hsdSyncPures + hsdBackPorch + hsdDisp
	unsigned short T4;		// vsdForntPorch
	unsigned short T5;		// vsdForntPorch + vsdSyncPures
	unsigned short T6;		// vsdForntPorch + vsdSyncPures + vsdBackPorch
	unsigned short T7;		// vsdForntPorch + vsdSyncPures + vsdBackPorch + vsdDisp
}fpgaSeqInfo_t;

typedef struct
{
	/*	[5:3]	[2:0]
	000		000		停止

	001		000		上			FPGA RGE 2B [13:6]
	010		000		下
	011		000		垂直

	000		001		左			FPGA RGE B	[13:6]
	000		010		右
	000		011		水平

	011		011		中心
	*/

	unsigned int Xmsg;		// [31:16] X方向 [15:0]X步进
	unsigned int Ymsg;		// [31:16] Y方向 [15:0]Y步进
}imageMoveInfo_t;

typedef struct
{
	unsigned char link0;
	unsigned char link1;
	unsigned char link2;
	unsigned char link3;
}signalModeInfo_t;

typedef struct
{
    unsigned short rgb_r;
    unsigned short rgb_g;
    unsigned short rgb_b;
}showRgbInfo_t;

typedef struct
{
	char d0;		// 分频系数0
	char m;			// 倍频系数
	char d1;		// 分频系数1
}freqConfigInfo_t;

typedef struct
{
	unsigned char lvdsChange;

#define VESA 0
#define JEDA 1
}vesaJedaSwitchInfo_t;

typedef struct _SYNC_SINGAL_LEVEL_
{
	unsigned char sync_pri_h;		// 行同步信号电平
	unsigned char sync_pri_v;		// 场同步电平
	unsigned char de;				// DE信号电平
}syncSingalLevelInfo_t;

int fpgaTransPicData(const char *pName, unsigned int dataSize);

void fpga_write(unsigned int reg, unsigned int val);

unsigned short fpga_read(unsigned int reg);

void fpga_write_16bits(unsigned short reg, unsigned short val);

unsigned short fpga_read_16bits(unsigned short reg);

void fpgaEnable();

fpgaVersionInfo_t fpgaGetVersion();

void fpgaShowPicture(unsigned short position, unsigned short size);

void fpgaReset();

void fpgaCrossCursorSet(crossCursorInfo_t* pInfo);

int fpgaPictureDownloadConfig(fpgaDdrBufferInfo_t* pInfo);

void fpgaLinkSet(unsigned int mode);

void fpgaSeqSet(fpgaSeqInfo_t* pInfo);

void fpgaBitSet(int bits);

void fpgaImageMove(imageMoveInfo_t* pInfo);

void fpgaSignalModeSet(signalModeInfo_t* pInfo);

int fpgaFreqSet (freqConfigInfo_t* pInfo);

void fpgaShowRgb(showRgbInfo_t* pInfo);

void fpgaVesaJedaSwitch(vesaJedaSwitchInfo_t* pInfo);

void fpgaSignalSyncLevel(syncSingalLevelInfo_t* pInfo);

int initFpga(void);

#ifdef __cplusplus
}
#endif
#endif
