#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <semaphore.h>
#include "pgos/MsOS.h"
#include "common.h"
#include "comStruct.h"
#include "pubpwr.h"
#include "comUart.h"

#include"pubLcdTask.h"
#include "ca310.h"

static pthread_mutex_t gLcdReadLock;
static MS_APARTMENT *lcdApartMent = 0;

typedef struct {
	int fd;
	char name[64]; //设备名
	int channel; //设备对应的通道号
}lcd_device_t;

typedef struct  smsghead_lcd{
	BYTE cmd;
	BYTE type;
	WORD16 len;
	int channel;
} SMsgHeadLcd;

#define LCD_MODE_FLICKER  0
#define LCD_MODE_XYLV     1

#define MAX_LCD_DEVICE_NUMBER   4
static lcd_device_t m_lcd_list[MAX_LCD_DEVICE_NUMBER];
unsigned int m_lcd_list_version = 0;

int lcd_deviceRemove(char* devname) //关闭一个打开的USB串口
{
	char* name = (char*)malloc(256);
	if(name != NULL)
	{
		strcpy(name, devname);
	}
	return MsOS_PostMessage(lcdApartMent->MessageQueue, MSG_LCD_CMD_TTYUSB_REMOVE, (MS_U32)name, 0);
}

int lcd_deviceAdd(char* devname) //当USB口插入一个USB设备时, 调节这个添加串口
{
	char* name = (char*)malloc(256);
	if(name != NULL)
	{
		strcpy(name, devname);
	}
	return MsOS_PostMessage(lcdApartMent->MessageQueue, MSG_LCD_CMD_TTYUSB_ADD, (MS_U32)name, 0);
}

int lcd_getStatus(int channel)
{
	int i;
	for(i=0; i < MAX_LCD_DEVICE_NUMBER; i++)
	{
		lcd_device_t* lcd = &m_lcd_list[i];
		if(lcd->channel > 0 && lcd->channel == channel)
		{
			return 1;
		}
	}
	return 0;
}

int lcd_getList(int* channel_list) //得到可以使用的列表个数, 列表
{
	int count = 0;
	int i;
	for(i=0; i < MAX_LCD_DEVICE_NUMBER; i++)
	{
		lcd_device_t* lcd = &m_lcd_list[i];
		if(lcd->channel > 0)
		{
			if(channel_list != NULL)
			{
				channel_list[count] = lcd->channel;
			}
			count += 1;
		}
	}
	return count;
}

int lcd_getListVersion() //得到设备列表更新之后版本
{
	return m_lcd_list_version;
}

static INT lcdCmdTransfer(int channel, BYTE cmd, void *pBuf, WORD16 nLen,int *pRetrunValue)
{
    int ret = -1;
    return ret;
}

