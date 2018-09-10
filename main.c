#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/un.h>  
#include <sys/ioctl.h>  
#include <sys/socket.h>  
#include <linux/netlink.h>  
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

#include "json/cJSON.h"
#include "util/util.h"
#include "rwini.h"
#include "packsocket/packsocket.h"
#include "client.h"
#include "util/debug.h"
#include "comStruct.h"
#include "fpgaFunc.h"
#include "common.h"
#include "comStruct.h"
#include "hw/box/box.h"
#include "hw/gpio/hi_unf_gpio.h"

#include "hw/i2c/hi_unf_i2c.h"
#include "hw/spi/hi_unf_spi.h"
#include "mipiflick.h"
#include "spi_sp6.h"
#include "pubPwrTask.h"
#include "mcu_spi.h"

extern int m_channel_1; 
extern int m_channel_2;
extern int m_channel_3;
extern int m_channel_4;

#define _INTSIZEOF(n) ( (sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1) )
#define va_start(ap,v) ( ap = (/*va_list*/char *)&v + _INTSIZEOF(v) )
#define va_arg(ap,t) ( *(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)) )
#define va_end(ap) ( ap = (/*va_list*/char *)0 )

static void Generic_Write(int Nnumber, ...)
{
	char * ap;
	va_start(ap, Nnumber);
	unsigned char i;
	unsigned char cmd[256];

	for(i=0; i < Nnumber; ++i)
	{
		cmd[i] = va_arg(ap, int);
	}
	va_end(ap);

	printf("len = %d     ", Nnumber);
	for(i=0; i < Nnumber; i++)
		printf("%02X ", cmd[i]);

	printf("\n");
}

static crossCursorInfo_t  gstCrossCursorInfo;

static char *skipWord(char *p)
{
   while((*p)==' ')
   {
       p++;
   }
   return p;
}

static int client_usr_cmd(char * usr_cmd,char arg[][32])
{
    int i=0;
    char *p = usr_cmd;
    char *pFind;
    while(p)
    {
        pFind = p;
        pFind = strstr(pFind," ");
        if(!pFind)
        {
            strcpy(arg[i],p);
            break;
        }
        else
        {
            *pFind = '\0';
            strcpy(arg[i],p);
            *pFind = ' ';
            p= pFind;
        }
        p = skipWord(p);
        i++;
    }

    if(!strcasecmp(arg[0], "quit"))
        return 1;
    if(!strcasecmp(arg[0],"login"))
        return 2;
    if(!strcasecmp(arg[0],"mcur "))	// mcu read
        return 3;
    if(!strcasecmp(arg[0],"mcuw"))	// mcu write
        return 4;
	
    if(!strcasecmp(arg[0], "sp6_read"))
        return 5;

    if(!strcasecmp(arg[0], "sp6_write"))
        return 6;

    if(!strcasecmp(arg[0],"mipi"))
        return 7;
    if(!strcasecmp(arg[0],"clock"))
        return 8;
    if(!strcasecmp(arg[0],"pwr"))
        return 9;
    if(!strcasecmp(arg[0],"ipwr"))
        return 10;
    if(!strcasecmp(arg[0],"cusor"))
        return 11;
    if(!strcasecmp(arg[0],"move"))
        return 12;
    if(!strcasecmp(arg[0],"reset"))
        return 13;
    if(!strcasecmp(arg[0],"flick"))
        return 14;
    if(!strcasecmp(arg[0],"autof"))
        return 15;
    if(!strcasecmp(arg[0],"i2cr"))
        return 16;
    if(!strcasecmp(arg[0],"i2cw"))
        return 17;
    if(!strcasecmp(arg[0],"reUsb"))
        return 18;
    if(!strcasecmp(arg[0],"tstFun"))
        return 19;
    if(!strcasecmp(arg[0],"rgb"))
        return 20;
    if(!strcasecmp(arg[0],"uart"))
        return 21;
    if(!strcasecmp(arg[0],"vcom"))
        return 22;
    if(!strcasecmp(arg[0],"ca310"))
        return 23;
	if(!strcasecmp(arg[0],"read"))
        return 24;

	if(!strcasecmp(arg[0], "fpga_read"))
        return 25;
	if(!strcasecmp(arg[0], "fpga_write"))
        return 26;
	
	if(!strcasecmp(arg[0], "gpio_set"))
        return 27;

	if(!strcasecmp(arg[0], "id"))
        return 28;
	
	if(!strcasecmp(arg[0], "vtest"))
        return 29;

	if(!strcasecmp(arg[0], "write"))
        return 30;

	if(!strcasecmp(arg[0], "channel"))
		return 31;
	
	if(!strcasecmp(arg[0], "mtpon"))
		return 32;
	
	if(!strcasecmp(arg[0], "mtpoff"))
		return 33;

	if(!strcasecmp(arg[0], "mread"))
		return 34;

	if (!strcasecmp(arg[0], "vspi_read"))
		return 35;

	if (!strcasecmp(arg[0], "vspi_write"))
		return 36;

	if (!strcasecmp(arg[0], "gpio_read"))
		return 37;

	if (!strcasecmp(arg[0], "i2c_write"))
		return 38;

	if (!strcasecmp(arg[0], "i2c_read"))
		return 39;

	if (!strcasecmp(arg[0], "vi2c_write"))
		return 40;

	if (!strcasecmp(arg[0], "vi2c_read"))
		return 41;

	if (!strcasecmp(arg[0], "read_vcom"))
		return 42;

	if (!strcasecmp(arg[0], "write_vcom"))
		return 43;

	if (!strcasecmp(arg[0], "burn_vcom"))
		return 44;

	if (!strcasecmp(arg[0], "read_vcom_opt_times"))
		return 45;

    return -1;
}


