#ifndef _CA310_H_
#define _CA310_H_

#define PROBE_STATU_OK				(0)
#define PROBE_STATU_TIMEOUT			(-100)
#define PROBE_STATU_OTHER_ERROR		(-200)
#define PROBE_STATU_COMMAND_ERROR	(-10)
#define PROBE_STATU_OUT_RANGE		(-50)
#define PROBE_STATU_LOW_LUMI		(-52)

#define FLOAT_INVALID_FLICK_VALUE	(999.9999)

#define INVALID_FLICK_VALUE			(99999.99)

int uart_open(char *p_dev_name);

int uart_close(int uart_fd);

int uart_set_parity(int uart_fd, int databits, int stopbits, int parity);

int uart_set_speed(int uart_fd, int speed);

int uart_read(int uart_fd, struct timeval *p_timeout, unsigned char *pOutBuf);

int uart_write(int uart_fd, unsigned char *p_data, int data_len);

int uart_read_end(int uart_fd, char* p_read_buf, int *p_read_len);

int ca310_test(char* p_dev);

int ca310_reset(int lcd_channel);

int ca310_open(int lcd_channel, char* p_dev_name);

int ca310_close(int lcd_channel);

int ca310_start_flick_mode(int lcd_channel);

int ca310_stop_flick_mode(int lcd_channel);

int ca310_capture_flick_data(int lcd_channel, float *p_f_flick);

int ca310_start_xylv_mode(int lcd_channel);

int ca310_stop_xylv_mode(int lcd_channel);

int ca310_capture_xylv_data(int lcd_channel, float *p_f_x, float *p_f_y, float *p_f_lv);

#endif

