#ifndef _COMM_CONFIG_H_
#define _COMM_CONFIG_H_

#define COMM_CFG_INI_FILE_PATH  "cfg/comm_cfg/"

typedef struct tag_comm_cfg_info_s
{
	// spi
    int	spi_enable;	// 0: disable; 1: enable;
	int	spi_speed_type;	// 0: normal; 1: standard; 2: fast;
	int	spi_proto_type;	

	// i2c
	int i2c_enable;		// 0: disable; 1: enable;
	int	i2c_speed_type;	// 0: normal; 1: standard; 2: fast;
	int	i2c_addr_bits;
	int	i2c_data_bits;	
	int i2c_chip_addr;
	
}comm_cfg_info_t;

int comm_config_read_cfg(char *str_comm_cfg_name, comm_cfg_info_t *p_comm_cfg_info);

#endif


