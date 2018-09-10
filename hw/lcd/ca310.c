#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <io.h>
#include "common.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <termios.h>
#include <unistd.h>

#include "ca310.h"

enum {
	E_CA310_WORK_MODE_IDLE = 0,
	E_CA310_WORK_MODE_XYLV,
	E_CA310_WORK_MODE_FLICK,
};

#define MAX_LCD_PROBE_NUMS			(2)

#define MAX_CA310_DEV_NAME_LEN		(32)
typedef struct tag_ca310_probe_info
{
	int	lcd_channel;
	int uart_fd;
	char dev_name[MAX_CA310_DEV_NAME_LEN];
	int work_mode;
}ca310_probe_info_t;

static ca310_probe_info_t s_ca310_probe[MAX_LCD_PROBE_NUMS] = { 0 };

static int s_uart_speed_arr[] =
{
	B38400, B19200, B9600, B4800, B2400, B1200, B300, B115200, B19200, B9600,
	B4800, B2400, B1200, B300,
};

static int s_uart_speed_name_arr[] =
{
	38400, 19200, 9600, 4800, 2400, 1200, 300, 115200,
	19200, 9600, 4800, 2400, 1200, 300,
};


int uart_open(char *p_dev_name)
{
  int fd_uart = -1;
  fd_uart = open(p_dev_name, O_RDWR | O_NDELAY | O_NOCTTY);
  if (-1 == fd_uart)
	{
		printf("uart_open error: Can't Open Serial Port %s\n", p_dev_name);
		return -1;
	}
	
	return fd_uart;
}

int uart_close(int uart_fd)
{
	close(uart_fd);
	return 0;
}

int uart_set_parity(int uart_fd, int databits, int stopbits, int parity)
{
	struct termios options = { 0 };

	if (tcgetattr(uart_fd, &options) != 0)
	{
		printf("uart_set_parity error: tcgetattr error!\n");
		return -1;
	}

	options.c_cflag &= ~CSIZE;

	switch (databits)
	{
		case 7:
			options.c_cflag |= CS7;
			break;
			
		case 8:
			options.c_cflag |= CS8;
			break;
			
		default:
			printf("uart_set_parity error: Unsupported data size: %d. \n", databits);
			return (-1);
	}

	switch (parity)
	{
		case 'n':
		case 'N':
			options.c_cflag &= ~PARENB;	/* Clear parity enable */
			options.c_iflag &= ~INPCK;	/* Enable parity checking */
			break;
			
		case 'o':
		case 'O':
			options.c_cflag |= (PARODD | PARENB);	/* 设置为奇效验 */
			options.c_iflag |= INPCK;	/* Disnable parity checking */
			break;
			
		case 'e':
		case 'E':
			options.c_cflag |= PARENB;	/* Enable parity */
			options.c_cflag &= ~PARODD;	/* 转换为偶效验 */
			options.c_iflag |= INPCK;	/* Disnable parity checking */
			break;
			
		case 'S':
		case 's':				/*as no parity */
			options.c_cflag &= ~PARENB;
			options.c_cflag &= ~CSTOPB;
			break;
			
		default:
			printf("uart_set_parity error: Unsupported parity: %d.\n", parity);
			return (-1);
	}

	/* 设置停止位 */
	switch (stopbits)
	{
		case 1:
			options.c_cflag &= ~CSTOPB;
			break;
			
		case 2:
			options.c_cflag |= CSTOPB;
			break;
			
		default:
			printf("uart_set_parity error: Unsupported stop bits: %d.n", stopbits);
			return (-1);
	}

	/* Set input parity option */
	if (parity != 'n')
	{ 
		options.c_iflag |= INPCK; 
	}

	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG | INLCR | ICRNL | IEXTEN);
	//Choosing   Raw   Input
	//options.c_oflag   &=~(OCRNL|ONLCR|ONLRET);
	options.c_cflag |= (CLOCAL | CREAD);
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	options.c_oflag &= ~OPOST;
	options.c_oflag &= ~(ONLCR | OCRNL);	//添加的
	options.c_iflag &= ~(ICRNL | INLCR);
	options.c_iflag &= ~(IXON | IXOFF | IXANY);	//添加的
	
	tcflush(uart_fd, TCIOFLUSH);
	options.c_cflag &= ~CRTSCTS;
	options.c_cc[VTIME] = 0;	/* 设置超时15 seconds */
	options.c_cc[VMIN] = 0;		/* Update the options and do it NOW */

	if (tcsetattr(uart_fd, TCSANOW, &options) != 0)
	{
		printf("uart_set_parity error: tcsetattr error!\n");
		return (-1);
	}

	return (0);
}
 