void show_help()
{
    printf("\[help]--print this command list\n");
    printf("\[login]--login to server\n");
    printf("\get [remote file]--get [remote file] to local host as\n");
    printf("\tif  isn't given, it will be the same with [remote_file] \n");
    printf("\tif there is any \' \' in , write like this \'\\ \'\n");
    printf("\[quit]--quit this ftp client program\n");
}

static int client_cmd()
{
    int  cmd_flag;
    char usr_cmd[256];
    char arg[6][32];

    while(1)
    {
        printf("cmd_svn_z:>");
        fgets(usr_cmd, 256, stdin);
        fflush(stdin);
        if(usr_cmd[0] == '\n')
            continue;
        usr_cmd[strlen(usr_cmd)-1] = '\0';
        cmd_flag = client_usr_cmd(usr_cmd,arg);
        switch(cmd_flag)
        {
            case 1:
            {	
				printf("exit!\n");
                exit(0);
                break;
            }
			
            case 2:
            {
				printf("regeister!\n");
                client_register("123456","789123","12345678");
            }
            break;

            case 3:
            {
            	unsigned char cmd1  = strtol(arg[1],0,0);
                unsigned char cmd2 = strtol(arg[2],0,0);
				printf("mcu read: 0x%02x, 0x%02x:\n", cmd1, cmd2);
				unsigned short value = mcu_read_data_16(cmd1, cmd2);
				printf("read value = %d\r\n", value);
            }
            break;

            case 4:
            {
            	unsigned char cmd1  = strtol(arg[1],0,0);
                unsigned char cmd2 = strtol(arg[2],0,0);
                unsigned short value = strtol(arg[3],0,0);
				printf("mcu write: 0x%02x, 0x%02x, 0x%x\n", cmd1, cmd2, value);
				mcu_write_data_16(cmd1, cmd2, value);
            }
            break;

			// sp6 read
            case 5:
            {
				printf("sp6 read reg\n");
                unsigned char reg = strtol(arg[1],0,0);
                unsigned char value = sp6_read_8bit_data(reg);
				printf("sp6 read: reg: 0x%02x, value: 0x%02x.\n", reg, value);
            }
			break;

			// sp6 write
            case 6:
            {
                unsigned char reg  = strtol(arg[1],0,0);
                unsigned char data = strtol(arg[2],0,0);
				printf("sp6 write: reg: 0x%02x, data: 0x%02x.\n", reg, data);
				sp6_write_8bit_data(reg, data);
            }
			break;
			
            case 7:
            {
				printf("7: test mipi!\n");
                //test_mipi();
            }
			break;
			
            case 8:
            {
				printf("8: clock freq!\n");
                clock_5338_setFreq(strtol(arg[1],0,0));
            }
			break;

            case 11:
            {
				printf("11: cross cursor!\n");
                crossCursorInfo_t info;
                info.enable = 1;
                info.x = strtol(arg[1],0,0);
                info.y = strtol(arg[2],0,0);
                info.wordColor = 0xFFFFFF;
                info.crossCursorColor = 0;
                info.RGBchang = 0;
                info.HVflag = 0;
                info.startCoordinate = 0;
                fpgaCrossCursorSet(&info);
                memcpy(&gstCrossCursorInfo,&info,sizeof(crossCursorInfo_t));
            }
            break;

            case 12:
            {
				printf("12: hide cursor!\n");
                crossCursorInfo_t info = {0};
                memcpy(&info,&gstCrossCursorInfo,sizeof(crossCursorInfo_t));
                int moveup = strtol(arg[1],0,0);
                if(moveup)
                	info.x += 10;
                else
                	info.x -= 10;

                fpgaCrossCursorSet(&info);
                memcpy(&gstCrossCursorInfo,&info,sizeof(crossCursorInfo_t));
            }
            break;

            case 13:
            {
				printf("13: fpga!\n");
				fpgaReset();
            }
            break;

            case 14:
            {
				printf("14: flick manual!\n");
                int vaule = strtol(arg[1],0,0);
                flick_manual(vaule);
            }
            break;

            case 15:
            {
				printf("15: auto flick!\n");
                flick_auto(1,0);
            }
            break;

            case 16:
            {
				printf("16: i2c_read tst!\n");
                int i2cdevAddr = strtol(arg[1],0,0);
                int i2cRegAddr = strtol(arg[2],0,0);
                int count = strtol(arg[3],0,0);

				printf("dev addr: %d, %#x, reg: %d, %#x.\n", i2cdevAddr, i2cdevAddr, i2cRegAddr, i2cRegAddr);
                //i2c_read_tst(i2cdevAddr,i2cRegAddr,count);
            }
            break;

            case 17:
            {
				printf("17: i2c_write tst!\n");
                int i2cdevAddr = strtol(arg[1],0,0);
                int i2cRegAddr = strtol(arg[2],0,0);
                int i2cRegVal = strtol(arg[3],0,0);
                //i2c_write_tst(i2cdevAddr,i2cRegAddr,i2cRegVal);
            }
            break;

            case 18:
            {
				printf("18: gpio!\n");
                int gpioCount = 0;
                while(gpioCount<10000)
                {
                    gpio_set_output_value(47, 0);
                    usleep(100);
                    gpio_set_output_value(47, 1);
                    usleep(100);
                }
            }
            break;

            case 19:
            {
				printf("19: module!\n");
                dbInitModuleSelf();
                dbModuleSelfGetByName("8WX");
            }
            break;

            case 20:
            {
				printf("20: rgb!\n");
				showRgbInfo_t   showRgb;
                showRgb.rgb_r = strtol(arg[1],0,0);
                showRgb.rgb_g = strtol(arg[2],0,0);
                showRgb.rgb_b = strtol(arg[3],0,0);
                fpgaShowRgb(&showRgb);
                printf("show rgb %x %x %x\n",showRgb.rgb_r,showRgb.rgb_g,showRgb.rgb_b);
            }
            break;

            case 21:
            {
				printf("21: uart fd!\n");
                //unsigned char outBuf[] = {0x55,0xaa,0x0c,0x00,0x7e,0x7e,0x13,0x0c,0x36,0x00,0x7c,0x4f,0xee,0xe3,0x7e,0x7e};
                unsigned char outBuf[] = {0x55,0xaa,0x0d,0x00,0x7e,0x7e,0x13,0x0c,0x37,0x00,0x3d,0x7d,0x5e,0xf5,0xfa,0x7e,0x7e};
                int  len = sizeof(outBuf);
                int  fd = uartGetSerialfd(2);
                if (fd >= 0)
                {
                    int rlen = write(fd, outBuf, len);
                    printf("len is %d %d %d\n",len,rlen,fd);
                }
            }
            break;

            case 22:
            {
				printf("22: vcom!\n");

				int vcom_index = strtol(arg[1],0,0);
				flick_manual(vcom_index);
            }
            break;

            case 23:
            {
				printf("23: ca310!\n");
            }
            break;

			case 24:
            {
				printf("read:\n");
                int reg = strtol(arg[1],0,0);
				printf("spi read reg: 0x%02x.\n", reg);
				SSD2828_read_reg(1, reg);
            }
            break;

			case 25:
            {
				printf("fpga read:\n");
                int reg = strtol(arg[1], 0, 0);	
				int val = fpga_read(reg);
				printf("FPGA Read reg: 0x%02x, val: %#x.\n", reg, val);
				
            }
            break;

			case 26:
            {
				printf("fpga write:\n");
                int reg = strtol(arg[1], 0, 0);	
				int val = strtol(arg[2], 0, 0);	
				fpga_write(reg, val);
				printf("FPGA Write reg: 0x%02x, val: %#x.\n", reg, val);
				
            }
            break;
			
			case 27:
			{
				printf("gpio_set:\n");
				int pin = strtol(arg[1], 0, 0); 
				int value = strtol(arg[2], 0, 0); 

				gpio_set_output_value(pin, value);
				printf("gpio_set pin: %d, value: %d.\n", pin, value);
				
			}
			break;

			case 28:
            {
				printf("do nothing!\n");
            }
            break;
			
			case 29:
			{
				printf("do nothing!\n");

			}
			break;

			case 30:
            {
				printf("write:\n");
                int reg = strtol(arg[1], 0, 0);
				int val = strtol(arg[2], 0, 0);
				printf("spi write reg: 0x%02x.\n", reg);

				SSD2828_write_reg(1, reg, val);
            }
            break;

			case 31:
				{
					Generic_Write(2, 0x00, 0x00);
					Generic_Write(4, 0xff, 0x80, 0x19, 0x01);
					Generic_Write(2, 0x00, 0x80);
					Generic_Write(3, 0xff, 0x80, 0x19);
				}
				break;

			case 32: //mtpon
				{
					int channel = strtol(arg[1], 0, 0);
					//int val = strtol(arg[2], 0, 0);
					mtp_power_on(channel);
					printf("\n==========mtp_power_on %d\n", channel);
				}
				break;

			case 33: //mtpoff
				{
					int channel = strtol(arg[1], 0, 0);
					//int val = strtol(arg[2], 0, 0);
					//mtp_power_off(channel); 
					printf("\n==========mtp_power_off %d\n", channel);
					extern volatile int is_mtp_exit;
					is_mtp_exit = 1;
				}
				break;

			case 34:
				{
					unsigned char mipi_channel = 1;
					unsigned char type = 1;	// 0: generic; 1: dcs.
					unsigned char reg = strtol(arg[2], 0, 0);
					unsigned char data = 0x00;
					unsigned char page = strtol(arg[1], 0, 0);
					int vcom = 50;

					page = 1;
					reg = 0x53;

					vcom = 50;

					ili_9806_set_page(mipi_channel, page);
					ili_9806_write_vcom(mipi_channel, vcom, 1);

					
					int i = 1000;
					for (; i ; i --)
					{
						
	    				ReadModReg(mipi_channel, type, reg, 1, &data);
						printf("i = %d.\n", i);
						printf("read page: %d, reg:%02x: => %02x.\n", page, reg, data);

						if (data != vcom)
						{
							printf("read error!\n");
							break;
						}
					}
				}
				break;

			case 35:
			{
				int channel = strtol(arg[1], 0, 0);
				int addr = strtol(arg[2], 0, 0);
				int value = 0;
				vspi_read(channel, addr&0xff, &value);

				printf("vspi_read: channel<%d>, addr<0x%x>, value<0x%0x>\r\n", channel, addr&0xff, value);
			}
				break;

			case 36:
			{
				int channel = strtol(arg[1], 0, 0);
				int addr = strtol(arg[2], 0, 0);
				int value = strtol(arg[3], 0, 0);
				vspi_write(channel, addr&0xff, value);

				printf("vspi_write: channel<%d>, addr<0x%x>, value<0x%0x>\r\n", channel, addr&0xff, value);
			}
				break;

			case 37:
			{
				int pin = strtol(arg[1], 0, 0);
				int value = gpin_read_value(pin);
				printf("pin:%d, value:0x%x\r\n", pin, value);
			}
			break;

			case 38:
			{
				int slaveAddr = strtol(arg[1], 0, 0);
				int regAddr = strtol(arg[2], 0, 0);
				int value = strtol(arg[3], 0, 0);
				HI_UNF_I2C_Write2(slaveAddr&0xff, regAddr&0xff, value&0xff);

				printf("write slaveAddr:0x%x, regAddr:0x%x, value:0x%x\r\n", slaveAddr, regAddr, value);
			}
			break;

			case 39:
			{
				int slaveAddr = strtol(arg[1], 0, 0);
				int regAddr = strtol(arg[2], 0, 0);
				unsigned char value = 0;
				HI_UNF_I2C_Read2(slaveAddr&0xff, regAddr&0xff, &value);

				printf("read slaveAddr:0x%x, regAddr:0x%x, value:0x%x\r\n", slaveAddr, regAddr, value);
			}
			break;

			case 40:
			{
				int channel = strtol(arg[1], 0, 0);
				int slaveAddr = strtol(arg[2], 0, 0);
				int regAddr = strtol(arg[3], 0, 0);
				int value = strtol(arg[4], 0, 0);
				vi2c_write(channel, slaveAddr&0xff, regAddr&0xff, value&0xff);
				printf("vi2c_write channel:%d, slaveAddr:0x%x, regAddr:0x%x, value:0x%x\r\n", channel, slaveAddr, regAddr, value);
			}
			break;

			case 41:
			{
				int channel = strtol(arg[1], 0, 0);
				int slaveAddr = strtol(arg[2], 0, 0);
				int regAddr = strtol(arg[3], 0, 0);
				unsigned char value = 0;
				vi2c_read(channel, slaveAddr&0xff, regAddr&0xff, &value);
				printf("read channel: %d, slaveAddr:0x%x, regAddr:0x%x, value:0x%x\r\n", channel, slaveAddr, regAddr, value);
			}
			break;

			case 42:
			{
				int channel = strtol(arg[1], 0, 0);
				unsigned char value = readVcom(channel);
				printf("read vcom: channel <%d>, value <%d>\r\n", channel, value);
			}
				break;

			case 43:
			{
				int channel = strtol(arg[1], 0, 0);
				int value = strtol(arg[2], 0, 0);
				writeVcom(channel, value & 0xff);
				printf("write vcom: channel <%d>, value <%d>\r\n", channel, value & 0xff);
			}
				break;

			case 44:
			{
				int channel = strtol(arg[1], 0, 0);
				int value = strtol(arg[2], 0, 0);
				burnVcom(channel, value & 0xff);
				printf("burn vcom: channel<%d>, value<%d>\r\n", channel, value & 0xff);
			}
				break;

			case 45:
			{
				int channel = strtol(arg[1], 0, 0);
				int times = 0;
				times = readVcomOtpTimes(channel);

				printf("burn vcom times: channel<%d>, value <%d>\r\n", channel, times);
			}
				break;

            default:
                show_help();
                memset(usr_cmd, '\0',sizeof(usr_cmd));
                break;
        }
    }
    return 1;
}

