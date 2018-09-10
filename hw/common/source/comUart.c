#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "common.h"
#include "comUart.h"
#include "debug.h"
#include "comifnet.h"

static INT uartGetSerialFromName(BYTE *uartName, T_XML_UART *uartXmlInfo);

static INT gsspeed_arr[] =
{
	B38400, B19200, B9600, B4800, B2400, B1200, B300, B115200, B19200, B9600,
	B4800, B2400, B1200, B300,
};
static INT gsname_arr[] =
{
	38400, 19200, 9600, 4800, 2400, 1200, 300, 115200,
	19200, 9600, 4800, 2400, 1200, 300,
};

enum
{
	OUTSIDE_FRAME,
	LINK_ESCAPE,
	INSIDE_FRAME
};


#define FLAG_TRANS 0x20
#define CRCPOLY_LE 0xedb88320


//#define LCD_UART    2
#define PWR_UART	1
#define BOX_UART    0

static S_UART_INFO gsUartInfo[MAX_UART_COUNT] =
{
    {-1,"ttyPS0",0,0},	// box
    {-1,"ttyPS1",1,0} 	// power
};

unsigned long jcrc32_le(unsigned long crc, BYTE const *p, long len)
{
	INT i;

	while (len--)
	{
		crc ^= *p++;

		for (i = 0; i < 8; i++)
			{ crc = (crc >> 1) ^((crc & 1) ? CRCPOLY_LE : 0); }
	}

	return crc;
}


unsigned long datapack(void *inBuf, unsigned long len, void *outBuf)
{
	WORD16 *hdr;
	BYTE *in = (BYTE *)inBuf;
	BYTE *out = (BYTE *)outBuf;
	BYTE *pscrc;
	BYTE *ptr = out;
	unsigned long i;
    unsigned long fcs = ~(jcrc32_le(~0, in, len));
	*ptr++ = 0x55;
	*ptr++ = 0xAA;
	hdr = (WORD16 *)ptr;
	ptr += 2;
	*ptr++ = 0x7e;
	*ptr++ = 0x7e;

	for (i = 0; i < len; i++)
	{
		switch (in[i])
		{
			case 0x7d:
				*ptr++ = 0x7d;
				*ptr++ = 0x5d;
				break;
			case 0x7e:
				*ptr++ = 0x7d;
				*ptr++ = 0x5e;
				break;
			default:
				*ptr++ = in[i];
				break;
		}
	}

	pscrc = (BYTE *)&fcs;

	for (i = 0; i < 4; i++, pscrc++)
	{
		switch (*pscrc)
		{
			case 0x7d:
				*ptr++ = 0x7d;
				*ptr++ = 0x5d;
				break;
			case 0x7e:
				*ptr++ = 0x7d;
				*ptr++ = 0x5e;
				break;
			default:
				*ptr++ = *pscrc;
				break;
		}
	}

	*ptr++ = 0x7e;
	*ptr++ = 0x7e;
	*hdr = ptr - out - 4;
	return ptr - out;
}


INT dataunpack(void *inBuf, unsigned long inlen, void *outBuf)
{
	BYTE *in = (BYTE *)inBuf;
	BYTE *out = (BYTE *)outBuf;
	INT state = OUTSIDE_FRAME;
	INT i, len = 0;
	BYTE *data = out, *crcinpkt;
	unsigned long crc32 = 0, *crctmp;
	BYTE byte;
	WORD16 nlen;

	if (*in != 0x55 && *(in + 1) != 0xaa)
	{
		return 0;
	}

	in += 2;
	memcpy(&nlen, in, sizeof(WORD16));
	in += 2;

	for (i = 0; i < nlen; i++)
	{
		byte = in[i];

		switch (state)
		{
			case OUTSIDE_FRAME:

				if (byte == 0x7E)
					{ continue; }

				state = INSIDE_FRAME;
			case INSIDE_FRAME:

				switch (byte)
				{
					case 0x7d: //
						state = LINK_ESCAPE;
						continue;
					case 0x7E:

						if (i != (nlen) - 2)
							{ return 0; }
						else
							{ state = OUTSIDE_FRAME; }

						continue;
				}

				break;
			case LINK_ESCAPE:

				if (byte == 0x5e)
				{
					state = INSIDE_FRAME;
					byte ^= FLAG_TRANS;
					break;
				}

				if (byte == 0x5d)
				{
					state = INSIDE_FRAME;
					byte ^= FLAG_TRANS;
					break;
				}

				goto frame_error;
		}

		data[len++] = byte;
		continue;
frame_error:
		return 0;
	}

	crcinpkt = (data + len - 4);
    crc32 = (~jcrc32_le(~0, data, len - 4));
	crctmp = &crc32;

	if (memcmp(crctmp, crcinpkt, 4) == 0)
		{ return len - 4; }
	else
		{ return -2; }

	//return 0;
}


