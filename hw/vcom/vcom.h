#ifndef __VCOM_H__
#define __VCOM_H__


typedef struct tag_vcom_info_s{
    int maxvalue;
    int minvalue;
    int isBurner;
    int isVcomCheck;
    int type; //0:spi,1:i2c
    int i2cChannel;
    int i2cDevAddr;
    int i2cRegAddr;
	int vcomotpmax;
	int vcomBurner;
	int idBurner;
	int gammaBurner;
	int codeBurner;
	int pwrOnDelay;
	int gammaotpmax;
	int readDelay;
	int initVcom;
	int id1;
	int id2;
	int id3;
	float f_min_valid_flick;
	float f_max_valid_flick;
	float f_ok_flick;
}vcom_info_t;


#define MAX_GAMMA_REG_NUMS		(255)
typedef struct tag_gamma_cfg
{
	int enable_gamma_reg;
	int gamma_reg_nums;
	unsigned char gamma_reg[MAX_GAMMA_REG_NUMS];
}gamma_cfg_t;

#endif
