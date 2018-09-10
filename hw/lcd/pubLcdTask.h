/*
 * pubLcdTask.h
 *
 *  Created on: 2018-7-24
 *      Author: tys
 */

#ifndef PUBLCDTASK_H_
#define PUBLCDTASK_H_
#ifdef __cplusplus
extern "C" {
#endif

int lcd_deviceRemove(char* devname);

int lcd_deviceAdd(char* devname);

int lcd_getStatus(int channel);

int lcd_getList(int* channel_list);

int lcd_getListVersion();

int initLcd(void);

unsigned int lcd_message_queue_get();
#ifdef __cplusplus
}
#endif

#endif /* PUBLCDTASK_H_ */
