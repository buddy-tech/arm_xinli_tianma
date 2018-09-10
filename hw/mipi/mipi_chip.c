#include "stdio.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

static int ilc8891_vcom;
static int ilc8891_vcom_otp;
static int ilc8891_otpStatus;
static int ilc8891_otp_check_flag = 0;

typedef struct tag_ilc8891_nvm_info_s
{
    char id1;
    char id2;
    char id3;
    char vcm1;
    char vcm2;
    char gamma_n;
    char gamma_m;
    char optstu;
}ilc8891_nvm_info_t;

static ilc8891_nvm_info_t ilc8891_nvm_info;

void ilc8891_vcom_page_code(char *pCode)
{
    char vcomPage[] = {0x39,0xff,0x03,0x98,0x81,0x01};
    if(pCode)
    {
        memcpy(pCode,vcomPage,sizeof(vcomPage));
    }
}

int ilc8891_vcom_get_byIndex(int vcomIndex)
{
    return vcomIndex+0x10;
}

static int vcom1_set = 1;
static int vcom2_set = 0;

void ilc8891_vcom_set_code(int vcom,char *pCode,int *codeCount)
{
    char vcomSetCode[256];
    int  count = 0;

    if(vcom1_set)
    {
        if(vcom<0x100)
        {
            vcomSetCode[count++] = 0x39;
            vcomSetCode[count++] = 0x52;
            vcomSetCode[count++] = 1;   //len
            vcomSetCode[count++] = 0;
        }
        else
        {
            vcomSetCode[count++]  = 0x39;
            vcomSetCode[count++]  = 0x52;
            vcomSetCode[count++]  = 1;   //len
            vcomSetCode[count++]  = 1;
        }
        vcomSetCode[count++] = 0x39;
        vcomSetCode[count++] = 0x53;
        vcomSetCode[count++] = 1;   //len
        vcomSetCode[count++] = vcom;
    }
    if(vcom2_set)
    {
        if(vcom<0x100)
        {
            vcomSetCode[count++] = 0x39;
            vcomSetCode[count++] = 0x54;
            vcomSetCode[count++] = 1;   //len
            vcomSetCode[count++] = 0;
        }
        else
        {
            vcomSetCode[count++]  = 0x39;
            vcomSetCode[count++]  = 0x54;
            vcomSetCode[count++]  = 1;   //len
            vcomSetCode[count++]  = 1;
        }
        vcomSetCode[count++] = 0x39;
        vcomSetCode[count++] = 0x55;
        vcomSetCode[count++] = 1;   //len
        vcomSetCode[count++] = vcom;
    }
    memcpy(pCode,vcomSetCode,count);
    *codeCount = count;
}

#ifdef ENABLE_2828_MODULE
void ilc8891_vcom_reset(int channel)
{
    char cmd[2];
    char page[] = {0x98,0x81,0x01};
    //1.read 0x56
    page[2] = 0x01;
    DCSLongWriteWithPara(channel,0xff,page,4);
    cmd[0] = 0;
    DCSLongWriteWithPara(channel,0x56,cmd,2);
}

void ilc8891_vcom_read(int channel)
{
    char reg[2];
    char page[] = {0x98,0x81,0x01};
    //1.read 0x56
    page[2] = 0x01;
    DCSLongWriteWithPara(channel,0xff,page,4);

    ReadModReg(channel, 1, 0x56, 1,&reg[0]);
    if(reg[0]&0x01)
    {
        page[2] = 0x04;
        DCSLongWriteWithPara(channel,0xff,page,4);
        ReadModReg(channel, 1, 0xc4, 1,&reg[0]);
        ReadModReg(channel, 1, 0xc5, 1,&reg[1]);
    }
    else
    {
        page[2] = 0x01;
        ReadModReg(channel, 1, 0x52, 1,&reg[0]);
        ReadModReg(channel, 1, 0x53, 1,&reg[1]);
    }
    ilc8891_vcom = (reg[0]<<8)|reg[1];
    printf("8891 vcom read1 is %d--%x:%x\n",ilc8891_vcom,reg[0],reg[1]);

    page[2] = 0x04;
    DCSLongWriteWithPara(channel,0xff,page,4);
    ReadModReg(channel, 1, 0xc4, 1,&reg[0]);
    ReadModReg(channel, 1, 0xc5, 1,&reg[1]);
    printf("8891 vcom read2 is %d--%x:%x\n",ilc8891_vcom,reg[0],reg[1]);
}