void uartRevDataProcess(BYTE *buf, INT recvLen, T_UART_BUF *pUartBuf)
{
	INT i;
	pUartBuf->uartleftdatalen = recvLen;

	/* 对buf中的数据逐字节进行接收处理 */
	for (i = 0; i < recvLen; i++)
	{
		/* 判断串口uartcatch中是否已收满了数据 */
		if (pUartBuf->uartrecvcnt >= UART_BUF_SIZE)
		{
			pUartBuf->uartdo = 0;
			pUartBuf->uartrecvcnt = 0;
			pUartBuf->curuartptr = pUartBuf->uartcatch;
			pUartBuf->uartstate = UART_IDLE;
		}

		/* 将数据接收进串口uartcatch中 */
		*(pUartBuf->curuartptr)++ = buf[i];
		(pUartBuf->uartrecvcnt)++;
		(pUartBuf->uartleftdatalen)--;

		switch (pUartBuf->uartstate)
		{
				/* UART_IDLE态：串口uartcatch中无有效数据 */
			case UART_IDLE:
			{
				if (buf[i] == 0X55)
				{
					/* 遇到有效数据的起始标志字节，进入UART_RECV_BEGIN态 */
					(pUartBuf->uartstate) = UART_RECV_BEGIN;
				}
				else
				{
					/* 接收的是无效数据，将其清掉 */
					(pUartBuf->uartrecvcnt) = 0;
					(pUartBuf->curuartptr) = (pUartBuf->uartcatch);
				}

				break;
			}
			/* UART_RECV_BEGIN态：串口uartcatch中有第一个有效字节0x55 */
			case UART_RECV_BEGIN:
			{
				if (buf[i] == 0XAA)
				{
					/* 遇到有效数据的第二个标志字节，进入UART_RECV_LENGTH态 */
					(pUartBuf->uartstate) = UART_RECV_LENGTH;
					(pUartBuf->uartbytecnt) = 0;
				}
				else
				{
					/* 接收到的不是第二个标志字节，前面接收到的数据构不成合法包，将
					   其清掉 */
					(pUartBuf->uartrecvcnt) = 0;
					(pUartBuf->curuartptr) = (pUartBuf->uartcatch);
					(pUartBuf->uartstate) = UART_IDLE;
					/* 字节序号减1使下一轮循环重新接收该字节，检查该字节及其后面
					   的数据是否构成合法包 */
					i--;
					(pUartBuf->uartleftdatalen)++;
				}

				break;
			}
			/* UART_RECV_LENGTH态：串口uartcatch已接收到有效数据的两起始标志字节
			   0x55和0xaa，开始接收两字节包长 */
			case UART_RECV_LENGTH:
			{
				(pUartBuf->uartbytecnt)++;

				if ((pUartBuf->uartbytecnt) == 2)
				{
					/* 两字节包长接收完毕，开始接收包头，进入UART_RECV_HEAD态 */
					(pUartBuf->uartstate) = UART_RECV_HEAD;
					(pUartBuf->uartbytecnt) = 0;
				}

				break;
			}
			/* UART_RECV_HEAD态：串口uartcatch已接收完包长字段，开始接收包头 */
			case UART_RECV_HEAD:
			{
				if (buf[i] == 0X7e)
				{
					(pUartBuf->uartbytecnt)++;

					if ((pUartBuf->uartbytecnt) == 2)
					{
						/* 接收完两字节包头“0x7e 0x7e”，进入UART_RECV_DATA态 */
						(pUartBuf->uartstate) = UART_RECV_DATA;
						(pUartBuf->uartbytecnt) = 0;
					}
				}
				else
				{
					/* 接收到的不是包头，前面接收的数据构不成合法包，将其清掉 */
					(pUartBuf->uartrecvcnt) = 0;
					(pUartBuf->curuartptr) = (pUartBuf->uartcatch);
					(pUartBuf->uartstate) = UART_IDLE;
					/* 字节序号减1使下一轮循环重新接收该字节，检查该字节及其后面
					   的数据是否构成合法包 */
					i--;
					(pUartBuf->uartleftdatalen)++;
				}

				break;
			}
			/* UART_RECV_DATA态：开始接收数据净荷 */
			case UART_RECV_DATA:
			{
				if (buf[i] == 0x7e)
					/* 遇到包尾，进入UART_RECV_END态 */
					{ (pUartBuf->uartstate) = UART_RECV_END; }

				break;
			}
			/* UART_RECV_END态：接收包尾末字节 */
			case UART_RECV_END:
			{
				if (buf[i] == 0x7e)
				{
					/* 遇到包尾末字节，一个完整数据包接收完毕，置相关标志位 */
					(pUartBuf->uartstate) = UART_RECV_BUSY;
					(pUartBuf->uartdo) = 1;
				}
				else
				{
					/* 接收到的不是包尾，前面接收的数据构不成合法包，将其清掉 */
					(pUartBuf->uartrecvcnt) = 0;
					(pUartBuf->curuartptr) = (pUartBuf->uartcatch);
					(pUartBuf->uartstate) = UART_IDLE;
					/* 字节序号减1使下一轮循环重新接收该字节，检查该字节及其后面
					   的数据是否构成合法包 */
					i--;
					(pUartBuf->uartleftdatalen)++;
				}

				break;
			}
			/* UART_RECV_BUSY态：串口uartcatch中有完整数据包待处理，处理过程
			   应该不会走进该状态中来 */
			case UART_RECV_BUSY:
			{
				(pUartBuf->curuartptr)--;
				(pUartBuf->uartrecvcnt)--;
				break;
			}
		}

		/* 如果uartcatch中已收到一个完整的包，则将剩下未处理完的数据保存
		    至串口剩余数据buff中，待uartcatch被清空后再继续接收处理 */
		if (pUartBuf->uartdo == 1)
		{
			memcpy(pUartBuf->uartbuff, buf + i + 1,
			       pUartBuf->uartleftdatalen);
			break;
		}
	}
}