volatile int is_mtp_exit = 0;

void refresh_lcd_msg_pos()
{
	int lcd_width = 0;
	int lcd_height = 0;
	get_module_res(&lcd_width, &lcd_height);
	
	// set lcd text message.
	set_fpga_text_show(0,0,0,0);

	int opt_w = 300;
	int opt_h = 240;
	int text_w = 0;
	int text_h = 65;

	int otp_x = (lcd_width - opt_w) / 2;
	int otp_y = lcd_height - text_h * 2 - opt_h;

	int text_x = 5;
	int text_y = lcd_height - text_h * 2;
	
	set_fpga_text_x(m_channel_1, text_x);
	set_fpga_text_x(m_channel_2, text_x);
	set_fpga_text_y(m_channel_1, text_y);
	set_fpga_text_y(m_channel_2, text_y);
	set_fpga_otp_x(m_channel_1, otp_x);
	set_fpga_otp_x(m_channel_2, otp_x);
	set_fpga_otp_y(m_channel_1, otp_y);
	set_fpga_otp_y(m_channel_2, otp_y);
}

void show_lcd_msg(int mipi_channel, int show_vcom, int show_ok, int vcom, int flick, int ok1, int ok2)
{
	#define LCD_MSG_COLOR_OK		(0)
	#define LCD_MSG_COLOR_NG		(1)
	
	if (show_ok)
	{
		if (ok1)
			set_fpga_text_color(1, LCD_MSG_COLOR_OK);
		else
			set_fpga_text_color(1, LCD_MSG_COLOR_NG);

		if (ok2)
			set_fpga_text_color(4, LCD_MSG_COLOR_OK);
		else
			set_fpga_text_color(4, LCD_MSG_COLOR_NG);
	}
	else
	{
		set_fpga_text_color(mipi_channel, LCD_MSG_COLOR_OK);
	}
	
	set_fpga_text_show(show_vcom, show_ok, ok1, ok2); //VCOM, OTP, OK, OK2
	set_fpga_text_vcom(mipi_channel, vcom);
	set_fpga_text_flick(mipi_channel, flick);
}