void ilc8891_otp_read(int channel)
{
    char reg;
    ilc8891_otp_check_flag = 0;
    char page[] = {0x98,0x81,0x01};
    //1.read 0xe8
    page[2] = 0x01;
    DCSLongWriteWithPara(channel,0xff,page,4);
	
    ReadModReg(channel, 1, 0xe6, 1,&reg);
    memset(&ilc8891_nvm_info,0,sizeof(ilc8891_nvm_info));
    ilc8891_nvm_info.id1 = reg &0x07;
    ilc8891_nvm_info.id2 = (reg &0x70)>>4;
	
    ReadModReg(channel, 1, 0xe7, 1,&reg);
    ilc8891_nvm_info.id3 = reg &0x07;
	
    ReadModReg(channel, 1, 0xe8, 1,&reg);
    ilc8891_nvm_info.vcm1 = reg &0x07;
    ilc8891_nvm_info.vcm2 = (reg &0x38)>>3;
    ilc8891_nvm_info.gamma_n = (reg & 0x40)>>6;
    ilc8891_nvm_info.gamma_m = (reg & 0x80)>>7;
	
    ReadModReg(channel, 1, 0xe9, 1,&reg);
    ilc8891_nvm_info.optstu = (reg&0x80)>>7;
    printf("id1 %d\n",ilc8891_nvm_info.id1);
    printf("id2 %d\n",ilc8891_nvm_info.id2);
    printf("id3 %d\n",ilc8891_nvm_info.id3);
    printf("vcm1 %d\n",ilc8891_nvm_info.vcm1);
    printf("vcm2 %d\n",ilc8891_nvm_info.vcm2);
    printf("gamma_n %d\n",ilc8891_nvm_info.gamma_n);
    printf("gamma_m %d\n",ilc8891_nvm_info.gamma_m);
    printf("optstu %d\n",ilc8891_nvm_info.optstu);
	
    ilc8891_otpStatus = 0;
    if(ilc8891_nvm_info.id1 != 0)
    {
        ilc8891_otpStatus |= 1<<0;
    }
    if(ilc8891_nvm_info.id2 != 0)
    {
        ilc8891_otpStatus |= 1<<1;
    }
    if(ilc8891_nvm_info.id3 != 0)
    {
        ilc8891_otpStatus |= 1<<2;
    }
    if(ilc8891_nvm_info.vcm1 != 0)
    {
        ilc8891_otpStatus |= 1<<3;
    }
    if(ilc8891_nvm_info.vcm2 != 0)
    {
        ilc8891_otpStatus |= 1<<4;
    }
    if(ilc8891_nvm_info.gamma_n != 0)
    {
        ilc8891_otpStatus |= 1<<5;
    }
    if(ilc8891_nvm_info.gamma_m != 0)
    {
        ilc8891_otpStatus |= 1<<6;
    }
	
    ilc8891_vcom_otp = 0;
    if((ilc8891_nvm_info.vcm1&0x07) == 0x07)
    {
        ilc8891_vcom_otp = 3;
    }
    else if((ilc8891_nvm_info.vcm1&0x03) == 0x03)
    {
        ilc8891_vcom_otp = 2;
    }
    else if((ilc8891_nvm_info.vcm1&0x01) == 0x01)
    {
        ilc8891_vcom_otp = 1;
    }
    else
    {
        ilc8891_vcom_otp = 0;
    }
}

int ilc8891_vcom_get()
{
    return ilc8891_vcom;
}

int ilc8891_vcom_otp_get()
{
    return ilc8891_vcom_otp;
}

int ilc8891_nvm_otp_get()
{
    return ilc8891_otpStatus;
}

void ilc8891_otp_check()
{
    ilc8891_otp_check_flag = 1;
}

int ilc8891_otp_isChecked()
{
    if(ilc8891_otp_check_flag)
        return ilc8891_vcom_otp;
    else
        return 1;
}