static MS_U32 lcd_message_proc(MS_APARTMENT *pPartMent,MSOS_MESSAGE Message)
{
    int ret=-1;
 //   unsigned char  cmd  = Message.MessageID;
    unsigned char *pBuf = Message.Parameter1;
    unsigned int  nLen  = Message.Parameter2;
    unsigned int  returnValue = -1;
    switch(Message.MessageID)
    {
    	case MSG_LCD_CMD_FLICK_START:
			{
				SMsgHeadLcd* afmsg = (SMsgHeadLcd*)pBuf;
				int i;
				for(i=0; i < MAX_LCD_DEVICE_NUMBER; i++)
				{
					lcd_device_t* lcd = &m_lcd_list[i];
					if(lcd->channel > 0 && lcd->channel == afmsg->channel)
					{
						ca310_start_flick_mode(lcd->channel);
						break;
					}
				}
			}
			break;
			
        case MSG_LCD_CMD_SET_FLICK:
        	{
        		SMsgHeadLcd* afmsg = (SMsgHeadLcd*)pBuf;
				int i;
        		for(i=0; i < MAX_LCD_DEVICE_NUMBER; i++)
				{
					lcd_device_t* lcd = &m_lcd_list[i];
					if(lcd->channel > 0 && lcd->channel == afmsg->channel)
					{
						float f_flick = 0.00;
						ca310_capture_flick_data(lcd->channel, &f_flick);
						break;
					}
    			}
        	}
			break;
			
        case MSG_LCD_CMD_FLICK_END:
        	{
        		SMsgHeadLcd* afmsg = (SMsgHeadLcd*)pBuf;
				int i;
				for(i=0; i < MAX_LCD_DEVICE_NUMBER; i++)
				{
					lcd_device_t* lcd = &m_lcd_list[i];
					if(lcd->channel > 0 && lcd->channel == afmsg->channel)
					{
						ca310_stop_flick_mode(lcd->channel);
						break;
					}
				}
        	}
			break;

		case MSG_LCD_CMD_XYLV_START:
			{
				SMsgHeadLcd* afmsg = (SMsgHeadLcd*)pBuf;
				int i;
				for(i=0; i < MAX_LCD_DEVICE_NUMBER; i++)
				{
					lcd_device_t* lcd = &m_lcd_list[i];
					if(lcd->channel > 0 && lcd->channel == afmsg->channel)
					{
						ca310_start_xylv_mode(lcd->channel);
						break;
					}
				}
			}
			break;

		case MSG_LCD_CMD_XYLV_READ:
        	{
        		SMsgHeadLcd* afmsg = (SMsgHeadLcd*)pBuf;
				int i;
        		for(i=0; i < MAX_LCD_DEVICE_NUMBER; i++)
				{
					lcd_device_t* lcd = &m_lcd_list[i];
					if(lcd->channel > 0 && lcd->channel == afmsg->channel)
					{
						float f_x = 0.00;
						float f_y = 0.00;
						float f_lv = 0.00;
						ca310_capture_xylv_data(lcd->channel, &f_x, &f_y, &f_lv);
						printf("x: %f, y: %f, lv: %f\n");
						break;
					}
    			}
        	}
			break;
			
        case MSG_LCD_CMD_XYLV_STOP:
        	{
        		SMsgHeadLcd* afmsg = (SMsgHeadLcd*)pBuf;
				int i;
				for(i=0; i < MAX_LCD_DEVICE_NUMBER; i++)
				{
					lcd_device_t* lcd = &m_lcd_list[i];
					if(lcd->channel > 0 && lcd->channel == afmsg->channel)
					{
						ca310_stop_xylv_mode(lcd->channel);
						break;
					}
				}
        	}
			break;

		case MSG_LCD_CMD_TTYUSB_ADD: //打开一个USB串口设备节点
		if(Message.Parameter1 > 0)
		{
			char* devname = (char*)Message.Parameter1;
			int i;
			
			for(i=0; i < MAX_LCD_DEVICE_NUMBER; i++)
			{
				lcd_device_t* lcd = &m_lcd_list[i];
				if(lcd->channel == 0) //没有使用
				{
					int channel = 0;
					//分配一个新的通道号
					int tmp_channel = 1;
					while(tmp_channel <= 4)
					{
						int tmp_find = 0;
						int j=0;
						for(j=0; j < MAX_LCD_DEVICE_NUMBER; j++)
						{
							if(m_lcd_list[j].channel > 0 && m_lcd_list[j].channel == tmp_channel)
							{
								tmp_find = 1; //找到已经存在通道号
								break;
							}
						}
						if(tmp_find == 0) //没有找到, 说明这个通道号可以使用
						{
							channel = tmp_channel;
							break;
						}
						tmp_channel += 1;
					}
					//找到一个通道号
					if(channel > 0)
					{
						//打开串口设备, 打开成功, 才更新信息
						//usleep(1000*/*1000*/200); //刚探测到设备插入, 还需要延时一会, 否则容易打开失败
						usleep(100 * 1000);
						ca310_reset(channel);

						char dev_path_name[128] = "";
						sprintf(dev_path_name, "/dev/%s", devname);

						ca310_test(dev_path_name);
						
						if (0 == ca310_open(channel, dev_path_name))
						{
							//sleep(1);
							if (ca310_stop_flick_mode(channel)!= PROBE_STATU_OK)
							{
								//ca310_close(channel);
								//sleep(1);
								//ca310_open(channel, dev_path_name);
								//ca310_stop_flick_mode(channel);
							}

							//lcd->fd = fd;
							strcpy(lcd->name, devname);
							lcd->channel = channel;
							printf(">>>>>>>>>>>>>>>>>>>>>>>>CA310: %d = %s\n", lcd->channel, lcd->name);

							//更新版本
							unsigned int ver = m_lcd_list_version;
							if(ver == 0xffffffff)
							{
								ver = 1;
							}
							else
							{
								ver += 1;
							}
							m_lcd_list_version = ver;
						}
					}
					break;
				}
			}
			free(devname);
		}
		break;

		case MSG_LCD_CMD_TTYUSB_REMOVE: //关闭一个USB串口设备节点
		if(Message.Parameter1 > 0)
		{
			char* devname = (char*)Message.Parameter1;
			int i;
			for(i=0; i < MAX_LCD_DEVICE_NUMBER; i++)
			{
				lcd_device_t* lcd = &m_lcd_list[i];
				if(lcd->channel > 0 && strlen(devname) == strlen(lcd->name) && strcmp(devname, lcd->name) == 0)
				{
					ca310_close(lcd->channel);
					memset(lcd, 0, sizeof(lcd_device_t));
					//更新版本
					unsigned int ver = m_lcd_list_version;
					if(ver == 0xffffffff)
					{
						ver = 1;
					}
					else
					{
						ver += 1;
					}
					m_lcd_list_version = ver;
					break;
				}
			}
			free(devname);
		}
		break;

        default:
        break;
    }
    return ret;
}

static int initLcd_SearchUSB(void)
{
	DIR *dir;
	struct dirent *ptr;
	int usbListNumber = 0;
	char usbList[8][100];
	char base[20] = "ttyUSB"; //ttyUSB0, ttyUSB1, ttyUSB2, ttyUSB3...
	int base_len = strlen(base);
	
	if ((dir=opendir("/dev")) == NULL)
	{
		perror("Open dir error...");
		return -1;
	}	
	
	memset(usbList, 0, sizeof(usbList));
	
	while ((ptr=readdir(dir)) != NULL)
	{
		if(ptr->d_type == 2)
		{
			if(memcmp(ptr->d_name, base, base_len) == 0)
			{
				strcpy(usbList[usbListNumber], ptr->d_name);
				usbListNumber += 1;
			}
		}
	}

	int i,j;
	char txt[100];
	for(i=0; i < 8; i++)
	{
		sprintf(txt, "ttyUSB%d", i);
		for(j=0; j < usbListNumber; j++)
		{
			if(strcmp(txt, usbList[j]) == 0)
			{
				lcd_deviceAdd(txt);
				break;
			}
		}
	}
	
	closedir(dir);
	return 0;
}

int initLcd(void)
{
    MS_EVENT_HANDLER_LIST *pEventHandlerList = 0;

	memset(m_lcd_list, 0, sizeof(m_lcd_list));
    pthread_mutex_init(&gLcdReadLock, NULL);	

    lcdApartMent = MsOS_CreateApartment("lcdTask", lcd_message_proc, pEventHandlerList);
	
	initLcd_SearchUSB();
    return SUCCESS;
}

unsigned int lcd_message_queue_get()
{
    return lcdApartMent->MessageQueue;
}
