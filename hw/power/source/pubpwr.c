#include "comStruct.h"
#include "ini/iniparser.h"

#define POWER_INI_FILE_PATH  "cfg/power/"

int read_power_config(char *pwrName, power_cfg_info_t *pPower_cfg)
{
	char pwrpath[256];
	char section[256];
	dictionary	*ini;

	if (pPower_cfg == NULL)
	{
		printf("read_power_config error: Invalid Param!\n");
		return -1;
	}
	
	sprintf(pwrpath,"%s/%s", POWER_INI_FILE_PATH, pwrName);
	ini = iniparser_load(pwrpath);
	if (ini==NULL)
	{
		printf("read_power_config error: cannot parse power cfg: %s. \n", pwrpath);
		return -1;
	}
	
	sprintf(section,"power:vdd");
	pPower_cfg->power_module.VDD= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vddFlyTime");
	pPower_cfg->power_module.VDDFlyTime= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vddOverLimit");
	pPower_cfg->power_module.VDDOverLimit= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vddUnderLimit");
	pPower_cfg->power_module.VDDUnderLimit= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vddCurrentOverLimit");
	pPower_cfg->power_module.VDDCurrentOverLimit= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vddCurrentUnderLimit");
	pPower_cfg->power_module.VDDCurrentUnderLimit= iniparser_getint(ini, section, 0);
	//sprintf(section,"power:vddCurrentShortLimit");
	//pPower_info->vddCurrentShortLimit= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vddOpenDelay");
	pPower_cfg->power_module.VDDOpenDelay= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vddCloseDelay");
	pPower_cfg->power_module.VDDCloseDelay= iniparser_getint(ini, section, 0);

	sprintf(section,"power:vbl");
	pPower_cfg->power_module.VBL= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vblFlyTime");
	pPower_cfg->power_module.VBLFlyTime= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vblOverLimit");
	pPower_cfg->power_module.VBLOverLimit= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vblUnderLimit");
	pPower_cfg->power_module.VBLUnderLimit= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vblCurrentOverLimit");
	pPower_cfg->power_module.VBLCurrentOverLimit= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vblCurrentUnderLimit");
	pPower_cfg->power_module.VBLCurrentUnderLimit= iniparser_getint(ini, section, 0);
	//sprintf(section,"power:vblCurrentShortLimit");
	//pPower_info->VBLCurrentShortLimit= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vblOpenDelay");
	pPower_cfg->power_module.VBLOpenDelay= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vblCloseDelay");
	pPower_cfg->power_module.VBLCloseDelay= iniparser_getint(ini, section, 0);

	sprintf(section,"power:vif");
	pPower_cfg->power_module.MTP= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vifFlyTime");
	pPower_cfg->power_module.MTPFlyTime= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vifOverLimit");
	pPower_cfg->power_module.MTPOverLimit= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vifUnderLimit");
	pPower_cfg->power_module.MTPUnderLimit= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vifCurrentOverLimit");
	pPower_cfg->power_module.MTPCurrentOverLimit= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vifCurrentUnderLimit");
	pPower_cfg->power_module.MTPCurrentUnderLimit= iniparser_getint(ini, section, 0);
	//sprintf(section,"power:vifCurrentShortLimit");
	//pPower_info->MTPCurrentShortLimit= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vifOpenDelay");
	pPower_cfg->power_module.MTPOpenDelay= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vifCloseDelay");
	pPower_cfg->power_module.MTPCloseDelay= iniparser_getint(ini, section, 0);

	sprintf(section,"power:avdd");
	pPower_cfg->power_module.VDDIO= iniparser_getint(ini, section, 0);
	sprintf(section,"power:avddFlyTime");
	pPower_cfg->power_module.VDDIOFlyTime= iniparser_getint(ini, section, 0);
	sprintf(section,"power:avddOverLimit");
	pPower_cfg->power_module.VDDIOOverLimit= iniparser_getint(ini, section, 0);
	sprintf(section,"power:avddUnderLimit");
	pPower_cfg->power_module.VDDIOUnderLimit= iniparser_getint(ini, section, 0);
	sprintf(section,"power:avddCurrentOverLimit");
	pPower_cfg->power_module.VDDIOCurrentOverLimit= iniparser_getint(ini, section, 0);
	sprintf(section,"power:avddCurrentUnderLimit");
	pPower_cfg->power_module.VDDIOCurrentUnderLimit= iniparser_getint(ini, section, 0);
	//sprintf(section,"power:avddCurrentShortLimit");
	//pPower_info->VDDIOCurrentShortLimit= iniparser_getint(ini, section, 0);
	sprintf(section,"power:avddOpenDelay");
	pPower_cfg->power_module.VDDIOOpenDelay= iniparser_getint(ini, section, 0);
	sprintf(section,"power:avddCloseDelay");
	pPower_cfg->power_module.VDDIOCloseDelay= iniparser_getint(ini, section, 0);

	sprintf(section,"power:vsp");
	pPower_cfg->power_module.VSP= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vspFlyTime");
	pPower_cfg->power_module.VSPFlyTime= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vspOverLimit");
	pPower_cfg->power_module.VSPOverLimit= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vspUnderLimit");
	pPower_cfg->power_module.VSPUnderLimit= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vspCurrentOverLimit");
	pPower_cfg->power_module.VSPCurrentOverLimit= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vspCurrentUnderLimit");
	pPower_cfg->power_module.VSPCurrentUnderLimit= iniparser_getint(ini, section, 0);
	//sprintf(section,"power:vspCurrentShortLimit");
	//pPower_info->VSPCurrentShortLimit= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vspOpenDelay");
	pPower_cfg->power_module.VSPOpenDelay= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vspCloseDelay");
	pPower_cfg->power_module.VSPCloseDelay= iniparser_getint(ini, section, 0);

	sprintf(section,"power:vsn");
	pPower_cfg->power_module.VSN= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vsnFlyTime");
	pPower_cfg->power_module.VSNFlyTime= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vsnOverLimit");
	pPower_cfg->power_module.VSNOverLimit= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vsnUnderLimit");
	pPower_cfg->power_module.VSNUnderLimit= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vsnCurrentOverLimit");
	pPower_cfg->power_module.VSNCurrentOverLimit= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vsnCurrentUnderLimit");
	pPower_cfg->power_module.VSNCurrentUnderLimit= iniparser_getint(ini, section, 0);
	//sprintf(section,"power:vsnCurrentShortLimit");
	//pPower_info->VSNCurrentShortLimit= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vsnOpenDelay");
	pPower_cfg->power_module.VSNOpenDelay= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vsnCloseDelay");
	pPower_cfg->power_module.VSNCloseDelay= iniparser_getint(ini, section, 0);

	sprintf(section,"power:elvdd");
	pPower_cfg->power_module.ELVDD= iniparser_getint(ini, section, 0);
	sprintf(section,"power:elvddFlyTime");
	pPower_cfg->power_module.ELVDDFlyTime= iniparser_getint(ini, section, 0);
	sprintf(section,"power:elvddOverLimit");
	pPower_cfg->power_module.ELVDDOverLimit= iniparser_getint(ini, section, 0);
	sprintf(section,"power:elvddUnderLimit");
	pPower_cfg->power_module.ELVDDUnderLimit= iniparser_getint(ini, section, 0);
	sprintf(section,"power:elvddCurrentOverLimit");
	pPower_cfg->power_module.ELVDDCurrentOverLimit= iniparser_getint(ini, section, 0);
	sprintf(section,"power:elvddCurrentUnderLimit");
	pPower_cfg->power_module.ELVDDCurrentUnderLimit= iniparser_getint(ini, section, 0);
	//sprintf(section,"power:elvddCurrentShortLimit");
	//pPower_info->VSNCurrentShortLimit= iniparser_getint(ini, section, 0);
	sprintf(section,"power:elvddOpenDelay");
	pPower_cfg->power_module.ELVDDOpenDelay= iniparser_getint(ini, section, 0);
	sprintf(section,"power:elvddCloseDelay");
	pPower_cfg->power_module.ELVDDCloseDelay= iniparser_getint(ini, section, 0);

	sprintf(section,"power:elvss");
	pPower_cfg->power_module.ELVSS= iniparser_getint(ini, section, 0);
	sprintf(section,"power:elvssFlyTime");
	pPower_cfg->power_module.ELVSSFlyTime= iniparser_getint(ini, section, 0);
	sprintf(section,"power:elvssOverLimit");
	pPower_cfg->power_module.ELVSSOverLimit= iniparser_getint(ini, section, 0);
	sprintf(section,"power:elvssUnderLimit");
	pPower_cfg->power_module.ELVSSUnderLimit= iniparser_getint(ini, section, 0);
	sprintf(section,"power:elvssCurrentOverLimit");
	pPower_cfg->power_module.ELVSSCurrentOverLimit= iniparser_getint(ini, section, 0);
	sprintf(section,"power:elvssCurrentUnderLimit");
	pPower_cfg->power_module.ELVSSCurrentUnderLimit= iniparser_getint(ini, section, 0);
	//sprintf(section,"power:elvssCurrentShortLimit");
	//pPower_info->ELVSSCurrentShortLimit= iniparser_getint(ini, section, 0);
	sprintf(section,"power:elvssOpenDelay");
	pPower_cfg->power_module.ELVSSOpenDelay= iniparser_getint(ini, section, 0);
	sprintf(section,"power:elvssCloseDelay");
	pPower_cfg->power_module.ELVSSCloseDelay= iniparser_getint(ini, section, 0);

	sprintf(section,"power:signalOpenDelay");
	pPower_cfg->power_module.signalOpenDelay= iniparser_getint(ini, section, 0);
	sprintf(section,"power:signalCloseDelay");
	pPower_cfg->power_module.signalCloseDelay= iniparser_getint(ini, section, 0);

	sprintf(section,"power:vdimOpenDelay");
	pPower_cfg->power_module.vdimOpenDelay= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vdimCloseDelay");
	pPower_cfg->power_module.vdimCloseDelay= iniparser_getint(ini, section, 0);
	sprintf(section,"power:vdim");
	pPower_cfg->power_module.vdim= iniparser_getint(ini, section, 0);

	sprintf(section,"power:pwmOpenDelay");
	pPower_cfg->power_module.pwmOpenDelay= iniparser_getint(ini, section, 0);
	sprintf(section,"power:pwmCloseDelay");
	pPower_cfg->power_module.pwmCloseDelay= iniparser_getint(ini, section, 0);
	sprintf(section,"power:pwm");
	pPower_cfg->power_module.pwm= iniparser_getint(ini, section, 0);
	sprintf(section,"power:pwmFreq");
	pPower_cfg->power_module.pwmFreq= iniparser_getint(ini, section, 0);
	sprintf(section,"power:pwmDuty");
	pPower_cfg->power_module.pwmDuty= iniparser_getint(ini, section, 0);

	sprintf(section,"power:invertOpenDelay");
	pPower_cfg->power_module.invertOpenDelay= iniparser_getint(ini, section, 0);
	sprintf(section,"power:invertCloseDelay");
	pPower_cfg->power_module.invertCloseDelay= iniparser_getint(ini, section, 0);
	sprintf(section,"power:invert");
	pPower_cfg->power_module.invert= iniparser_getint(ini, section, 0);

	sprintf(section,"power:LEDEnable");
	pPower_cfg->power_module.LEDEnable= iniparser_getint(ini, section, 0);
	sprintf(section,"power:LEDchannel");
	pPower_cfg->power_module.LEDChannel= iniparser_getint(ini, section, 0);
	sprintf(section,"power:LEDdriverCurrent");
	pPower_cfg->power_module.LEDCurrent= iniparser_getint(ini, section, 0);
	sprintf(section,"power:LEDnumber");
	pPower_cfg->power_module.LEDNumber= iniparser_getint(ini, section, 0);
	sprintf(section,"power:LEDdriverOVP");
	pPower_cfg->power_module.LEDOVP= iniparser_getint(ini, section, 0);
	sprintf(section,"power:LEDdriverUVP");
	pPower_cfg->power_module.LEDUVP= iniparser_getint(ini, section, 0);

	// gpio timing
	sprintf(section, "power:gpio1_OpenDelay");
	pPower_cfg->power_module.udOpenDelay = iniparser_getint(ini, section, 0);
	sprintf(section, "power:gpio1_CloseDelay");
	pPower_cfg->power_module.udCloseDelay = iniparser_getint(ini, section, 0);

	sprintf(section, "power:gpio2_OpenDelay");
	pPower_cfg->power_module.lrOpenDelay = iniparser_getint(ini, section, 0);
	sprintf(section, "power:gpio2_CloseDelay");
	pPower_cfg->power_module.lrCloseDelay  = iniparser_getint(ini, section, 0);

	sprintf(section, "power:gpio3_OpenDelay");
	pPower_cfg->power_module.modeOpenDelay = iniparser_getint(ini, section, 0);
	sprintf(section, "power:gpio3_CloseDelay");
	pPower_cfg->power_module.modeCloseDelay = iniparser_getint(ini, section, 0);

	sprintf(section, "power:gpio4_OpenDelay");
	pPower_cfg->power_module.stbyOpenDelay = iniparser_getint(ini, section, 0);
	sprintf(section, "power:gpio4_CloseDelay");
	pPower_cfg->power_module.stbyCloseDelay = iniparser_getint(ini, section, 0);

	sprintf(section, "power:gpio5_OpenDelay");
	pPower_cfg->power_module.rstOpenDelay = iniparser_getint(ini, section, 0);
	sprintf(section, "power:gpio5_CloseDelay");
	pPower_cfg->power_module.rstCloseDelay = iniparser_getint(ini, section, 0);

	// vcom
	sprintf(section, "power:vcom");
	pPower_cfg->vcom_set_volt = iniparser_getint(ini, section, 0);
	printf("==== VCOM: %d. =====\n", pPower_cfg->vcom_set_volt);
	sprintf(section, "power:vcomOverLimit");
	pPower_cfg->vcom_max_volt = iniparser_getint(ini, section, 0);
	sprintf(section, "power:vcomUnderLimit");
	pPower_cfg->vcom_min_volt = iniparser_getint(ini, section, 0);
	sprintf(section, "power:vcomCurrentOverLimit");
	pPower_cfg->vcom_max_current = iniparser_getint(ini, section, 0);
	
	iniparser_freedict(ini);
	return 0;
}