#define LOCKFILE "/home/pgClient.pid" 
#define LOCKMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

/* set advisory lock on file */
int lockfile(int fd)
{
	struct flock fl;

	fl.l_type = F_WRLCK;  /* write lock */
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;  //lock the whole file

	return(fcntl(fd, F_SETLK, &fl));
}

int already_running(const char *filename)
{
	int fd;
	char buf[16];

	fd = open(filename, O_RDWR | O_CREAT, LOCKMODE);
	if (fd < 0) {
		printf("can't open %s: %m\n", filename);
		exit(1);
	}

	if (lockfile(fd) == -1) {
		if (errno == EACCES || errno == EAGAIN) {
			printf("file: %s already locked", filename);
			close(fd);
			return 1;
		}
		
		printf("can't lock %s: %m\n", filename);
		exit(1);
	}
	
	ftruncate(fd, 0);
	sprintf(buf, "%ld", (long)getpid());
	write(fd, buf, strlen(buf) + 1);
	return 0;
}

static int init_hotplug_sock(void)  
{  

	struct sockaddr_nl snl;  
	const int buffersize = 16 * 1024 * 1024;  
	int retval;  

	memset(&snl, 0x00, sizeof(struct sockaddr_nl));  
	snl.nl_family = AF_NETLINK;  
	snl.nl_pid = getpid();  
	snl.nl_groups = 1;  

	int hotplug_sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);  
	if (hotplug_sock == -1)   
	{  
		printf("error getting socket: %s", strerror(errno));  
		return -1;  
	}  

	/* set receive buffersize */  
	setsockopt(hotplug_sock, SOL_SOCKET, SO_RCVBUFFORCE, &buffersize, sizeof(buffersize));  
	retval = bind(hotplug_sock, (struct sockaddr *) &snl, sizeof(struct sockaddr_nl));  
	if (retval < 0) {  
		printf("bind failed: %s", strerror(errno));  
		close(hotplug_sock);  
		hotplug_sock = -1;  
		return -1;  
	}  

	return hotplug_sock;  
}

