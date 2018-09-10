
#if 0
#include "common.h"
#include "comStruct.h"
#include "comUtils.h"

CHAR hexToAscOneByte(CHAR cValue)
{
    CHAR cRtn = 0;

    if ((cValue <= 9) && (cValue >= 0))
    {
        cRtn	=	cValue + '0';
    }
    else if ((cValue <= 0xf) && (cValue >= 0xa))
    {
        cRtn	=	cValue + 'a' - 0xa;
    }
    else
    {
        /*数值非法*/
    }

    return(cRtn);
}


WORD16 charToAsc(BYTE cTemp)
{
    WORD16	usTemp;
    BYTE	cFirst4Bit;
    BYTE	cSecond4Bit;
    BYTE	cFirst4BitChar;
    BYTE	cSecond4BitChar;
    cFirst4Bit	= ((cTemp & 0xf0) >> 4);
    cSecond4Bit	= (cTemp & 0x0f);
    cFirst4BitChar	=	hexToAscOneByte(cFirst4Bit);
    cSecond4BitChar	=	hexToAscOneByte(cSecond4Bit);
    usTemp	=	cFirst4BitChar + cSecond4BitChar * 16 * 16;
    return(usTemp);
}

INT stringToChar(BYTE *ch, CHAR *source, INT len)
{
    BYTE tmpchar;
    CHAR *chartmp = malloc(len);
    INT i;

    for (i = 0; i < len; i++)
    {
        tmpchar = source[i];

        switch (tmpchar)
        {
            case '0':
                tmpchar = 0;
                break;
            case '1':
                tmpchar = 1;
                break;
            case '2':
                tmpchar = 2;
                break;
            case '3':
                tmpchar = 3;
                break;
            case '4':
                tmpchar = 4;
                break;
            case '5':
                tmpchar = 5;
                break;
            case '6':
                tmpchar = 6;
                break;
            case '7':
                tmpchar = 7;
                break;
            case '8':
                tmpchar = 8;
                break;
            case '9':
                tmpchar = 9;
                break;
            case 'A':
            case 'a':
                tmpchar = 0x0a;
                break;
            case 'B':
            case 'b':
                tmpchar = 0x0b;
                break;
            case 'C':
            case 'c':
                tmpchar = 0x0c;
                break;
            case 'D':
            case 'd':
                tmpchar = 0x0d;
                break;
            case 'E':
            case 'e':
                tmpchar = 0x0e;
                break;
            case 'F':
            case 'f':
                tmpchar = 0x0f;
                break;
        }

        memcpy(chartmp + i, &tmpchar, 1);
    }

    for (i = 0; i < len / 2; i++)
    {
        tmpchar = (*(chartmp + i * 2) << 4) + chartmp[i * 2 + 1];
        memcpy(ch + i, &tmpchar, 1);
    }

    free(chartmp);
    chartmp = NULL;
    return len / 2;
}

void hexMemToString(BYTE *pFileDataAscii, BYTE *pFileDataHex, WORD32 ulFileLenHex)
{
    INT i;
    BYTE	cTemp;
    WORD16	usTemp;
    BYTE	*pTemp1 = NULL;
    BYTE	*pTemp2 = NULL;
    /*指向ASCII文件的指针*/
    pTemp2	=	pFileDataAscii;

    for (i = 0; i < ulFileLenHex; i++)
    {
        /*依次指向HEX文件的每个字节*/
        pTemp1 = pFileDataHex + i;
        /*取得每个字节的值*/
        cTemp	= *pTemp1;
        /*将1个字节的HEX转化成2个字节的ASCII*/
        usTemp	=	charToAsc(cTemp);
        /*将转化得到的2字节ASCII放入文件中*/
        *(WORD16 *)pTemp2	=	usTemp;
        /*指向下两个字节*/
        pTemp2	=	pTemp2 + 2;

        /*如果是一条记录的最后一个字节，则增加回车换行*/
        if ((((i + 1) % 16) == 0) && (i != 0))
        {
            *(WORD16 *)pTemp2	=	0x0d0a;
            pTemp2	=	pTemp2 + 2;
        }
    }
}

void outputMsg(INT nMask, BYTE *pucMsg, WORD32 dwLen)
{
    WORD32	dwFileLenAscii = 0;
    BYTE	*pFileDataAscii;
    dwFileLenAscii	=	dwLen * 2 + (dwLen / 16 + 1) * 2 + 2;
    pFileDataAscii	= (BYTE *)malloc(dwFileLenAscii);
    memset((void *)pFileDataAscii, 0, dwFileLenAscii);

    if (dwLen > 256)
        { dwLen = 256; }

    hexMemToString(pFileDataAscii, pucMsg, dwLen);
    traceMsg(nMask, pFileDataAscii);
    free(pFileDataAscii);
}



INT regCmdFun(T_CMD_PROCFUN *pSrcCmdFun, T_CMD_PROCFUN *pOutCmdFun)
{
    WORD32 i = 0;
    WORD32 j = 0;

    if ((NULL == pSrcCmdFun) || (NULL == pOutCmdFun))
    {
        return SUCCESS;
    }

    for (i = 0; i < MAX_CMD_NUM; i++)
    {
        /*到注册函数结束，退出处理*/
        if (0xff == pSrcCmdFun[i].cmd)
        {
            break;
        }

        if ((0xff != pSrcCmdFun[i].cmd) && (NULL != pSrcCmdFun[i].cmdFun))
        {
            for (j = 0; j < (MAX_CMD_NUM - 1); j++)
            {
                if (pSrcCmdFun[i].cmd == pOutCmdFun[j].cmd)
                {
                    pOutCmdFun[j].cmdFun = pSrcCmdFun[i].cmdFun;
                    break;
                }

                if (0xff == pOutCmdFun[j].cmd)
                {
                    pOutCmdFun[j].cmd = pSrcCmdFun[i].cmd;
                    pOutCmdFun[j].cmdFun = pSrcCmdFun[i].cmdFun;

                    pOutCmdFun[j + 1].cmd = 0xff;
                    pOutCmdFun[j + 1].cmdFun = NULL;

                    break;
                }
            }
        }
    }

    return SUCCESS;
}

#endif

