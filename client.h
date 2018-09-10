#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "pgos/MsOS.h"
#include <pthread.h>
#include "packsocket/packsocket.h"
#include "loop/loop.h"
#include "mypath.h"
#include "vcom.h"
#include "comStruct.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct tag_pointer_xy_s
{
    int x;
    int y;
}pointer_xy_t;


typedef enum
{
    SYNC_FILE_ERROR,
}PG_ERROR_INFO_E;

enum {
	E_ERROR_OK = 0,
	E_ERROR_OTP_TIMES_END = -1,
	E_ERROR_OTP_TIMES_NOT_CHANGE = -2,
	E_ERROR_OTP_DATA_WRITE_FAILED = -3,
};

void show_version();

void client_register(char *pSoftVer,char *pHardVer,char *pgCode);

void client_register_ex(unsigned int fpga_version, unsigned int box_version, unsigned int power_version, char *pgCode);

int client_getFileFromSet(void *infoSet);

void client_getFile(char *pFileName, void *infoSet);

void client_syncStatu();

void client_syncFinish();

void client_sendEdid();

void client_sendVcom(int channel,int vcom, int otp,int flick);

void client_sendReg(int regAddr,int regValue);

void client_sendRegStatus(int regAddr,int regstatu);

void client_updateStatus();

void client_rebackHeart();

void client_rebackPower(int powerOn);

void client_sendOtp(int channel, int otp_result, int max_otp_times);

void client_send_id_Otp_end(int channel, int result, int max_otp_times);

void client_end_rgbw(int channel, int rgbw, double x, double y, double Lv);

void client_end_gamma_write(int channel, int write_type);

void client_end_read_gamma(int channel, int len, unsigned char* data, int otpp, int otpn, int read_type);

void client_end_gamma_otp(int channel, char* msg);

void client_end_gamma_otp_info(int channel, int error_no, int vcom, int flick,
		double f_x, double f_y, double f_lv);

void client_end_realtime_control(int channel, int len, unsigned char* data);

void client_end_firmware_upgrade(int upgrade_type, int upgrade_state);

void client_sendPower(int channel, sByPtnPwrInfo *pwrInfo);

void client_sendReg(int regAddr,int regValue);

void client_sendRegStatus(int regAddr,int regStatu);

void client_notify_msg(int msg_flag, char* p_msg);

void client_rebackError(int errorNo);

void client_rebackMipiCode(char *pMipiCode,int len);

void client_rebackFlick(char flick,char polar);

void client_rebackautoFlick(int channel,int index,int autoflick);

void client_rebackFlickOver(int channel, int vcom, int flick);

void client_sendPowerVersion(unsigned int channel, unsigned int version);

void client_sendBoxVersion(unsigned int channel, unsigned int version);

void client_lcd_read_reg_ack(int channel, int len, unsigned char* data);

void client_flick_test_ack(int channel, int vcom, double flick);

void client_power_test_read_open_short_mode_ack(unsigned char mode);

void client_power_test_read_vcom_volt_ack(short vcom_volt);

void client_power_test_read_vcom_status_ack(short vcom_status);

void client_power_test_read_otp_volt_ack(unsigned short otp_volt);

void client_power_test_read_otp_status_ack(short otp_status);

void client_power_test_read_open_short_ack(unsigned short open_short);

void client_power_test_read_ntc_ag_ack(unsigned short ntc_ag);

void client_gpio_get_state_ack(unsigned char index, unsigned char pin, unsigned char dir, unsigned char value);

unsigned int get_server_ip();

void set_server_ip(unsigned int new_ip);

void close_all_socket();

void client_init(int multicast_port);

void client_destory();

/**************pg local****************************/
typedef struct tag_dwn_file_info_s
{
    char srcFileName[256];
    char srcFileType[16];
    char dstFileName[256];
}dwn_file_info_t;

typedef struct tag_recv_file_info_s
{
    char  rfiName[256];
    int   rfiSize;
    int   rfiTime;
    unsigned char  *pFileData;
    int   actRecvSize;
}recv_file_info_t;

typedef void (*Fn_recv_file_t)(const void *);
typedef recv_file_info_t* (*Fn_get_cur_file_info_t)(void *);
typedef void (*Fn_add_file_index_t)(void *);

typedef struct tag_recv_file_info_set_s
{
    int						curNo;
    int						totalFilesNums;
    recv_file_info_t		*pRecv_file_info;
    Fn_recv_file_t			fn_recv_file;
    void					*param;
    Fn_get_cur_file_info_t	fn_get_cur_file_info;
    Fn_add_file_index_t     fn_add_file_index;
}recv_file_info_set_t;

/**********************************************/
#define LISTENED_SERVER_EVENT  0x1000
#define LOST_SERVER_EVENT      0x1001

typedef struct tag_cliservice_s
{
    unsigned short local_port;
    unsigned short server_port;
    char       server_ip[32];
    int        srv_timeout;
    int        srv_working;
    int        mutil_working;
    int        guard_working;
    int        listen_socket;
    int        client_socket;
    int        send_fd;
    int        muticast_fd;
    pthread_t  thread_guard;
    pthread_t  thread_muticast;
    pthread_t  thread_server;
	pthread_t  thread_connect;
	pthread_t  thread_network;
	
	int is_network;
	
    pthread_mutex_t m;
	int 			link_on;	// 0: link down; 1: link on;
	int        		time_alive_timeout;
	
    MS_EVENT_HANDLER_LIST *pEventHanderList;
    socket_cmd_class_t     socketCmdClass;

	// debug mode
	int multicast_port;
}cliService_t;

#define SERVER_CLIENT_PORT  (8013)  //send to server port
#define CLIENT_SERVER_PORT  (8014)  //recv from server port

#define MCAST_PORT 			(8888)
#define MCAST_ADDR 			"224.0.0.88"

int msgProc(socket_cmd_t *pSocketCmd);
void client_init();
void client_destory();

void recv_file_list_init();
void recv_file_list_insert(void *pData);
void *recv_file_list_get(void *pData);
void recv_file_list_del(void *pData);

cliService_t* get_client_service_data();

#define thread_pool  1

#ifdef __cplusplus
}
#endif

#endif
