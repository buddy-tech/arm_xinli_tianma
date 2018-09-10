/*
 * clipg.h
 *
 *  Created on: 2018-7-24
 *      Author: tys
 */

#ifndef CLIPG_H_
#define CLIPG_H_

#include "vcom.h"
#include "comStruct.h"
#include "monitor_config.h"
#include "client.h"

#ifdef __cplusplus
extern "C" {
#endif

int client_pg_shutON(int enable, char *pModelName, int channel, int ptn_id);

power_cfg_info_t* get_current_power_cfg();

monitor_cfg_info_t* get_current_monitor_cfg();

int get_module_res(int* p_width, int* p_height);

void client_pg_setTime(int stampTime);

void client_pg_showPtn(int ptnId);

int client_pg_pwr_status_get();

void  client_pg_pwr_status_set(int enable);

int load_cur_module();

int  client_pg_ptnId_get();

vcom_info_t* get_current_vcom_info();

int load_current_gamma_info(gamma_cfg_t* p_gamma_cfg);

gamma_cfg_t* get_current_gamma_cfg();

void show_signal_type_info(int signal_type);

void client_pg_syncFile(void *param);

void client_pg_downFile(void *param);

void client_pg_showCrossCur(pointer_xy_t *pPointer_xy,int num);

void client_pg_hideCrossCur(void);
void client_pg_upgradeFile(char *pSrcFile,char *pFileType,char *pDstFile);
void client_pg_firmware(void *param);

#ifdef __cplusplus
}
#endif

#endif /* CLIPG_H_ */
