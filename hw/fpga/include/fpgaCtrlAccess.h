/*
 * fpgaCtrlAccess.h
 *
 *  Created on: 2018-8-19
 *      Author: tys
 */

#ifndef FPGACTRLACCESS_H_
#define FPGACTRLACCESS_H_

#include "semApi.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 *FPGA REG1 寄存器说明
 *bit0~bit1:图形量化位宽(01:6bit；10:8bit；11:10bit)。只写不能读。
 *bit2~bit3:link数选择(00:单link；01:双link；11:4link)
 *bit4 DE信号电平 poloar
 *bit5 行同步信号电平  poloar
 *bit6 场同步电平 poloar
 *bit7 0:vasa 1:jeida
 *bit8~bit15 控制输出信号连接模式，每个link均可以配置为0、1、2、3中的一种;
 *其中link0由bit9~bit8控制，link1由bit11~bit10控制，link2由bit13~bit12控制，link3由bit15~bit14控制；
*/
#define FPGA_DDR_REG1 (0x02)

/*
* FPGA REG2寄存器说明
* bit0.写复位寄存器(0:ok,1:复位)
* bit3.全局FPGA复位；(先高再低)
* bit4.十字光标开关(0:不显示,1:显示)
* bit5.图片显示使能(0:不显示，1:显示)
* #bit6.triger下降沿，触发后判断0x3_bit2;
* bit6.3D引脚开关 1开 0关
* bit7.DDR写选择；bit8,pixel信息显示开关，0，关，1，开;
* Bit8.十字光标像素点信息显示开关。0，开；1，关.
* bit9.触发屏幕分频，低高低，高电平保持>200ns;
* Bit10.RGB画面显示开关。1，显示。0，关闭。
* Bit11.ARM启动标识寄存器。1，启动；0，未启动
* bit12.FPGA时钟开关 0关闭 1开始
* Bit13,图片移动停止位。1、停止，0，继续移动。
* Bit14，十字光标信息原点坐标选择位。1，原点（1，1）；0，原点（0，0）
* bit15 LVDS信号开关 0关 1开
*/
#define FPGA_DDR_REG2 (0x04)

/*
* FPGA REG3寄存器说明
* bit0.FPGA复位检测，1，复位完毕，0，复位执行中。
* bit1 多分配发射命令，写1 发射 检测跳变到0时，发射完毕
* bit2.写有效(0:不可写,1:可写);
* bit3.写入数据是否写完 0未写完 1写完
* bit4,判断屏幕频率配置是否有效，1:完毕，0:等待。
* bit5.接收缓存器空指示
* bit0~bit7 多分配时从端地址。00无效 FF广播 bit0~bit3 级联级数地址
* bit8-bit15 消息类型
 */
#define FPGA_DDR_REG3 (0x06)

/*
* FPGA REG4寄存器说明
* 显示图形编号寄存器，
* bit0~bit7位，图形编号0~255；
* Bit11~Bit8,图形尺寸，(k+1)Mbytes
* 多分配响应 8位地址加1位响应标志
*/
#define FPGA_DDR_REG4 (0x08)

/*
* FPGA REG5寄存器说明
* 写入图形编号寄存器,
* bit7~bit0位，图形编号0~255；
* Bit11~Bit8,图形尺寸，(k+1)Mbytes
*/
#define FPGA_DDR_REG5 (0x0A)

/*
* FPGA REG6寄存器说明
* 十字光标的水平(X)坐标
*/
#define FPGA_DDR_REG6 (0x0C)

/*
* FPGA REG7寄存器说明
* 十字光标的垂直(Y)坐标
*/
#define FPGA_DDR_REG7 (0x0E)

/*
* FPGA REG8寄存器说明
* 水平偏移寄存器
*/
#define FPGA_DDR_REG8 (0x10)

/*
* FPGA REG9寄存器说明
* 垂直偏移寄存器
*/
#define FPGA_DDR_REG9 (0x12)

/*
* FPGA REGA寄存器说明
* FPGA 测试寄存器 ARM不使用
*/
#define FPGA_DDR_REGA (0x14)

