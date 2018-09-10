#include <stdio.h>
#include <sys/time.h>

#include "hi_unf_gpio.h"
#include "openshort.h"
#include "module_cfg/monitor_config.h"
#include "common.h"
#include "pubPwrTask.h"

void open_short_singal_notify_to_pc(int pin, int status)
{
	char msg[256] = "";
	
	if (status == PIN_STATU_OPEN)
		sprintf(msg, "NG: 开短路检测：PIN %d 处于开路状态!", pin + 1);
	else if (status == PIN_STATU_SHORT)
		sprintf(msg, "NG: 开短路检测：PIN %d 处于短路状态!", pin + 1);
	else
		sprintf(msg, "OK: 开短路检测：PIN %d 处于正常状态!", pin + 1);

	client_notify_msg(NOTIFY_ERROR_OPEN_SHORT_CHECK, msg);
}

void open_short_gpio_notify_to_pc(int pin, int status)
{
	char msg[256] = "";

	if (status == PIN_STATU_OPEN)
	{
		sprintf(msg, "NG: 开短路检测：GPIO %d 处于开路状态!", pin + 1);
	}
	else if (status == PIN_STATU_SHORT)
	{
		sprintf(msg, "NG: 开短路检测：GPIO %d 处于短路状态!", pin + 1);
	}
	else 
	{
		return ;
	}
		
	client_notify_msg(NOTIFY_ERROR_OPEN_SHORT_CHECK, msg);
}

static void reset_short_gpio(TTL_LVDS_CHANNEL_E channel)
{
	if (TTL_LVDS_CHANNEL_RIGHT == channel) //right
	{
		gpio_set_output_value(CHANNEL_ENABLE_GPIO_R, 0);
		gpio_set_output_value(GPIO_PIN_UD_R, 0);
		gpio_set_output_value(GPIO_PIN_LR_R, 0);
		gpio_set_output_value(GPIO_PIN_MODE_R, 0);
		gpio_set_output_value(GPIO_PIN_STBY_R, 0);
		gpio_set_output_value(GPIO_PIN_RST_R, 0);
	}
	else	//left
	{
		gpio_set_output_value(CHANNEL_ENABLE_GPIO_L, 0);
		gpio_set_output_value(GPIO_PIN_UD_L, 0);
		gpio_set_output_value(GPIO_PIN_LR_L, 0);
		gpio_set_output_value(GPIO_PIN_MODE_L, 0);
		gpio_set_output_value(GPIO_PIN_STBY_L, 0);
		gpio_set_output_value(GPIO_PIN_RST_L, 0);
	}
}

static int check_gpio_short(monitor_cfg_info_t* p_monitor_cfg, TTL_LVDS_CHANNEL_E channel, unsigned int gpioIndex)
{
	int ret = 0;
	gpio_set_output_value(gpioIndex, 1);

	_msleep(50);

	unsigned short value = mcu_read_open_short_data(channel);

	float shortValue = value * 0.1;
	printf("gpio<%d> high: %4.2f. Status:",gpioIndex, shortValue);
	gpio_set_output_value(gpioIndex, 0);

	int io_index = 0;
	if (shortValue < p_monitor_cfg->open_short_check_open_value)
	{
		printf("Open\r\n");
		open_short_gpio_notify_to_pc(gpioIndex, PIN_STATU_OPEN);
		ret = 1;
	}
	else if (shortValue > p_monitor_cfg->open_short_check_short_value)
	{
		printf("Short\r\n");
		open_short_gpio_notify_to_pc(gpioIndex, PIN_STATU_SHORT);
		ret = 1;
	}
	else
	{
		printf("Normal\r\n");
		open_short_gpio_notify_to_pc(gpioIndex, PIN_STATU_SHORT);
	}

	return ret;
}