int uart_set_speed(int uart_fd, int speed)
{
	int i = 0;
	int status = 0;
	struct termios Opt = { 0 };
	tcgetattr(uart_fd, &Opt);

	for (i = 0; i < sizeof(s_uart_speed_arr) / sizeof(int); i++)
	{
		if (speed == s_uart_speed_name_arr[i])
		{
			tcflush(uart_fd, TCIOFLUSH);
			cfsetispeed(&Opt, s_uart_speed_arr[i]);
			cfsetospeed(&Opt, s_uart_speed_arr[i]);
			status = tcsetattr(uart_fd, TCSANOW, &Opt);

			if (status != 0)
			{
				printf("uart_set_speed error: tcsetattr failed!\n");
				return -1;
			}

			tcflush(uart_fd, TCIOFLUSH);
			break;
		}
	}
	
	return 0;
}

int uart_read(int uart_fd, struct timeval *p_timeout, unsigned char *pOutBuf)
{
	int ret = -1;
	int maxFd = 0;
	fd_set readFds;
	
	#define MAX_BUFF_SIZE 2048
	unsigned char buf[MAX_BUFF_SIZE] = { 0 };

	/* 判断输出buff是否为NULL */
	if (pOutBuf == NULL)
	{
		printf("uart_read Error: Output buff is NULL!\r\n");
		return -3;
	}

    //printf("uart fd is %d\n",gsUartInfo[serial].fd);
	FD_ZERO(&readFds);
	FD_SET(uart_fd, &readFds);

	ret = select(uart_fd + 1, &readFds, NULL, NULL, p_timeout);
	if (ret > 0)
	{
		if (FD_ISSET(uart_fd, &readFds))
		{
			ret = read(uart_fd, buf, MAX_BUFF_SIZE);
			if (ret > 0)
				memcpy(pOutBuf, buf, ret);
		}
	}

	return ret;
}

int uart_write(int uart_fd, unsigned char *p_data, int data_len)
{
	int slen = write(uart_fd, p_data, data_len);
	if(slen <= 0)
	{
		printf("\n write data error: ret = %d.\n", slen);
		return 0;
	}

	return slen;
}

int uart_read_end(int uart_fd, char* p_read_buf, int *p_read_len)
{
	int ret = 0;
	unsigned char buf[1024] = { 0 };
	int offset = 0;
		
	struct timeval time_out = { 0 };
	time_out.tv_sec = 2;
	time_out.tv_usec = 0;
	
	while (ret >= 0)
	{
		ret = uart_read(uart_fd, &time_out, buf + offset);
		if (ret > 0)
		{
			//printf("uart_read: %d. \n", ret);
			//printf("%s", buf);
			offset += ret;

			if (buf[offset -1] == '\r')
			{
				break;
			}
			
			continue;
		}
		else
		{
			break;
		}
	}
	
	if (offset > 0)
	{
		memcpy(p_read_buf, buf, offset);
	}

	*p_read_len = offset;
	
	return offset;
}

int ca310_test(char* p_dev)
{
	#define SYS_CMD_LEN			(8)
	char cmd[] = "COM,0\n";
	
	int uart_fd = -1;
	int ret = -1;
	
	printf("ca310_test: %s\n", p_dev);
	
	// open uart
	uart_fd = uart_open(p_dev);
	if (uart_fd > 0)
	{
		// ca310
		ret = uart_set_parity(uart_fd, 7, 2, 'E');		
		ret = uart_set_speed(uart_fd, 38400);
		
		ret = uart_write(uart_fd, cmd, strlen(cmd));
		printf("Write len = %d.\ncmd: %s", ret, cmd); 
		
		unsigned char buf[1024] = "";
		int buf_len = 1024;		
		ret = uart_read_end(uart_fd, buf, &buf_len);		
		printf("Read: %s\n", buf);
		
		// close uart
		uart_close(uart_fd);
	}
	return 0;
}