int getPwrUartNo()
{
    return gsUartInfo[PWR_UART].serial;
}

int getBoxUartNo()
{
    return gsUartInfo[BOX_UART].serial;
}

void uartInitInfo()
{
	INT i;

	for (i = 0; i < MAX_UART_COUNT; i++)
	{
		gsUartInfo[i].fd = -1;
		gsUartInfo[i].UartBuf.uartdo = 0;
		gsUartInfo[i].UartBuf.uartrecvcnt = 0;
		gsUartInfo[i].UartBuf.uartbytecnt = 0;
		gsUartInfo[i].UartBuf.uartleftdatalen = 0;
		gsUartInfo[i].UartBuf.uartstate = UART_IDLE;
		gsUartInfo[i].UartBuf.curuartptr = gsUartInfo[i].UartBuf.uartcatch;
	}

	return;
}


INT uartOpen(BYTE *Dev)
{
    INT fd_uart;
    fd_uart = open(Dev, O_RDWR | O_NDELAY | O_NOCTTY);	//| O_NOCTTY | O_NDELAY
    if (-1 == fd_uart)
	{
        printf("Can't Open Serial Port %s\n",Dev);
		return -1;
	}
	else
    {
        return fd_uart;
    }
}

void uartSetSpeed(INT fd, INT speed)
{
	INT i;
	INT status;
	struct termios Opt;
	tcgetattr(fd, &Opt);

	for (i = 0; i < sizeof(gsspeed_arr) / sizeof(INT); i++)
	{
		if (speed == gsname_arr[i])
		{
			tcflush(fd, TCIOFLUSH);
			cfsetispeed(&Opt, gsspeed_arr[i]);
			cfsetospeed(&Opt, gsspeed_arr[i]);
			status = tcsetattr(fd, TCSANOW, &Opt);

			if (status != 0)
			{
				perror("tcsetattr fd1");
				return;
			}

			tcflush(fd, TCIOFLUSH);
		}
	}
}


