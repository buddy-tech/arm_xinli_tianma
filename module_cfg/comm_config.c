#include "ini/iniparser.h"

#include "comm_config.h"

int comm_config_read_cfg(char *str_comm_cfg_name, comm_cfg_info_t *p_comm_cfg_info)
{
    int i;
    char str_comm_cfg_path[256];
    char section[256];
    dictionary	*ini;
	
    sprintf(str_comm_cfg_path, "%s/%s", COMM_CFG_INI_FILE_PATH, 
				str_comm_cfg_name);
	
    ini = iniparser_load(str_comm_cfg_path);
    if (ini==NULL)
    {
        printf("comm_config_read_cfg error: cannot parse %s.\n", str_comm_cfg_path);
        return -1;
    }
    else
    {
        printf("read comm config: %s\n", str_comm_cfg_path);
    }

    sprintf(section,"spi:enable");
    p_comm_cfg_info->spi_enable = iniparser_getint(ini, section, 1);
	
    sprintf(section,"spi:speed_type");
    p_comm_cfg_info->spi_speed_type = iniparser_getint(ini, section, 0);
    sprintf(section,"spi:proto_type");
    p_comm_cfg_info->spi_proto_type = iniparser_getint(ini, section, 0);
	
    sprintf(section,"i2c:enable");
    p_comm_cfg_info->i2c_enable = iniparser_getint(ini, section, 0);
    sprintf(section,"i2c:speed_type");
    p_comm_cfg_info->i2c_speed_type = iniparser_getint(ini, section, 0);
    sprintf(section,"i2c:addr_bits");
    p_comm_cfg_info->i2c_addr_bits = iniparser_getint(ini, section, 0);
    sprintf(section,"i2c:data_bits");	
    p_comm_cfg_info->i2c_data_bits = iniparser_getint(ini, section, 0);

    iniparser_freedict(ini);

	return 0;
}