/*
* FPGA REGB寄存器
* 图形移动方式寄存器
*	[5:3]	[2:0]
*	000		000		停止
*
*	001		000		上			FPGA RGE 2B [13:6]
*	010		000		下
*	011		000		垂直
*
*	000		001		左			FPGA RGE B	[13:6]
*	000		010		右
*	000		011		水平
*
*	011		011		中心
* bit0~bit5图形变化方式寄存器
* Bit6~bit13，R/W。图形移动速度寄存器。
* 取值范围0~255。按照数值的增大而变快。
* 移动的速度是 (屏幕刷新频率)*(移动速度-1)
 */
#define FPGA_DDR_REGB (0x16)

/*
* FPGA REGC寄存器
* FPGA程序版本寄存器。
* 只读，格式：x.x.xx。由高到低位。每个x代表4个2进制位，BCD编码。
*/
#define FPGA_DDR_REGC (0x18)

/*
* FPGA REGD寄存器
* FPGA发布时间，年寄存器。
* 只读，BCD编码，由高到低，依次为千、百、十、个位
*/
#define FPGA_DDR_REGD (0x1A)

/*
* FPGA REGE寄存器
* FPGA发布时间，月、日寄存器。
* 只读，BCD编码，高两位数值为月，低两位数值为日。
*/
#define FPGA_DDR_REGE (0x1C)
#define FPGA_DDR_REGF (0x1E)

/*
* FPGA REG10-REG11寄存器
* RGB纯色画面颜色寄存器
* 这两个寄存器组成一个32bits的逻辑画面颜色寄存器。只写。
* FPGA_DDR_REG10为高16bits寄存器，FPGA_DDR_REG11为低16bits寄存器。
* Bit31~bit22为颜色分量蓝色，取值范围0~1023。
* Bit21~bit12为颜色分量绿色，取值范围0~1023。
* Bit11~bit2为颜色分量红色，取值范围0~1023。
* Bit1~bit0，保留。
* 初始值：
*    FPGA_DDR_REG10 = 0;
*    FPGA_DDR_REG11 = 0;
*/
#define FPGA_DDR_REG10 (0x20)
#define FPGA_DDR_REG11 (0x22)

#define FPGA_DDR_REG12 (0x24)

// FPGA REG13寄存器说明
// Bit0~bit12，有效，R/W。
// Bit4~bit0，像素点补足位，用于补足横轴像素点，在单LINK（1366*768）有使用。
// Bit7~bit5，消色选择位。
// 7,不变；1.消R；2，消G；3，消B；4，消RG；5，消RB；6，消GB；0，消RGB。
// Bit8: 反色位。
// 1、	反色。0，正常显示。
// Bit9：颜色分量Blue增量方向位。1，灰阶值增加；0，灰阶值减小。
// Bit10:颜色分量Green增量方向位。1，灰阶值增加；0，灰阶值减小。
// Bit11: 颜色分量Red增量方向位。1，灰阶值增加；0，灰阶值减小。
// Bit12：灰阶使能位。1、使能。0，禁止使用灰阶调色功能。
#define FPGA_DDR_REG13  (0x26)

// FPGA显示时序寄存器
#define HSD_BACK_PORCH 	(0x28)	// 水平后肩
#define HSD_PLUSE 		(0x2A)	// 水平后肩 + 水平同步
#define HSD_FRONT 		(0x2C)	// 水平后肩 + 水平同步 + 水平前肩
#define HSD_DISPLAY 	(0x2E)	// 水平后肩 + 水平同步 + 水平后肩 + 水平有效图像
#define VSD_BACK_PORCH 	(0x30)	// 垂直后肩
#define VSD_PLUSE 		(0x32)	// 垂直后肩 + 垂直同步
#define VSD_FRONT 		(0x34)	// 垂直后肩 + 垂直同步 + 垂直后肩
#define VSD_DISPLAY 	(0x36)	// 垂直后肩 + 垂直同步 + 垂直后肩 + 垂直有效图像