INT uartSetParity(INT fd, INT databits, INT stopbits, INT parity)
{
	struct termios options;

	if (tcgetattr(fd, &options) != 0)
	{
		perror("SetupSerial 1");
		return (-1);
	}

	options.c_cflag &= ~CSIZE;

	switch (databits)
		/*设置数据位数 */
	{
		case 7:
			options.c_cflag |= CS7;
			break;
		case 8:
			options.c_cflag |= CS8;
			break;
		default:
			fprintf(stderr, "Unsupported data sizen");
			return (-1);
	}

	switch (parity)
	{
		case 'n':
		case 'N':
			options.c_cflag &= ~PARENB;	/* Clear parity enable */
			options.c_iflag &= ~INPCK;	/* Enable parity checking */
			break;
		case 'o':
		case 'O':
			options.c_cflag |= (PARODD | PARENB);	/* 设置为奇效验 */
			options.c_iflag |= INPCK;	/* Disnable parity checking */
			break;
		case 'e':
		case 'E':
			options.c_cflag |= PARENB;	/* Enable parity */
			options.c_cflag &= ~PARODD;	/* 转换为偶效验 */
			options.c_iflag |= INPCK;	/* Disnable parity checking */
			break;
		case 'S':
		case 's':				/*as no parity */
			options.c_cflag &= ~PARENB;
			options.c_cflag &= ~CSTOPB;
			break;
		default:
			fprintf(stderr, "Unsupported parityn");
			return (-1);
	}

	/* 设置停止位 */
	switch (stopbits)
	{
		case 1:
			options.c_cflag &= ~CSTOPB;
			break;
		case 2:
			options.c_cflag |= CSTOPB;
			break;
		default:
			fprintf(stderr, "Unsupported stop bitsn");
			return (-1);
	}

	/* Set input parity option */
	if (parity != 'n')
		{ options.c_iflag |= INPCK; }

	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG | INLCR | ICRNL | IEXTEN);
//   Choosing   Raw   Input
//options.c_oflag   &=~(OCRNL|ONLCR|ONLRET);
	options.c_cflag |= (CLOCAL | CREAD);
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	options.c_oflag &= ~OPOST;
	options.c_oflag &= ~(ONLCR | OCRNL);	//添加的
	options.c_iflag &= ~(ICRNL | INLCR);
	options.c_iflag &= ~(IXON | IXOFF | IXANY);	//添加的
	tcflush(fd, TCIOFLUSH);
	options.c_cflag &= ~CRTSCTS;
	options.c_cc[VTIME] = 0;	/* 设置超时15 seconds */
	options.c_cc[VMIN] = 0;		/* Update the options and do it NOW */

	if (tcsetattr(fd, TCSANOW, &options) != 0)
	{
		perror("SetupSerial 3");
		return (-1);
	}

	return (0);
}


INT uartOpenAndSet(BYTE *devName)
{
	INT uartFd;
	uartFd = uartOpen(devName);

	if (uartFd < 0)
	{
		printf("uartOpenAndSet error, uartFd = %d\t devName: %s\r\n", uartFd, devName);
		return uartFd;
	}

	uartSetSpeed(uartFd, 115200);

	if (uartSetParity(uartFd, 8, 1, 'N') < 0)
	{
		printf("Set Parity Error\r\n");
		return -1;
	}

	return uartFd;
}

