#include "monitor_config.h"

#include "ini/iniparser.h"


int monitor_config_read_cfg(char *str_monitor_cfg_name, monitor_cfg_info_t *p_monitor_cfg_info)
{
    int i;
    char str_monitor_cfg_path[256];
    char section[256];
    dictionary	*ini;
	
    sprintf(str_monitor_cfg_path, "%s/%s", MONITOR_CFG_INI_FILE_PATH, 
				str_monitor_cfg_name);
	
    ini = iniparser_load(str_monitor_cfg_path);
    if (ini == NULL)
    {
        printf("monitor_config_read_cfg error: cannot parse %s.\n", str_monitor_cfg_path);
        return -1;
    }
    else
    {
        printf("read monitor config: %s\n", str_monitor_cfg_path);
    }

    sprintf(section,"ntc:enable");
    p_monitor_cfg_info->ntc_enable = iniparser_getint(ini, section, 1);
    sprintf(section,"ntc:min_check_value");
    p_monitor_cfg_info->ntc_min_check_val = iniparser_getint(ini, section, 0);
    sprintf(section,"ntc:max_check_value");
    p_monitor_cfg_info->ntc_max_check_val = iniparser_getint(ini, section, 0);

	printf("ntc enable: %d.\n", p_monitor_cfg_info->ntc_enable);
	printf("ntc min: %d.\n", p_monitor_cfg_info->ntc_min_check_val);
	printf("ntc max: %d.\n", p_monitor_cfg_info->ntc_max_check_val);
	
    sprintf(section,"openshort:enable");
    p_monitor_cfg_info->open_short_enable = iniparser_getint(ini, section, 0);
    sprintf(section,"openshort:open_check_value");
    p_monitor_cfg_info->open_short_check_open_value = iniparser_getint(ini, section, 0);
    sprintf(section,"openshort:short_check_value");	
    p_monitor_cfg_info->open_short_check_short_value = iniparser_getint(ini, section, 0);
	
    sprintf(section,"openshort:io_selecter");
    p_monitor_cfg_info->io_selecter = iniparser_getint(ini, section, 0);
    sprintf(section,"openshort:singal_select_1");
    p_monitor_cfg_info->single_selecter[0] = iniparser_getint(ini, section, 0);
    sprintf(section,"openshort:singal_select_2");	
    p_monitor_cfg_info->single_selecter[1] = iniparser_getint(ini, section, 0);
    sprintf(section,"openshort:singal_select_3");
    p_monitor_cfg_info->single_selecter[2] = iniparser_getint(ini, section, 0);
    sprintf(section,"openshort:singal_select_4");	
    p_monitor_cfg_info->single_selecter[3] = iniparser_getint(ini, section, 0);
	
	printf("openshort enable: %d.\n", p_monitor_cfg_info->open_short_enable);
	printf("open value: %d.\n", p_monitor_cfg_info->open_short_check_open_value);
	printf("short value: %d.\n", p_monitor_cfg_info->open_short_check_short_value);
	printf("io selecter: 0x%02x.\n", p_monitor_cfg_info->io_selecter);
	printf("singal 1 selecter: 0x%02x.\n", p_monitor_cfg_info->single_selecter[0]);
	printf("singal 2 selecter: 0x%02x.\n", p_monitor_cfg_info->single_selecter[1]);
	printf("singal 3 selecter: 0x%02x.\n", p_monitor_cfg_info->single_selecter[2]);
	printf("singal 4 selecter: 0x%02x.\n", p_monitor_cfg_info->single_selecter[3]);
	
	iniparser_freedict(ini);

	return 0;
}

