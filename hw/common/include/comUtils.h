#ifndef __COM_UTILS_H__
#define __COM_UTILS_H__

#include "comStruct.h"
#include "common.h"

#define MAX_CFG_BUF 512


#define IBS(pos) 0x01<<((pos))                      
#define SET_BIT(number,pos) ((number)|= IBS(pos))   //设置一个数的二进制值
#define CLR_BIT(x,y)   x&=~(1<<y)

#define BitGet(Number,pos) ((Number) >> (pos)&1)   //判断一个数的二进制pos位是否为1


// 把ASCII码转换为16进制数值
INT stringToChar(BYTE* ch, CHAR *source, INT len); 
/* 一个数的二进制为1的个数*/
INT getBit(WORD32 dwType, BYTE BitSet[], WORD32 dwNum);
/* 获取当前系统时间*/
void getTime(BYTE *pStrTime);
/* 设置时间*/
void setTime(BYTE *pTime);
/*读配置文件 ,从配置文件中获取某个项目的字符串值*/
INT getProfileStr(FILE *pFd, BYTE *pSectionName, BYTE *pVal, INT lenth);
/*读配置文件 ,从配置文件中获取某个项目的整数值*/
INT getProfileInt(FILE *pFd,  BYTE *pSectionName, INT *pVal);
/*读配置文件 ,从配置文件中获取某个项目的短整数值*/
INT getProfileShort(FILE *pFd,  BYTE *pSectionName, SHORT *pVal);
/*读配置文件 ,从配置文件中获取某个项目的长整数值*/
INT getProfileLong(FILE *pFd, BYTE *pSectionName, unsigned long *pVal);
/*向配置文件中设置某个项目的值，为字符串*/
INT setProfileStr(BYTE *pPath, BYTE *pSectionName, BYTE *pVal);
/*根据关键字段与域值，从文件读取对应的值*/
INT configGetKey(BYTE *pFilePath, BYTE *pSection, BYTE *pKey, BYTE *pOutBuf);
INT getStringStr(const BYTE *string, const BYTE *subString, BYTE *value);
INT getStringInt(const BYTE *string, const BYTE *subString, INT *value);
INT regCmdFun(T_CMD_PROCFUN *pSrcCmdFun, T_CMD_PROCFUN *pOutCmdFun);
void hexMemToString(BYTE *pFileDataAscii, BYTE *pFileDataHex, WORD32 ulFileLenHex);
void outputMsg(INT nMask, BYTE *pucMsg, WORD32 dwLen);
WORD16 charToAsc(BYTE cTemp);
CHAR hexToAscOneByte(CHAR cValue);
FILE *openFile(CHAR *pFileName);
SHORT getCfgfileShort(FILE *Fd,  BYTE *SectionName, WORD16 *Val);
SHORT getCfgfileStr(FILE *Fd, BYTE *SectionName, BYTE *val,INT lenth);
void setRunLogInfo(BYTE *pSectionName);

#endif