INT uartOpenAndSetData(BYTE *devName,int baud,int databit,int stopbit,char ecc)
{
    INT uartFd;
    uartFd = uartOpen(devName);

    if (uartFd < 0)
    {
        printf("uartOpenAndSet error, uartFd = %d\t devName: %s\r\n", uartFd, devName);
        return uartFd;
    }

    uartSetSpeed(uartFd, baud);

    if (uartSetParity(uartFd, databit, stopbit, ecc) < 0)
    {
        printf("Set Parity Error\r\n");
        return -1;
    }

    return uartFd;
}


INT uartInitTty(INT serialNo)
{
	INT serial;
    BYTE Dev[16] = "/dev/";

	/* 寻找与指定tty编号对应的串口 */
	for (serial = 0; serial < MAX_UART_COUNT; serial++)
	{
		if (serialNo == gsUartInfo[serial].serial)
			{ break; }
	}

	/* 未找到对应的串口 */
	if (serial >= MAX_UART_COUNT)
	{
		printf("Can't find serial port for ttyNo: %d\r\n", serialNo);
		return -1;
	}

	/* 找到了对应的串口 */
	strcat(Dev, gsUartInfo[serial].uartName); /* 组串口节点名 */

    gsUartInfo[serial].fd = uartOpenAndSet(Dev); /* 打开并设置串口 */

	if (gsUartInfo[serial].fd < 0)
	{
		printf("Open %s fail!\r\n", Dev);
		return -2;
	}
	else
	{
        //printf("open %s success %d-%d!\r\n",Dev,serial,gsUartInfo[serial].fd);
		return 0;
	}
}



INT uartSendInterface(char *pBuf , INT len , void *pNode)
{
	struct sIntComDevicdInf comDevicdInf;
	INT ret, i;

	if (NULL == pNode)
		return -1;
	
	memcpy(&comDevicdInf, pNode, sizeof(struct sIntComDevicdInf));
	ret = uartWriteData(comDevicdInf.nodeInf.fd, pBuf, len);

	return ret;
}


void __writeData(int fd,void *outBuf,int len)
{
    int remainlen = len;
    int sendlen = 0;
    int sendTotallen = 0;
    while(remainlen)
    {
        sendlen = (remainlen>=16)?16:remainlen;
        int slen = write(fd, &outBuf[sendTotallen], sendlen);
        if(slen<=0)
        {
            printf("\n write data error!!!!!!!!!!!!1\n");
        }
        else
        {
            printf("send data len %d %d\n",sendlen,slen);
        }
        remainlen      -=sendlen;
        sendTotallen   +=sendlen;
        if(remainlen>0)
        {
            //usleep(100);
            printf("remain len is %d\n",remainlen);
        }
    }
}

INT uartWriteData(INT serialNo, void *pBuf, INT writeLen)
{
	INT ret, len, serial, i;
	BYTE outBuf[UART_BUF_SIZE];

	/* 检查要发送的数据长度是否超过发送buff的限制 */
	if (writeLen > 1080) //extboard update need 1024
	{
        printf("The data to be sent is too long\r\n");
		return 2;
	}

	/* 寻找tty编号对应的串口 */
	for (serial = 0; serial < MAX_UART_COUNT; serial++)
	{
		if (serialNo == gsUartInfo[serial].serial)
			{ break; }
	}

	/* 未找到对应的串口 */
	if (serial >= MAX_UART_COUNT)
	{
        printf("Can't find serial port for ttyNo: %d\r\n", serialNo);
		return 3;
	}

	/* 检查待发送数据是否为NULL */
	if (pBuf == NULL)
	{
        printf("The data to be sent is NULL\r\n");
		return 4;
	}
	/* 将待发送数据打包放进发送buff */
	len = datapack(pBuf, writeLen, outBuf);
	if (serialNo == 1)
		dump_data1(outBuf, len);

	/* 判断串口是否可用 */
	if (gsUartInfo[serial].fd >= 0)
    {
        int slen = write(gsUartInfo[serial].fd, outBuf, len);
        if(slen <= 0)
        {
            printf("\n write data error!!!!!!!!!!!!1\n");
        }

		if (slen != len)
		{
			printf("uart write error: slen = %d, len = %d.\n", slen, len);
		}
		
        ret = 0;
	}
	else
	{
        printf("Write error: uart %d fd = %d\r\n", serialNo,	gsUartInfo[serial].fd);
		ret = 1;
	}

	return ret;
}