int open_short_test_singal_and_io(monitor_cfg_info_t* p_monitor_cfg)
{
	int devChannel = 0;
	int checkRet[2] = {0};

	sp6_entry_open_short_mode();
	sp6_open_ttl_signal();

	_msleep(100);

	for (devChannel = 0; devChannel < 2; devChannel++)
	{
		char notifyMsg[128] = "";
		if (devChannel == 0)
			sprintf(notifyMsg, "++++++++++++++++开短路检测左侧通道++++++++++++++++\r\n");
		else
			sprintf(notifyMsg, "++++++++++++++++开短路检测右侧通道++++++++++++++++\r\n");

		client_notify_msg(0, notifyMsg);
		printf(notifyMsg);

		sp6_set_open_short_data(devChannel, 0);
		_msleep(100);

		// read open short
		unsigned short value = mcu_read_open_short_data(devChannel);

		printf("0 open short: %d.\n", value);

		int i = 0;
		for (i = 0; i < MAX_OPEN_SHORT_PIN_NUMS; i++)
		{
			// set pin
			unsigned int pinValue = 1 << i;
			sp6_set_open_short_data(devChannel, pinValue);
			_msleep(100);

			// read short data
			value = mcu_read_open_short_data(devChannel);
			float shortValue = value * 0.1;
			printf("pin: %2d. shortValue: %4.2f Status: ", i, shortValue);

			if (shortValue < p_monitor_cfg->open_short_check_open_value)
			{
				printf("Open\r\n");
				checkRet[devChannel] = checkRet[devChannel] + 1;
				open_short_singal_notify_to_pc(i, PIN_STATU_OPEN);
			}
			else if (shortValue > p_monitor_cfg->open_short_check_short_value)
			{
				printf("Short\r\n");
				checkRet[devChannel] = checkRet[devChannel] + 1;
				open_short_singal_notify_to_pc(i, PIN_STATU_SHORT);
			}
			else
			{
				open_short_singal_notify_to_pc(i, PIN_STATU_OK);
				printf("Normal\r\n");
			}
		}

		// close singal pin.
		sp6_set_open_short_data(devChannel, 0);
		_msleep(100);

		// check IO
		reset_short_gpio(devChannel);
		_msleep(100);
		//
		value = mcu_read_open_short_data(devChannel);
		_msleep(100);

		float shortValue = value * 0.1;
		printf("all gpio volta: %4.2f\r\n", shortValue);

		if (devChannel == TTL_LVDS_CHANNEL_LEFT)
		{
			// gpio 1
			checkRet[devChannel] += check_gpio_short(p_monitor_cfg, devChannel, GPIO_PIN_UD_L);

			// gpio 2
			checkRet[devChannel] += check_gpio_short(p_monitor_cfg, devChannel, GPIO_PIN_LR_L);

			// gpio 3
			checkRet[devChannel] += check_gpio_short(p_monitor_cfg, devChannel, GPIO_PIN_MODE_L);

			// gpio 4
			checkRet[devChannel] += check_gpio_short(p_monitor_cfg, devChannel, GPIO_PIN_STBY_L);

			// gpio 5
			checkRet[devChannel] += check_gpio_short(p_monitor_cfg, devChannel, GPIO_PIN_RST_L);
		}
		else
		{
			// gpio 1
			checkRet[devChannel] += check_gpio_short(p_monitor_cfg, devChannel, GPIO_PIN_UD_R);

			// gpio 2
			checkRet[devChannel] += check_gpio_short(p_monitor_cfg, devChannel, GPIO_PIN_LR_R);

			// gpio 3
			checkRet[devChannel] += check_gpio_short(p_monitor_cfg, devChannel, GPIO_PIN_MODE_R);

			// gpio 4
			checkRet[devChannel] += check_gpio_short(p_monitor_cfg, devChannel, GPIO_PIN_STBY_R);

			// gpio 5
			checkRet[devChannel] += check_gpio_short(p_monitor_cfg, devChannel, GPIO_PIN_RST_R);
		}

		if (checkRet[devChannel] != 0)
		{
			if (devChannel == 0)
				sprintf(notifyMsg, "\r\n*警告：开短路检测左侧通道未通过\r\n");
			else
				sprintf(notifyMsg, "\r\n*警告：开短路检测右侧通道未通过\r\n");

			client_notify_msg(0, notifyMsg);
			printf(notifyMsg);
		}

		sprintf(notifyMsg, "\r\n++++++++++++++++++++++++++++++++++++++++++++++++++\r\n");

		client_notify_msg(0, notifyMsg);
		printf(notifyMsg);
	}

	_msleep(200);
	sp6_close_ttl_signal();
	sp6_leave_open_short_mode();

	if (checkRet[0] == 0 && checkRet[1] == 0)
		return PWR_CHANNEL_ALL;

	if (checkRet[0] == 0)
		return PWR_CHANNEL_LEFT;

	if (checkRet[1] == 0)
		return PWR_CHANNEL_RIGHT;

	return -1;
}