//
void main_cleanup_null(int ret)
{
	
}

void main_cleanup(int ret)
{
	printf("\n%d\n\n", ret);
	mtp_power_off(1);
	mtp_power_off(2);
	mtp_power_off(3);
	mtp_power_off(4);

	spiCtrlfree();
	HI_UNF_I2C_DeInit();
	HI_UNF_GPIO_DeInit();
	exit(ret);
}

#define NETWORK_CFG_FILE		"/mnt/mmcblk0p1/eth_cfg"
#define NETWORK_MAC_CFG_FILE	"/mnt/mmcblk0p1/mac_cfg"

typedef struct tab_network_info
{
	char tag_ip[3];			// "IP "
	char ip[16];

	char tag_netmask[8];	// "NETMASK "
	char netmask[16];

	char tag_gateway[8];	// "GATEWAY "
	char gateway[16];

}network_info_t;

typedef struct tab_network_mac_info
{
	char tag_mac[4];	// "MAC"
	char mac[18];		// "xx:xx:xx:xx:xx:xx"
}network_mac_info_t;


int net_work_config_write(char* p_str_ip, char* p_str_netmask, char* p_str_gateway)
{
	FILE* f = NULL;

	if ( (p_str_ip == NULL) || (p_str_netmask == NULL) || (p_str_gateway == NULL) )
	{
		printf("net_work_config_write error: invalid param!\n");
		return -1;
	}

	f = fopen(NETWORK_CFG_FILE, "wb");
	if (f == NULL)
	{
		printf("net_work_config_write error: open file %s failed!\n", NETWORK_CFG_FILE);
		return -1;
	}

	network_info_t net_info = { 0 };
	strcpy(net_info.tag_ip, "IP ");
	strcpy(net_info.ip, p_str_ip);

	strcpy(net_info.tag_netmask, "NETMASK ");
	strcpy(net_info.netmask, p_str_netmask);

	strcpy(net_info.tag_gateway, "GATEWAY ");
	strcpy(net_info.gateway, p_str_gateway);

	fwrite(&net_info, 1, sizeof(net_info), f);

	fclose(f);

	return 0;
}