INT uartWriteDataNoCrc(INT serialno , CHAR *pBuf , INT writeLen)
{
	INT ret, len, serial;

	if (writeLen > 1080) //extboard update need 1024
	{
        traceMsg("The data to be sent is too long\r\n");
		return 2;
	}

	/* 寻找tty编号对应的串口 */
	for (serial = 0; serial < MAX_UART_COUNT; serial++)
	{
		if (serialno == gsUartInfo[serial].serial)
		{
			break;
		}
	}

	/* 未找到对应的串口 */
	if (serial >= MAX_UART_COUNT)
	{
        traceMsg("Can't find serial port for ttyNo: %d\r\n", serialno);
		return 3;
	}

	/* 检查待发送数据是否为NULL */
	if (pBuf == NULL)
	{
        traceMsg("The data to be sent is NULL\r\n");
		return 4;
	}

	if (gsUartInfo[serial].fd >= 0)
	{
		len = write(gsUartInfo[serial].fd, pBuf, writeLen);
		ret = 0;
	}
	else
	{
        traceMsg("Write error: uart %d fd = %d\r\n", serialno, gsUartInfo[serial].fd);
		ret = 1;
	}

	return ret;
}


INT uartReadData(INT serialNo, struct timeval timeout, BYTE *pOutBuf)
{
	INT ret, maxFd, serial;
	fd_set readFds;
	BYTE buf[MAX_BUFF_SIZE];

	/* 寻找tty编号对应的串口 */
	for (serial = 0; serial < MAX_UART_COUNT; serial++)
	{
		if (serialNo == gsUartInfo[serial].serial)
			{ break; }
	}

	/* 未找到对应的串口 */
	if (serial >= MAX_UART_COUNT)
	{
		return -4;
	}

	/* 判断输出buff是否为NULL */
	if (pOutBuf == NULL)
	{
		return -3;
	}

    //printf("uart fd is %d\n",gsUartInfo[serial].fd);
	maxFd = gsUartInfo[serial].fd;
	FD_ZERO(&readFds);
	FD_SET(gsUartInfo[serial].fd, &readFds);

	/* 处理上次未收完的数据 */
	if (gsUartInfo[serial].UartBuf.uartleftdatalen > 0)
	{
		printf("== last data: len = %d.\n", gsUartInfo[serial].UartBuf.uartleftdatalen);
		uartRevDataProcess(gsUartInfo[serial].UartBuf.uartbuff,
		                   gsUartInfo[serial].UartBuf.uartleftdatalen,
		                   &(gsUartInfo[serial].UartBuf));
	}

    ret = select(maxFd + 1, &readFds, NULL, NULL, &timeout);

	if (ret > 0)
	{
		if (gsUartInfo[serial].fd >= 0)
		{
			if (FD_ISSET(gsUartInfo[serial].fd, &readFds))
			{
				ret = read(gsUartInfo[serial].fd, buf, MAX_BUFF_SIZE);

				if (ret > 0)
				{
					//printf("uart recv len = %d.\n", ret);
					uartRevDataProcess(buf, ret, &(gsUartInfo[serial].UartBuf));
				}
				else
				{
					printf("==== PWR UART READ ERROR: %d. ======\n", ret);
				}
			}
		}
	}

	/* 如果提取到一个完整的包则进行解包操作 */
	if (gsUartInfo[serial].UartBuf.uartdo == 1)
	{        
		ret = dataunpack(gsUartInfo[serial].UartBuf.uartcatch,
		                 gsUartInfo[serial].UartBuf.uartrecvcnt,
		                 pOutBuf);       
        //printf("get the complete package %d\n",ret);
		/* 解包完毕，清理串口缓冲状态和已被处理完的数据包 */
		gsUartInfo[serial].UartBuf.uartdo = 0;
		gsUartInfo[serial].UartBuf.uartrecvcnt = 0;
		gsUartInfo[serial].UartBuf.uartbytecnt = 0;
		gsUartInfo[serial].UartBuf.uartstate = UART_IDLE;
		gsUartInfo[serial].UartBuf.curuartptr =
		      gsUartInfo[serial].UartBuf.uartcatch;
	}
	else
	{
		//printf(" data error: \n");
		ret = -1;
	}
	
	return ret;
}


