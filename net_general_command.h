#ifndef _NET_GENERAL_COMMAND_H_
#define _NET_GENERAL_COMMAND_H_

#include "packsocket.h"

enum {
	E_NET_GEN_CMD_READ_CABC_PWM_REQ = 1,
	E_NET_GEN_CMD_READ_CABC_PWM_ACK,

	E_NET_GEN_CMD_READ_DISPLAY_ID_NOTIFY,

	E_NET_GEN_CMD_SET_IP_INFO_REQ,
	E_NET_GEN_CMD_SET_MAC_INFO_REQ,

	E_NET_GEN_CMD_DO_FACOTRY_RESET_REQ,

	// Power calibration
	E_NET_GEN_CMD_POWER_CALIBRATION_REQ,
	E_NET_GEN_CMD_POWER_SETING_BURN_REQ,
	E_NET_GEN_CMD_NUMS
};

int client_send_gen_cmd_data(socket_cmd_class_t* p_socket_obj, int socket_fd, unsigned char* p_send_data, int send_len);

void client_send_gen_cmd_ack_read_cabc_end(socket_cmd_class_t* p_socket_obj, int socket_fd, int channel,
											unsigned short cabc_freq, unsigned short cabc_duty);

void client_send_gen_cmd_read_lcd_id_notify(socket_cmd_class_t* p_socket_obj, int socket_fd, int channel,
											unsigned char *p_id_data, int id_data_len, unsigned char otp_times);

int net_general_command_proc(socket_cmd_class_t* p_socket_obj, int socket_fd, char* p_json_command_data, int json_command_data_len);

int net_work_config_save(char* str_ip, char* str_mask, char* str_gateway);

int do_factory_reset();
#endif