int net_work_config_read(char* p_str_ip, int ip_buf_len, char* p_str_netmask, int netmask_buf_len,
							char* p_str_gateway, int gateway_buf_len)
{
	FILE* f = NULL;

	f = fopen(NETWORK_CFG_FILE, "rb");
	if (f == NULL)
	{
		printf("net_work_config_read error: open file %s failed!\n", NETWORK_CFG_FILE);
		return -1;
	}

	network_info_t net_info = { 0 };
	fread(&net_info, 1, sizeof(net_info), f);

	strcpy(p_str_ip, net_info.ip);
	strcpy(p_str_netmask, net_info.netmask);
	strcpy(p_str_gateway, net_info.gateway);

	fclose(f);

	return 0;
}

int net_work_mac_config_write(char* p_str_mac)
{
	FILE* f = NULL;

	if ( p_str_mac == NULL )
	{
		printf("net_work_mac_config_write error: invalid param!\n");
		return -1;
	}

	f = fopen(NETWORK_MAC_CFG_FILE, "wb");
	if (f == NULL)
	{
		printf("net_work_mac_config_write error: open file %s failed!\n", NETWORK_MAC_CFG_FILE);
		return -1;
	}

	network_mac_info_t net_mac_info = { 0 };
	strcpy(net_mac_info.tag_mac, "MAC ");
	strcpy(net_mac_info.mac, p_str_mac);
	fwrite(&net_mac_info, 1, sizeof(net_mac_info), f);

	fclose(f);

	return 0;
}

int net_work_mac_config_read(char* p_str_mac, int mac_buf_len)
{
	FILE* f = NULL;

	f = fopen(NETWORK_MAC_CFG_FILE, "rb");
	if (f == NULL)
	{
		printf("net_work_mac_config_read error: open file %s failed!\n", NETWORK_MAC_CFG_FILE);
		return -1;
	}

	network_mac_info_t mac_info = { 0 };
	fread(&mac_info, 1, sizeof(mac_info), f);

	strcpy(p_str_mac, mac_info.mac);

	fclose(f);

	return 0;
}

int net_work_config_save(char* str_ip, char* str_mask, char* str_gateway)
{
	printf("net_work_config_save: ip=%s, netmask=%s, gateway=%s.\n", str_ip, str_mask, str_gateway);
	net_work_config_write(str_ip, str_mask, str_gateway);
	return 0;
}

int net_work_mac_config_save(char* str_mac)
{
	if (str_mac)
	{
		printf("net_work_mac_config_save: mac=%s.\n", str_mac);
		net_work_mac_config_write(str_mac);
	}

	return 0;
}