INT uartClearData(INT serialNo)
{
	INT serial, ret, maxFd;
	fd_set readFds;
	struct timeval timeout;

	/* 寻找tty编号对应的串口 */
	for (serial = 0; serial < MAX_UART_COUNT; serial++)
	{
		if (serialNo == gsUartInfo[serial].serial)
			{ break; }
	}

	/* 未找到对应的串口 */
	if (serial >= MAX_UART_COUNT)
	{
		return -1;
	}

	/* 找到了对应的串口，判断串口是否打开 */
	if (gsUartInfo[serial].fd < 0)
	{
		return -2;
	}

	timeout.tv_sec = 0;
	timeout.tv_usec = 1000;
	FD_ZERO(&readFds);
	FD_SET(gsUartInfo[serial].fd, &readFds);
	maxFd = gsUartInfo[serial].fd;
	ret = select(maxFd + 1, &readFds, NULL, NULL, &timeout);

	if (ret <= 0)
	{
		return 0;
	}

	/* 清理串口读缓存中的数据 */
	ret = read(gsUartInfo[serial].fd, gsUartInfo[serial].UartBuf.uartcatch, UART_BUF_SIZE);
	/* 清理串口缓冲状态 */
	gsUartInfo[serial].UartBuf.uartdo = 0;
	gsUartInfo[serial].UartBuf.uartrecvcnt = 0;
	gsUartInfo[serial].UartBuf.uartbytecnt = 0;
	gsUartInfo[serial].UartBuf.uartstate = UART_IDLE;
	gsUartInfo[serial].UartBuf.curuartptr =
	      gsUartInfo[serial].UartBuf.uartcatch;
	return 0;
}


void uartInit(void)
{
	INT i;
	uartInitInfo(); //初始化串口全局变量gsUartInfo

	// 根据配置文件打开串口
	for (i = 0; i < MAX_UART_COUNT; i++)
	{
        if (gsUartInfo[i].serial >= 0)
		{
            //printf("UartOpenBySerialNo serial index %d %d.\r\n", i, gsUartInfo[i].serial);
            uartInitTty(gsUartInfo[i].serial);
		}
	}

	return;
}


INT uartSendMsgToBoard(INT serialNo, void *pBuf, WORD16 nLen)
{
	INT ret = SUCCESS;

	if (serialNo >= 0)
	{
        //uartClearData(serialNo);
        usleep(20 * 1000);
		ret = uartWriteData(serialNo, pBuf, nLen);
		//printf("uartSendMsgToBoard: send ret = %d.\n", ret);
	}
	else
	{
		printf("uartSendMsgToBoard error: serialNo = %d.\n", serialNo);
		ret = FAILED;
	}

	return ret;
}

INT uartWaitRecvData(INT nSerialNo, BYTE ucCmd, struct timeval delaytime, BYTE *aucreadbuf, INT *nBufLen)
{
	long long tmp = 0;
	struct timeval start, stop;	//delay for 5sec until get the ringht packet
	INT ret = -1;
	SMsgHead result;
	INT readlen = 0;
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 100 * 1000;
	gettimeofday(&start, 0);

	do
	{
		readlen = uartReadData(nSerialNo, timeout, aucreadbuf);

		if (readlen > 0)
		{
			result = *(SMsgHead *)aucreadbuf;
			
			/* 扩展盒应答的命令不一定跟下发给扩展盒的命令一样，ARM暂且先做兼容，不判断是否相等 by 游维平 2014-2-25 */
			if(1)
			//if (ucCmd == result.cmd)
			{
				if ((result.type != 0x85) || (result.cmd != 0x04))
				{
                 }
				
                *nBufLen = readlen;
				ret = 0;
				break;
			}
		}
		else if (readlen < 0)
		{
			//printf("uartWaitRecvData: error! %d.\n", readlen);
		}

		usleep(10 * 1000);
		gettimeofday(&stop, 0);
		tmp = (start.tv_sec * 1000000 + start.tv_usec + delaytime.tv_sec * 1000000 + delaytime.tv_usec) - (stop.tv_sec * 1000000 + stop.tv_usec);
		
	}while (tmp > 0);

	ret = (0 == ret) ? 0 : 1 ;
	return ret;
}