int ca310_reset(int lcd_channel)
{
	if ( (lcd_channel < 0) || (lcd_channel > MAX_LCD_PROBE_NUMS) )
	{
		printf("ca310_reset error: invalid lcd_channel: %d.\n", lcd_channel);
		return -1;
	}
	
	//ca310_close(lcd_channel);
	memset(s_ca310_probe + lcd_channel, 0, sizeof(ca310_probe_info_t));
	s_ca310_probe[lcd_channel].uart_fd = -1;
	return 0;
}

int ca310_open(int lcd_channel, char* p_dev_name)
{
	if ( (lcd_channel < 0) || (lcd_channel > MAX_LCD_PROBE_NUMS) )
	{
		printf("ca310_open error: invalid lcd_channel: %d.\n", lcd_channel);
		return -1;
	}

	printf("== ca310_open: lcd channel: %d, dev: %s.\n", lcd_channel, p_dev_name);
	s_ca310_probe[lcd_channel].uart_fd = uart_open(p_dev_name);
	strcpy(s_ca310_probe[lcd_channel].dev_name, p_dev_name);
	s_ca310_probe[lcd_channel].lcd_channel = lcd_channel;
	s_ca310_probe[lcd_channel].work_mode = E_CA310_WORK_MODE_IDLE;
	return 0;
}

int ca310_close(int lcd_channel)
{
	if ( (lcd_channel < 0) || (lcd_channel > MAX_LCD_PROBE_NUMS) )
	{
		printf("ca310_close error: invalid lcd_channel: %d.\n", lcd_channel);
		return -1;
	}
	
	if (s_ca310_probe[lcd_channel].uart_fd > 0)
	{
		uart_close(s_ca310_probe[lcd_channel].uart_fd);

		s_ca310_probe[lcd_channel].uart_fd = -1;
		s_ca310_probe[lcd_channel].work_mode = E_CA310_WORK_MODE_IDLE;
	}
	
	return 0;
}

#define CMD_OK_KEY			"OK00"
#define CMD_OK_01_KEY		"OK0"

#define FMT_FLICK_DATA		"OK00,P%d  %f"

#define FMT_XYLV_DATA		"OK%d,P%d %d;%d; %lf"
#define FMT_XYLV1_DATA		"OK%*d,P%d %d;%d; %lf"


// error
#define ERR_INVALID_COMMAND	"ER10"
#define ERR_VALUE_OUT		"ER50"
#define ERR_LOW_LUMI		"ER52"

char str_cmd_mode_remote_on[] = "COM,2\n";
char str_cmd_mode_remote_off[] = "COM,0\n";
char str_cmd_sync[] = "SCS,3\n";
char str_cmd_mds_flick[] = "MDS,6\n";
char str_cmd_mds_xylv[] = "MDS,0\n";

char str_cmd_meas[] = "MES\n";

int ca310_start_flick_mode(int lcd_channel)
{
	int ret = 0;
	char buf[32] = "";
	int recv_len = 0;
	
	if ( (lcd_channel < 0) || (lcd_channel > MAX_LCD_PROBE_NUMS) )
	{
		printf("ca310_start_flick_mode error: invalid lcd_channel: %d.\n", lcd_channel);
		return PROBE_STATU_OTHER_ERROR;
	}

	// COM,2
	ret = uart_write(s_ca310_probe[lcd_channel].uart_fd, str_cmd_mode_remote_on,
					strlen(str_cmd_mode_remote_on));

	ret = uart_read_end(s_ca310_probe[lcd_channel].uart_fd, buf, &recv_len);
	if (ret > 0)
	{
		// check error code.
		if (strncmp(CMD_OK_KEY, buf, strlen(CMD_OK_KEY)) != 0)
		{
			printf("ca310_start_flick_mode error: remote on => %s.\n", buf);
			return PROBE_STATU_OTHER_ERROR;
		}
	}
	else
	{
		// timeout.
		printf("ca310_start_flick_mode: remote on => timeout!\n", buf);
		return PROBE_STATU_TIMEOUT;
	}

	// SCS,3
	ret = uart_write(s_ca310_probe[lcd_channel].uart_fd, str_cmd_sync, strlen(str_cmd_sync));
	ret = uart_read_end(s_ca310_probe[lcd_channel].uart_fd, buf, &recv_len);
	if (ret > 0)
	{
		// check error code.
		if (strncmp(CMD_OK_KEY, buf, strlen(CMD_OK_KEY)) != 0)
		{
			printf("ca310_start_flick_mode error: sync => %s.\n", buf);
			return PROBE_STATU_OTHER_ERROR;
		}
	}
	else
	{
		// timeout.
		printf("ca310_start_flick_mode:  sync => timeout!\n", buf);
		return PROBE_STATU_TIMEOUT;
	}

	// MDS,6
	ret = uart_write(s_ca310_probe[lcd_channel].uart_fd, str_cmd_mds_flick,
					strlen(str_cmd_mds_flick));

	ret = uart_read_end(s_ca310_probe[lcd_channel].uart_fd, buf, &recv_len);
	if (ret > 0)
	{
		// check error code.
		if (strncmp(CMD_OK_KEY, buf, strlen(CMD_OK_KEY)) != 0)
		{
			printf("ca310_start_flick_mode error: mds: flick => %s.\n", buf);
			return PROBE_STATU_OTHER_ERROR;
		}
	}
	else
	{
		// timeout.
		printf("ca310_start_flick_mode:  mds: flick => timeout!\n", buf);
		return PROBE_STATU_TIMEOUT;
	}

	s_ca310_probe[lcd_channel].work_mode = E_CA310_WORK_MODE_FLICK;
	
	return PROBE_STATU_OK;
}