int net_work_config()
{
	// init net device
	char* net_dev = "eth0";
	char str_ip[16] = "192.168.1.150";
	char str_mask[16] = "255.255.255.0";
	char str_gateway[16] = "192.168.1.1";

	char str_mac[18] = "70:B3:D5:00:00:00";

	int ip_buf_len = 16;
	int netmask_buf_len = 16;
	int gateway_buf_len = 16;
	int mac_buf_len = 18;

	net_work_config_read(str_ip, ip_buf_len, str_mask, netmask_buf_len, str_gateway, gateway_buf_len);
	net_work_mac_config_read(str_mac, mac_buf_len);

	printf("=== NetWork Info ===\n");
	printf("IP: %s\n", str_ip);
	printf("NetMask: %s\n", str_mask);
	printf("GateWay: %s\n", str_gateway);
	printf("MAC: %s\n", str_mac);

	char str_cmd[128] = { 0 };

	//sprintf(str_cmd,"ifconfig eth0 %s netmask %s", str_ip, str_mask);
	system("ifconfig eth0 down");

	// set mac.
	sprintf(str_cmd, "ifconfig %s hw ether %s", net_dev, str_mac);
	system(str_cmd);
	printf("Set MAC Addr: %s\n", str_cmd);

	sprintf(str_cmd,"ifconfig eth0 %s netmask %s", str_ip, str_mask);
	system(str_cmd);

	sprintf(str_cmd,"route add default gw %s", str_gateway);
	system(str_cmd);

	system("ifconfig eth0 up");
	return 0;
}

int do_factory_reset()
{
	printf("do_factory_reset ...\n");
	system("rm /mnt/mmcblk0p1/eth_cfg -f");
	system("rm /mnt/mmcblk0p1/mac_cfg -f");

	system("rm /mnt/mmcblk0p2/cfg -rf");
	system("rm /mnt/mmcblk0p2/local.db -f");

	printf("do_factory_reset end.\n");
	return 0;
}

// pgClient -mport 8888
int main(int argc, char * argv[])
{
	if(already_running(LOCKFILE))
	{
		char txt[100] = "";
		FILE* fp = fopen(LOCKFILE, "rb");
		if(fp != NULL)
		{
			memset(txt, 0, sizeof(txt));
			fread(txt, sizeof(txt), 1, fp);
			fclose(fp);
		}
		printf("pgClient: running... %s %s %s \n", __DATE__, __TIME__, txt);
		return 0;
	}
	
	int multicast_port = MCAST_PORT;
	//multicast_port = 9999;
	
	if (argc > 1)
	{
		// check params.
		int i = 0;
		for (i = 1; i < argc; i ++)
		{
			if (strcmp(argv[i], "-mport") == 0)
			{
				i ++;

				if (i < argc)
				{
					multicast_port = strtol(argv[i], 0, 0);
				}
			}
		}
	}
	
	printf("%s running, multicast port: %d.\n", argv[0], multicast_port);
	show_version();	

	signal(SIGINT , main_cleanup); /* Interrupt (ANSI).    */
	signal(SIGTERM, main_cleanup); /* Termination (ANSI).  */
	signal(SIGILL, main_cleanup);                               //SIGILL 4 C ??????
	signal(SIGFPE, main_cleanup);                               //SIGFPE 8 C ??????
	signal(SIGKILL, main_cleanup);                              //SIGKILL 9 AEF Kill???
	signal(SIGSEGV, main_cleanup);                              //SIGSEGV 11 C ??��?????????
	signal(SIGPIPE, main_cleanup_null); //2015-12-28
   
	printf("start main ...\n");

	MsOS_TimerInit(); 
	HI_UNF_GPIO_Init();
    HI_UNF_I2C_Init();
    spiCtrlInit();
	ic_mgr_init();
	mtp_power_off(1);
	mtp_power_off(2);
	mtp_power_off(3);
	mtp_power_off(4);

    uartInit();

	int nRet = initFpga();
	if (SUCCESS != nRet)
	{
		printf("initFpga failed.\r\n");
		//return FAILED;
	}

	// CA310 Module
    nRet = initLcd();
	if (SUCCESS != nRet)
	{
		printf("initLcd failed.\r\n");
		//return FAILED;
    }
	
	pwrTaskInit();  //电源管理模块初始化
    localDBInit();
    clock_5338_init();
    move_ptn_init();
    pwrSetState(false, PWR_CHANNEL_ALL);
    client_rebackPower(0);
    load_cur_module();
	initBox();

	client_init(multicast_port);
#ifdef TM_MES_START
	init_tm_mes();
#endif
	usleep(2 * 1000 * 1000);
	pg_do_box_home_proc();

	mipi2828ModuleInit();

	client_cmd();

    return 0;
}

//
int string_number_to_digit_number(char c)
{
	int v = 0;
	switch(c){
	case '0':
		v = 0;
		break;

	case '1':
		v = 1;

		break;
	case '2':
		v = 2;
		break;

	case '3':
		v = 3;
		break;

	case '4':
		v = 4;
		break;

	case '5':
		v = 5;
		break;

	case '6':
		v = 6;
		break;

	case '7':
		v = 7;
		break;

	case '8':
		v = 8;
		break;

	case '9':
		v = 9;
		break;

	default:
		v = 0;
		break;
	}
	return v;
}