void ilc8891_burn_vcom(int channel,int vcomIndex)
{
    int vcom = ilc8891_vcom_get_byIndex(vcomIndex);
    printf("!!!!!!!!!!burn begin --%d!!!!!!!!!!!!!!!!!!!\n",vcom);
    char cmd[32];
    char reg;
    char page[] = {0x98,0x81,0x01};
	
    //1.reset the drive ic.
    page[2] = 0x00;
    DCSLongWriteWithPara(channel,0xff,page,4);
    reg = 0x01;
    DCSShortWriteNoPara(channel,reg);
	
    //2.delay(10) //10ms
    usleep(10000);
	
    //3.page 4/d7 write
    page[2] = 0x04;
    DCSLongWriteWithPara(channel,0xff,page,4);
    cmd[0] = 0x2a;
    DCSLongWriteWithPara(channel,0x6e,cmd,2);
    cmd[0] = 0x35;
    DCSLongWriteWithPara(channel,0x6f,cmd,2);
    cmd[0] = 0x04;//0x0c;
    DCSLongWriteWithPara(channel,0xd7,cmd,2);
    cmd[0] = 0xeb;
    DCSLongWriteWithPara(channel,0x8b,cmd,2);
	
    //4.page 0/11 write
    page[2] = 0x00;
    DCSLongWriteWithPara(channel,0xff,page,4);
    reg = 0x11;
    DCSShortWriteNoPara(channel,reg);
    //5.delay(120)
    usleep(120000);
    //6.ff 98 81 01
    page[2] = 0x01;
    DCSLongWriteWithPara(channel,0xff,page,4);

    //7.check e6~e8
    ReadModReg(channel/*Channel*/, 1, 0xe8, 1,&reg);
    if(((reg&0x07) ==0x07) || ((reg&0x038) ==0x038))
    {
        printf("reg e8 value is %d\n",reg);
        return;
    }
    //8.delay(10)
    usleep(10000);
    //9.write vcom1
    //e0-->value e1-->addr e2-->addr
#if 1
    if(vcom>255)
    {
        cmd[0] = 0x00;
        DCSLongWriteWithPara(channel,0xe2,cmd,2); //8-15  e2
        cmd[0] = 0x04;
        DCSLongWriteWithPara(channel,0xe1,cmd,2); //0-7   e1
        cmd[0] = 0x01;
        DCSLongWriteWithPara(channel,0xe0,cmd,2); //0-7   e0

        cmd[0] = 0x00;
        DCSLongWriteWithPara(channel,0xe2,cmd,2); //8-15  e2
        cmd[0] = 0x05;
        DCSLongWriteWithPara(channel,0xe1,cmd,2); //0-7   e1
        cmd[0] = vcom-255;
        DCSLongWriteWithPara(channel,0xe0,cmd,2); //0-7   e0
    }
    else
    {
        cmd[0] = 0x00;
        DCSLongWriteWithPara(channel,0xe2,cmd,2); //8-15  e2
        cmd[0] = 0x04;
        DCSLongWriteWithPara(channel,0xe1,cmd,2); //0-7   e1
        cmd[0] = 0x00;
        DCSLongWriteWithPara(channel,0xe0,cmd,2); //0-7   e0

        cmd[0] = 0x00;
        DCSLongWriteWithPara(channel,0xe2,cmd,2); //8-15  e2
        cmd[0] = 0x05;
        DCSLongWriteWithPara(channel,0xe1,cmd,2); //0-7   e1
        cmd[0] = vcom;
        DCSLongWriteWithPara(channel,0xe0,cmd,2); //0-7   e0
    }
    //e3-->55
    //e4-->aa
    //e5-->66
    cmd[0] = 0x55;
    DCSLongWriteWithPara(channel,0xe3,cmd,2); //8-15  e2
    cmd[0] = 0xaa;
    DCSLongWriteWithPara(channel,0xe4,cmd,2); //0-7   e1
    cmd[0] = 0x66;
    DCSLongWriteWithPara(channel,0xe5,cmd,2); //0-7   e0
#endif
    //10.write vcom2
    //e0-->value e1-->addr e2-->addr
#if 1
    if(vcom>255)
    {
        cmd[0] = 0x00;
        DCSLongWriteWithPara(channel,0xe2,cmd,2); //8-15  e2
        cmd[0] = 0x06;
        DCSLongWriteWithPara(channel,0xe1,cmd,2); //0-7   e1
        cmd[0] = 0x01;
        DCSLongWriteWithPara(channel,0xe0,cmd,2); //0-7   e0

        cmd[0] = 0x00;
        DCSLongWriteWithPara(channel,0xe2,cmd,2); //8-15  e2
        cmd[0] = 0x07;
        DCSLongWriteWithPara(channel,0xe1,cmd,2); //0-7   e1
        cmd[0] = vcom-255;
        DCSLongWriteWithPara(channel,0xe0,cmd,2); //0-7   e0
    }
    else
    {
        cmd[0] = 0x00;
        DCSLongWriteWithPara(channel,0xe2,cmd,2); //8-15  e2
        cmd[0] = 0x06;
        DCSLongWriteWithPara(channel,0xe1,cmd,2); //0-7   e1
        cmd[0] = 0x00;
        DCSLongWriteWithPara(channel,0xe0,cmd,2); //0-7   e0

        cmd[0] = 0x00;
        DCSLongWriteWithPara(channel,0xe2,cmd,2); //8-15  e2
        cmd[0] = 0x07;
        DCSLongWriteWithPara(channel,0xe1,cmd,2); //0-7   e1
        cmd[0] = vcom;
        DCSLongWriteWithPara(channel,0xe0,cmd,2); //0-7   e0
    }

    //e3-->55
    //e4-->aa
    //e5-->66
    cmd[0] = 0x55;
    DCSLongWriteWithPara(channel,0xe3,cmd,2); //8-15  e2
    cmd[0] = 0xaa;
    DCSLongWriteWithPara(channel,0xe4,cmd,2); //0-7   e1
    cmd[0] = 0x66;
    DCSLongWriteWithPara(channel,0xe5,cmd,2); //0-7   e0
#endif
    //delay(10)
    usleep(10000);
    //reset
    page[2] = 0x00;
    DCSLongWriteWithPara(channel,0xff,page,4);
    reg = 0x01;
    DCSShortWriteNoPara(channel,reg);

    printf("!!!!!!!!!!burn over!!!!!!!!!!!!!!!!!!!\n");
}


