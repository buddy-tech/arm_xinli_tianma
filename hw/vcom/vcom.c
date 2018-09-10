#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "vcom.h"
#include "mypath.h"
#include "util/debug.h"
#include "ini/iniparser.h"
#include "common.h"

int load_gamma_config(gamma_cfg_t *p_gamma_cfg, char *fileName)
{
	char filePath[256];
    char section[256];
    dictionary	*ini = NULL;

	if ( (p_gamma_cfg == NULL) || (fileName == NULL) )
	{
		printf("load_gamma_config error: Invalid Param!\n");
		return -1;
	}
	
    sprintf(filePath, "%s%s", GAMMA_PATH, fileName);
    ini = iniparser_load(filePath);
    if (ini == NULL)
    {
    	printf("load_gamma_config error: open %s failed!\n", filePath);
		return -1;
	}	
	
    p_gamma_cfg->enable_gamma_reg = 0;
	p_gamma_cfg->gamma_reg_nums = 0;

	p_gamma_cfg->enable_gamma_reg = iniparser_getboolean(ini, "init_data:enable_gamma_reg", 0);
	p_gamma_cfg->gamma_reg_nums = iniparser_getint(ini, "init_data:gamma_reg_nums", 0);
	char *p_gamma_reg_data = iniparser_getstring(ini, "init_data:gamma_reg", "");

	printf("enable_gamma_reg: %d.\n", p_gamma_cfg->enable_gamma_reg);
	printf("gamma data: %s\n", p_gamma_reg_data);

	// parse gamma data.
	int i = 0;
	char* new_str = p_gamma_reg_data;
	char* temp_str = NULL;
	for ( ; i < p_gamma_cfg->gamma_reg_nums, new_str; i ++)
	{
		sscanf(new_str, "%x", p_gamma_cfg->gamma_reg + i);

		temp_str = new_str;
		new_str = strstr(temp_str, " ");
		if (new_str)
			new_str ++;
		//printf("\na = %d, %x, \nnew_str: %s.", p_gamma_cfg->gamma_reg[i], p_gamma_cfg->gamma_reg[i], new_str);
	}
	 
    iniparser_freedict(ini);

	printf("dump gamma data.\n");
	dump_data1(p_gamma_cfg->gamma_reg, p_gamma_cfg->gamma_reg_nums);

	return 0;
}

void read_vcom_config(vcom_info_t *vcom_info,char *fileName)
{
    char filePath[256];
    char section[256];
    dictionary	*ini;
    sprintf(filePath,"%s%s",VCOM_PATH,fileName);
    ini = iniparser_load(filePath);
    if (ini==NULL)
    {
        printf("read_vcom_config: cannot parse vcom config: %s.\n", filePath);
        vcom_info->maxvalue = 120;
        vcom_info->minvalue = 90;
        vcom_info->type = 0; //spi,1:i2c

		vcom_info->f_min_valid_flick = 0.5f;
		vcom_info->f_max_valid_flick = 10.0f;
		vcom_info->f_ok_flick = 2.0f;
		
        vcom_info->isBurner = 0;
        vcom_info->isVcomCheck = 0;
		vcom_info->vcomotpmax = 1;
		vcom_info->gammaotpmax = 0;
		vcom_info->vcomBurner = 0;
		vcom_info->idBurner = 0;
		vcom_info->gammaBurner = 0;
		vcom_info->codeBurner = 0;
		vcom_info->pwrOnDelay = 0;
		vcom_info->readDelay = 0;
		vcom_info->initVcom = 0;
		vcom_info->id1 = 0;
		vcom_info->id2 = 0;
		vcom_info->id3 = 0;
        return;
    }
	
    sprintf(section,"vcom:maxvalue");
    vcom_info->maxvalue = iniparser_getint(ini, section, 0);
    sprintf(section,"vcom:minvalue");
    vcom_info->minvalue = iniparser_getint(ini, section, 0);

	vcom_info->f_min_valid_flick = iniparser_getdouble(ini, "vcom:min_valid_flick", 0.5f);
	vcom_info->f_max_valid_flick = iniparser_getdouble(ini, "vcom:max_valid_flick", 10.0f);
	vcom_info->f_ok_flick = iniparser_getdouble(ini, "vcom:ok_flick", 2.0f);

	printf("vcom flick setting: min_flick: %f. max_flick: %f, ok_flick: %f.\n", vcom_info->f_min_valid_flick,
			vcom_info->f_max_valid_flick, vcom_info->f_ok_flick);
	
    sprintf(section,"vcom:isBurner");
    vcom_info->isBurner = iniparser_getint(ini, section, 0);
    sprintf(section,"vcom:isVcomCheck");
    vcom_info->isVcomCheck = iniparser_getint(ini, section, 0);
    sprintf(section,"vcom:type");
    vcom_info->type = iniparser_getint(ini, section, 0);
    sprintf(section,"vcom:i2cChannel");
    vcom_info->i2cChannel = iniparser_getint(ini, section, 0);
    sprintf(section,"vcom:i2cDevAddr");
    vcom_info->i2cDevAddr = iniparser_getint(ini, section, 0);
    sprintf(section,"vcom:i2cRegAddr");
    vcom_info->i2cRegAddr = iniparser_getint(ini, section, 0);

	sprintf(section,"vcom:vcomotpmax");
	vcom_info->vcomotpmax = iniparser_getint(ini, section, 0);

	sprintf(section,"vcom:idBurner");
	vcom_info->idBurner = iniparser_getint(ini, section, 0);
	sprintf(section,"vcom:vcomBurner");
	vcom_info->vcomBurner = iniparser_getint(ini, section, 0);
	sprintf(section,"vcom:gammaBurner");
	vcom_info->gammaBurner = iniparser_getint(ini, section, 0);
	sprintf(section,"vcom:codeBurner");
	vcom_info->codeBurner = iniparser_getint(ini, section, 0);
	
	sprintf(section,"vcom:pwrOnDelay");
	vcom_info->pwrOnDelay = iniparser_getint(ini, section, 0);
	sprintf(section,"vcom:gammaotpmax");
	vcom_info->gammaotpmax = iniparser_getint(ini, section, 0);
	sprintf(section,"vcom:readDelay");
	vcom_info->readDelay = iniparser_getint(ini, section, 0);
	sprintf(section,"vcom:initVcom");
	vcom_info->initVcom = iniparser_getint(ini, section, 0);

	sprintf(section,"vcom:id1");
	vcom_info->id1 = iniparser_getint(ini, section, 0);
	sprintf(section,"vcom:id2");
	vcom_info->id2 = iniparser_getint(ini, section, 0);
	sprintf(section,"vcom:id3");
	vcom_info->id3 = iniparser_getint(ini, section, 0);

	printf("vcomotpmax = %d.\n", vcom_info->vcomotpmax);
	
    iniparser_freedict(ini);
}