int ca310_stop_flick_mode(int lcd_channel)
{
	int ret = 0;
	char buf[32] = "";
	int recv_len = 0;

	if ( (lcd_channel < 0) || (lcd_channel > MAX_LCD_PROBE_NUMS) )
	{
		printf("ca310_stop_flick_mode error: invalid lcd_channel: %d.\n", lcd_channel);
		return -1;
	}
	
	// COM,0
	ret = uart_write(s_ca310_probe[lcd_channel].uart_fd, str_cmd_mode_remote_off,
					strlen(str_cmd_mode_remote_off));

	ret = uart_read_end(s_ca310_probe[lcd_channel].uart_fd, buf, &recv_len);
	if (ret > 0)
	{
		// check error code.
		if (strncmp(CMD_OK_KEY, buf, strlen(CMD_OK_KEY)) != 0)
		{
			printf("ca310_stop_flick_mode error: remote off => %s.\n", buf);
			return -1;
		}
	}
	else
	{
		// timeout.
		printf("ca310_stop_flick_mode: remote off => timeout!\n", buf);
		return -1;
	}

	s_ca310_probe[lcd_channel].work_mode = E_CA310_WORK_MODE_IDLE;
	
	return PROBE_STATU_OK;
}

int ca310_capture_flick_data(int lcd_channel, float *p_f_flick)
{
	int ret = 0;
	char buf[32] = "";
	int recv_len = 0;

	if (p_f_flick == NULL)
	{
		printf("ca310_capture_flick_data error: invalid param: p_f_flick = NULL.\n");
		return PROBE_STATU_OTHER_ERROR;
	}

	if ( (lcd_channel < 0) || (lcd_channel > MAX_LCD_PROBE_NUMS) )
	{
		printf("ca310_capture_flick_data error: invalid lcd_channel: %d.\n", lcd_channel);
		return PROBE_STATU_OTHER_ERROR;
	}

	*p_f_flick = FLOAT_INVALID_FLICK_VALUE;
	
	// MES
	ret = uart_write(s_ca310_probe[lcd_channel].uart_fd, str_cmd_meas, strlen(str_cmd_meas));
	ret = uart_read_end(s_ca310_probe[lcd_channel].uart_fd, buf, &recv_len);
	if (ret > 0)
	{
		// check error code.
		if (strncmp(CMD_OK_KEY, buf, strlen(CMD_OK_KEY)) != 0)
		{
			if (strncmp(ERR_INVALID_COMMAND, buf, strlen(CMD_OK_KEY)) == 0)
			{
				printf("ca310_stop_flick_mode error: command error! meas => %s.\n", buf);
				return PROBE_STATU_COMMAND_ERROR;
			}

			if (strncmp(ERR_VALUE_OUT, buf, strlen(ERR_VALUE_OUT)) == 0)
			{
				printf("ca310_stop_flick_mode error: out range! meas => %s.\n", buf);
				return PROBE_STATU_OUT_RANGE;
			}

			if (strncmp(ERR_LOW_LUMI, buf, strlen(ERR_LOW_LUMI)) == 0)
			{
				printf("ca310_stop_flick_mode error: low lumi! meas => %s.\n", buf);
				return PROBE_STATU_LOW_LUMI;
			}
			else
			{
				printf("ca310_stop_flick_mode error: meas => %s.\n", buf);
			}
			
			return PROBE_STATU_OTHER_ERROR;
		}
		else
		{
			int porbe_channel = 0;			
			sscanf(buf, FMT_FLICK_DATA, &porbe_channel, p_f_flick);
			printf("capture flick data: %f.\n", *p_f_flick);
		}
	}
	else
	{
		// timeout.
		printf("ca310_stop_flick_mode: meas => timeout!\n", buf);
		return PROBE_STATU_TIMEOUT;
	}

	//OK00,P1  10.6
	return PROBE_STATU_OK;
}