int convert_to_fpga_text_val(float val)
{
	int fpga_val = 0;
	char txt[100];
	sprintf(txt, "%f", val); // "100.000"  "10.000"  "1.000"

	char* ptr_dot = strstr(txt, ".");
	if(ptr_dot != NULL)
	{
		*ptr_dot = 0;
		ptr_dot += 1;
	}

	char* ptr = txt;
	int len = strlen(txt);

	if(len >= 3)
	{
		int v = string_number_to_digit_number(ptr[0]);
		fpga_val |= (v << 12);
		ptr += 1;
	}

	if(len >= 2)
	{
		int v = string_number_to_digit_number(ptr[0]);
		fpga_val |= (v << 8);
		ptr += 1;
	}

	if(len >= 1)
	{
		int v = string_number_to_digit_number(ptr[0]);
		fpga_val |= (v << 4);
		ptr += 1;
	}

	if(ptr_dot != NULL)
	{
		int v = string_number_to_digit_number(ptr_dot[0]);
		fpga_val |= v;
		//ptr += 1;
	}

	return fpga_val;
}

void set_fpga_text_y(int channel, int y)
{
	if(channel == 1 || channel == 3)
	{
		fpga_write(0x45, y);
	}
	else
	{

	}
}

void set_fpga_text_x(int channel, int x)
{
	if(channel == 1 || channel == 3)
	{
		fpga_write(0x44, x);
	}
	else
	{

	}
}

void set_fpga_otp_y(int channel, int y)
{
	if(channel == 1 || channel == 3)
	{
		fpga_write(0x4A, y);
	}
	else
	{

	}
}

void set_fpga_otp_x(int channel, int x)
{
	if(channel == 1 || channel == 3)
	{
		fpga_write(0x49, x);
	}
	else
	{

	}
}

void set_fpga_text_color(int channel, int red)
{
	//FPGA REG42~REG43
	// Flick??Vcom??OTP???????????
	// ?????????????????30bits???????????????
	// FPGA_DDR_REG42???16bits???????FPGA_DDR_REG1F???14bits???????
	// Bit29~bit20?????????????????��0~1023??
	// Bit19~bit10?????????????????��0~1023??
	// Bit9~bit0?????????????????��0~1023??

	if(red)
	{
		if(channel == 1 || channel == 3) //1,3
		{
			fpga_write(0x42, 0x3ff);
			fpga_write(0x43, 0);
		}
		else //2,4
		{
			fpga_write(0x4B, 0x3ff);
			fpga_write(0x4C, 0);
		}
	}
	else
	{
		if(channel == 1 || channel == 3) //1,3
		{
			fpga_write(0x42, 0);
			fpga_write(0x43, 0x3fc0);
		}
		else //2,4
		{
			fpga_write(0x4B, 0);
			fpga_write(0x4C, 0x3fc0);
		}
	}
}

void set_fpga_text_show(int vcom, int otp, int ok, int ok2)
{
	int ret = 0;
	if(vcom) //VCOM, FLICK
	{
		ret |= 0x01; //001  Bit0???Flick,Vcom??????????
	}

	if(otp) //OTP
	{
		ret |= 0x02; //010  Bit1???OTP??????????
	}

	if(ok)
	{
		ret |= 0x04; //100  Bit2?OTP flag????????????OK?????????NG??
		//set_fpga_text_color(m_channel_1, 0); //???
		//client_pg_writeReg(0x42, 0);
		//client_pg_writeReg(0x43, 0x3fc0);
	}
	else
	{
		//set_fpga_text_color(m_channel_1, 1); //???
		//client_pg_writeReg(0x42, 0x3ff);
		//client_pg_writeReg(0x43, 0);
	}

	if(ok2) //???2
	{
		ret |= 0x08; //1000  Bit3?OTP flag????????????OK?????????NG??
		//set_fpga_text_color(m_channel_2, 0); //???
	}
	else
	{
		//set_fpga_text_color(m_channel_2, 1); //???
	}

	fpga_write(0x46, ret);
}

void set_fpga_text_vcom(int channel, int vcom)
{
	int val = 0;
	if(vcom > 0)
	{
		float v = (float)vcom;
		val = convert_to_fpga_text_val(v);
	}
	if(channel == 1 || channel == 3)
	{
		fpga_write(0x41, val);
	}
	else
	{
		fpga_write(0x4E, val);
	}
}

void set_fpga_text_flick(int channel, int flick)
{
	int val = 0;
	if(flick > 0)
	{
		float v = (float)flick;
		v = v / 100.0;
		val = convert_to_fpga_text_val(v);
	}
	if(channel == 1 || channel == 3)
	{
		fpga_write(0x40, val);
	}
	else
	{
		fpga_write(0x4D, val);
	}
}

