#ifndef _MONITOR_CONFIG_H_
#define _MONITOR_CONFIG_H_

#define MONITOR_CFG_INI_FILE_PATH  "cfg/monitor_cfg/"

#define MAX_V_TEST_NUMS		(4)
typedef struct tag_v_test_info_t
{
	int enable;
	double f_min_val;
	double f_max_val;
}v_test_info_t;

#define OPEN_SHORT_MAX_SINGAL_GROUP_NUMS		(4)
typedef struct tag_monitor_cfg_info_s
{
	// NTC
	int ntc_enable;	
	int ntc_min_check_val;
	int ntc_max_check_val;

	// Open/Short
	int open_short_enable;
	//int open_short_check_mode;
	int open_short_check_open_value;
	int open_short_check_short_value;

	unsigned char io_selecter;
	unsigned char single_selecter[OPEN_SHORT_MAX_SINGAL_GROUP_NUMS];

	// V Test
	//v_test_info_t v_test[MAX_V_TEST_NUMS];
	
}monitor_cfg_info_t;

int monitor_config_read_cfg(char *str_monitor_cfg_name, monitor_cfg_info_t *p_monitor_cfg_info);

#endif