int ca310_start_xylv_mode(int lcd_channel)
{
	int ret = 0;
	char buf[32] = "";
	int recv_len = 0;
	
	if ( (lcd_channel < 0) || (lcd_channel > MAX_LCD_PROBE_NUMS) )
	{
		printf("ca310_start_xylv_mode error: invalid lcd_channel: %d.\n", lcd_channel);
		return PROBE_STATU_OTHER_ERROR;
	}

	// COM,2
	ret = uart_write(s_ca310_probe[lcd_channel].uart_fd, str_cmd_mode_remote_on,
					strlen(str_cmd_mode_remote_on));

	ret = uart_read_end(s_ca310_probe[lcd_channel].uart_fd, buf, &recv_len);
	if (ret > 0)
	{
		// check error code.
		if (strncmp(CMD_OK_KEY, buf, strlen(CMD_OK_KEY)) != 0)
		{
			printf("ca310_start_xylv_mode error: remote on => %s.\n", buf);
			return PROBE_STATU_COMMAND_ERROR;
		}
	}
	else
	{
		// timeout.
		printf("ca310_start_xylv_mode: remote on => timeout!\n", buf);
		return PROBE_STATU_TIMEOUT;
	}

	// SCS,3
	ret = uart_write(s_ca310_probe[lcd_channel].uart_fd, str_cmd_sync, strlen(str_cmd_sync));
	ret = uart_read_end(s_ca310_probe[lcd_channel].uart_fd, buf, &recv_len);
	if (ret > 0)
	{
		// check error code.
		if (strncmp(CMD_OK_KEY, buf, strlen(CMD_OK_KEY)) != 0)
		{
			printf("ca310_start_xylv_mode error: sync => %s.\n", buf);
			return PROBE_STATU_COMMAND_ERROR;
		}
	}
	else
	{
		// timeout.
		printf("ca310_start_xylv_mode:  sync => timeout!\n", buf);
		return PROBE_STATU_TIMEOUT;
	}

	// MDS,0
	ret = uart_write(s_ca310_probe[lcd_channel].uart_fd, str_cmd_mds_xylv,
					strlen(str_cmd_mds_xylv));

	ret = uart_read_end(s_ca310_probe[lcd_channel].uart_fd, buf, &recv_len);
	if (ret > 0)
	{
		// check error code.
		if (strncmp(CMD_OK_KEY, buf, strlen(CMD_OK_KEY)) != 0)
		{
			printf("ca310_start_xylv_mode error: mds: xylv => %s.\n", buf);
			return PROBE_STATU_COMMAND_ERROR;
		}
	}
	else
	{
		// timeout.
		printf("ca310_start_xylv_mode:  mds: xylv => timeout!\n", buf);
		return PROBE_STATU_TIMEOUT;
	}

	s_ca310_probe[lcd_channel].work_mode = E_CA310_WORK_MODE_XYLV;
	
	return PROBE_STATU_OK;
}