void otm8019a_enable_orise_command(int channel)
{
#if 1
	unsigned char cmd[100] = {
		0x39, 0x00, 0x01, 0x00,                 //00 00
		0x39, 0xff, 0x03, 0x80, 0x19, 0x01,     //FF 80 19 01
		0x39, 0x00, 0x01, 0x80,                 //00 80
		0x39, 0xff, 0x02, 0x80, 0x19,           //FF 80 19
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	SendCode(channel, cmd);
	printf("%d--------------------------------- SendCode 00 00; FF 80 19 01; 00 80; FF 80 19; \n", channel);
#else
	unsigned char cmd[100] = {
		0x39, 0xff, 0x03, 0x80, 0x19, 0x01,     //FF 80 19 01
		0x39, 0x00, 0x01, 0x80,                 //00 80
		0x39, 0xff, 0x02, 0x80, 0x19,           //FF 80 19
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	SendCode(channel, cmd);
	printf("%d--------------------------------- SendCode FF 80 19 01; 00 80; FF 80 19; \n", channel);
#endif
}

void otm8019a_address_shift(int channel)
{
	unsigned char cmd1[100] = {
		0x39, 0x00, 0x01, 0x00,  //00 00;
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	SendCode(channel, cmd1);
	printf("%d--------------------------------- SendCode 00 00; \n", channel);
}

void otm8019a_address_shift_end(int channel)
{
	unsigned char cmd1[100] = {
		0x39, 0x00, 0x01, 0x00, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	SendCode(channel, cmd1);
	printf("%d--------------------------------- SendCode 00 00; End \n", channel);
}

void otm8019a_vcom(int channel, int vcom)
{
	int n=0;
	unsigned char cmd[100];
	memset(cmd, 0xff, sizeof(cmd));
	//00 00
	cmd[n++] = 0x39;
	cmd[n++] = 0x00;
	cmd[n++] = 1;
	cmd[n++] = 0x00;
	//D9 xx
	cmd[n++] = 0x39;
	cmd[n++] = 0xD9;
	cmd[n++] = 1;
	cmd[n++] = vcom & 0xff;
	SendCode(channel, cmd);
	printf("%d--------------------------------- SendCode 00 00; D9 %02X; \n", channel, vcom & 0xff);
}

int otm8019a_read_otp(int channel, unsigned char* count)
{
	unsigned char cmd[100] = {
		0x39, 0x00, 0x01, 0x02, //00 02
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff}; 
	SendCode(channel, cmd);
	printf("%d--------------------------------- send 00 02; \n", channel);
	//
	unsigned char data[8] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
	int ret = ReadModReg(channel, 1, 0xF1, 1, data);
	short opt_count = 0;
	if(data[0] & 0x80) opt_count += 1;
	if(data[0] & 0x40) opt_count += 1;
	if(data[0] & 0x20) opt_count += 1;
	if(data[0] & 0x10) opt_count += 1;
	printf("%d--------------------------------- OTP-VCOM-TIMES 0xF1 = 0x%X (%d)   ret=%d\n", channel, data[0], opt_count, ret);
	*count = opt_count; //返回次数
	return ret;
}

int otm8019a_read_driver_id(int channel)
{
	unsigned char data[8] = {0,0,0,0,0,0,0,0};
	int ret = ReadModReg(channel, 1, 0xA1, 5, data);
	printf("%d--------------------------------- Driver_ID 0xA1 = %02X %02X %02X %02X %02X   ret=%d\n", channel, data[0], data[1], data[2], data[3], data[4], ret);
	//没有读取到任何信息
	if(ret == 0)
	{
		return 0;
	}
	//读取到全部是0
	if(data[0]==0 && data[1]==0 && data[2]==0 && data[3]==0 && data[4]==0)
	{
		return 0;
	}
	//正常是 01 8B 80 19 FF
	return 1;
}
#endif