INT uartProcNormalCmd(INT nSerialNo, struct timeval delaytime, BYTE cmd, void *pBuf, WORD16 nLen, void *pNode)
{
	INT ret = SUCCESS;
	BYTE aucreadbuf[1024] = {0};
	int nBufLen = 0;

	if (0 == uartSendMsgToBoard(nSerialNo, pBuf, nLen))
	{
        traceMsg("uart send cmd: 0x%x %x, UartNo:%d\r\n", (BYTE)*((BYTE*)pBuf+1), cmd, nSerialNo);
		ret = uartWaitRecvData(nSerialNo, cmd, delaytime, aucreadbuf, &nBufLen);        
	}

	if (NULL != pNode && 0 == ret)  // pNode == NULL 表示本地消息，不需要回复，只需要串口返回值
	{
		ifnetDataSend(aucreadbuf, nBufLen, pNode);
	}

	return ret;
}


INT uartProcNormalCmdNoWait(INT nSerialNo,  BYTE cmd, void *pBuf, WORD16 nLen)
{
    traceMsg("begin the serial nSerialNo : %d\r\n", nSerialNo);

	if (0 == uartSendMsgToBoard(nSerialNo, pBuf, nLen))
	{
        traceMsg("enter uart send cmd no wait = %d\r\n", cmd);
        return SUCCESS;
	}
	else
	{
		return FAILED;
	}
}

int uartGetSerialfd(int nsserialNo)
{
	int i = 0;

	for (i = 0; i < MAX_UART_COUNT; i++)
	{
		if (nsserialNo == gsUartInfo[i].serial)
		{
			return gsUartInfo[i].fd;
		}
	}

	return -1;
}

int FindSeiralInUartInfo(int nSerialNo)
{
	int i;
	int nIndex = -1;

	for (i = 0; i < MAX_UART_COUNT; i++)
	{
		if (nSerialNo == gsUartInfo[i].serial)
		{
			nIndex = i;
		}
	}

	return nIndex;
}

int openUart(int nIndex)
{
    BYTE aucDev[16] = "/dev/";

	if (nIndex >= MAX_UART_COUNT || nIndex < 0)
	{
		printf("open Uart can not open nIndex, nIndex = %d.\r\n", nIndex);
		return FAILED;
	}

	strcat(aucDev, gsUartInfo[nIndex].uartName); /* 组串口节点名 */
	printf("Dev:%s\r\n", aucDev);
	printf("nIndex : %d\r\n", nIndex);
	gsUartInfo[nIndex].UartBuf.uartdo = 0;
	gsUartInfo[nIndex].UartBuf.uartrecvcnt = 0;
	gsUartInfo[nIndex].UartBuf.uartbytecnt = 0;
	gsUartInfo[nIndex].UartBuf.uartleftdatalen = 0;
	gsUartInfo[nIndex].UartBuf.uartstate = UART_IDLE;
	gsUartInfo[nIndex].UartBuf.curuartptr = gsUartInfo[nIndex].UartBuf.uartcatch;
	gsUartInfo[nIndex].fd = uartOpenAndSet(aucDev);
	uartClearData(gsUartInfo[nIndex].serial);
	return SUCCESS;
}


void closeUart(int nIndex)
{

	if (nIndex > MAX_UART_COUNT || nIndex < 0)
	{
		printf("close Uart can not close nIndex, nIndex = %d.\r\n", nIndex);
		return ;
	}

	uartClearData(gsUartInfo[nIndex].serial);
	close(gsUartInfo[nIndex].fd);
	//memset(&gsUartInfo[nIndex],0,sizeof(S_UART_INFO));
	memset(&gsUartInfo[nIndex].UartBuf,0,sizeof(T_UART_BUF));
	gsUartInfo[nIndex].fd = -1;
}