int ca310_stop_xylv_mode(int lcd_channel)
{
	int ret = 0;
	char buf[32] = "";
	int recv_len = 0;

	if ( (lcd_channel < 0) || (lcd_channel > MAX_LCD_PROBE_NUMS) )
	{
		printf("ca310_stop_xylv_mode error: invalid lcd_channel: %d.\n", lcd_channel);
		return PROBE_STATU_OTHER_ERROR;
	}
	
	// COM,0
	ret = uart_write(s_ca310_probe[lcd_channel].uart_fd, str_cmd_mode_remote_off,
					strlen(str_cmd_mode_remote_off));

	ret = uart_read_end(s_ca310_probe[lcd_channel].uart_fd, buf, &recv_len);
	if (ret > 0)
	{
		// check error code.
		if (strncmp(CMD_OK_KEY, buf, strlen(CMD_OK_KEY)) != 0)
		{
			printf("ca310_stop_xylv_mode error: remote off => %s.\n", buf);
			return PROBE_STATU_COMMAND_ERROR;
		}
	}
	else
	{
		// timeout.
		printf("ca310_stop_xylv_mode: remote off => timeout!\n", buf);
		return PROBE_STATU_TIMEOUT;
	}

	s_ca310_probe[lcd_channel].work_mode = E_CA310_WORK_MODE_XYLV;
	
	return PROBE_STATU_OK;
}

int ca310_capture_xylv_data(int lcd_channel, float *p_f_x, float *p_f_y, float *p_f_lv)
{
	int ret = 0;
	char buf[32] = "";
	int recv_len = 0;

	if ( (p_f_x == NULL) || (p_f_y == NULL) || (p_f_lv == NULL) )
	{
		printf("ca310_capture_xylv_data error: invalid param: p_f_x or p_f_y or p_f_lv = NULL.\n");
		return PROBE_STATU_OTHER_ERROR;
	}
	
	if ( (lcd_channel < 0) || (lcd_channel > MAX_LCD_PROBE_NUMS) )
	{
		printf("ca310_capture_xylv_data error: invalid lcd_channel: %d.\n", lcd_channel);
		return PROBE_STATU_OTHER_ERROR;
	}

	// MES
	ret = uart_write(s_ca310_probe[lcd_channel].uart_fd, str_cmd_meas, strlen(str_cmd_meas));
	ret = uart_read_end(s_ca310_probe[lcd_channel].uart_fd, buf, &recv_len);
	if (ret > 0)
	{
		// check error code.
		if ( strncmp(CMD_OK_KEY, buf, strlen(CMD_OK_KEY)) != 0 
				&& strncmp(CMD_OK_01_KEY, buf, strlen(CMD_OK_01_KEY)) != 0)
		{
			printf("ca310_capture_xylv_data error: meas => %s.\n", buf);

			if (strncmp(ERR_INVALID_COMMAND, buf, strlen(CMD_OK_KEY)) == 0)
			{
				printf("ca310_capture_xylv_data error: command error! meas => %s.\n", buf);
				return PROBE_STATU_COMMAND_ERROR;
			}

			if (strncmp(ERR_VALUE_OUT, buf, strlen(ERR_VALUE_OUT)) == 0)
			{
				printf("ca310_capture_xylv_data error: out range! meas => %s.\n", buf);
				return PROBE_STATU_OUT_RANGE;
			}

			if (strncmp(ERR_LOW_LUMI, buf, strlen(ERR_LOW_LUMI)) == 0)
			{
				printf("ca310_capture_xylv_data error: low lumi! meas => %s.\n", buf);
				return PROBE_STATU_LOW_LUMI;
			}
			else
			{		
				printf("ca310_capture_xylv_data other error: meas => %s.\n", buf);
			}
			
			return PROBE_STATU_OTHER_ERROR;
		}
		else
		{
			int porbe_channel = 0;	
			int x = 0;
			int y = 0;
			int err = 0;

			sscanf(buf, FMT_XYLV_DATA, &err, &porbe_channel, &x, &y, p_f_lv);
			//sscanf(buf, FMT_XYLV1_DATA, &porbe_channel, &x, &y, p_f_lv);
			if (x >= 1000)
				*p_f_x = x / 10000.00;
			else
				*p_f_x = x / 1000.00;

			if (y >= 1000)
				*p_f_y = y / 10000.00;
			else				
				*p_f_y = y / 1000.00;
		}
	}
	else
	{
		// timeout.
		printf("ca310_capture_xylv_data: meas => timeout!\n", buf);
		return PROBE_STATU_TIMEOUT;
	}
	
	return PROBE_STATU_OK;
}