/*
* FPGA REG1C～REG1D
* 十字光标线色寄存器
* 这两个寄存器组成一个32bits的十字光标像素点信息字体颜色寄存器，只写。
* FPGA_DDR_REG1C为高16bits寄存器，FPGA_DDR_REG1D为低16bits寄存器。
* Bit31~bit22为颜色分量蓝色，取值范围0~1023。
* Bit21~bit12为颜色分量绿色，取值范围0~1023。
* Bit11~bit2为颜色分量红色，取值范围0~1023。
* Bit1~bit0，保留。
*/
#define FPGA_DDR_REG1C (0x38)
#define FPGA_DDR_REG1D (0x3A)

/*
* FPGA REG1E~REG1F
* 十字光标点信息背景色寄存器
* 这两个寄存器组成一个32bits的十字光标背景颜色寄存器，只写。
* FPGA_DDR_REG1E为高16bits寄存器，FPGA_DDR_REG1F为低16bits寄存器。
* Bit31~bit22为颜色分量蓝色，取值范围0~1023。
* Bit21~bit12为颜色分量绿色，取值范围0~1023。
* Bit11~bit2为颜色分量红色，取值范围0~1023。
* Bit1~bit0，保留
*/
#define FPGA_DDR_REG1E (0x3C)
#define FPGA_DDR_REG1F (0x3E)

/*
* 多分配是否响应
* bit 0~7 地址
* bit 15 是响应标志 1有效 bit 14 1成功 0 失败
*/
#define FPGA_DDR_REG20 (0x40)

/*
* FPGA REG1E~REG1F
* 灰阶RGB颜色调整寄存器。R/W。
* REG21的bit7~bit0，红色增量值。
* Bit15~bit8，绿色增量值。
* REG22的bit7~bit0，蓝色增量值。
*/
#define FPGA_DDR_REG21 (0x42)
#define FPGA_DDR_REG22 (0x44)

/*
 * FPGA 串口buff，低8bit有效，R/W
*/
#define FPGA_DDR_REG23 (0x46)

/*
 * 接收数据时，串口buff缓存计数寄存器，低10bit有效。串口缓存为1024字节，只读。
*/
#define FPGA_DDR_REG24 (0x48)

/*
 *发送数据时，串口buff缓存计数寄存器，低10bit有效。串口缓存为1024字节，只读。
*/
#define FPGA_DDR_REG25 (0x4A)

/*
* bit0 : 视频BMP模式切换
* bit1 : 逻辑画面开关
*/
#define FPGA_DDR_REG3B (0x76)

/*
* 时钟控制寄存器
* Bit0~bit15有效。
* Bit7~bit0，倍频系数M。
* Bit15~bit8，分频系数d1.
*/
#define FPGA_DDR_REG26 (0x4C)

/*
* 时钟控制寄存器
* Bit15~bit8有效。
* Bit15~bit8 分频系数d0。
*/
#define FPGA_DDR_REG27 (0x4E)

/*
 * [13:6] 图形移动X方向步进
*/
#define FPGA_DDR_REG2B (0x56)

#define FPGA_CLOCK_REGA4 (0x148)
#define FPGA_CLOCK_REGA5 (0x14a)
#define FPGA_CLOCK_REGA6 (0x14c)
#define FPGA_CLOCK_REGA8 (0x150)

#define FPGA_CTRL_DEV			"/dev/hello"
#define FPGA_PHY_ADDR_SIZE		(0x10000)
#define FPGA_PHY_ADDR_BASE  	(0x43c00000)

typedef struct fpgaCtrlAccess
{
	int fd;
	unsigned short* pBaseAddr;
	semM_t* mutex;
}fpgaCtrlAccess_t;

int fpgaCtrlAccessInit();

void fpgaCtrlAccessDeinit();

void fpgaCtrlAccessWrite16Bits(unsigned int addr, unsigned short value);

unsigned short fpgaCtrlAccessRead16Bits(unsigned int addr);

void fpgaCtrlAccessWrite(unsigned int addr, unsigned int value);

unsigned int fpgaCtrlAccessRead(unsigned int addr);

int fpgaCtrlAccessBurstWrite(void* pData, int length);
#ifdef __cplusplus
}
#endif
#endif /* FPGACTRLACCESS_H_ */
