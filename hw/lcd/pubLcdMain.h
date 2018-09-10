#ifndef LCDMAIN_H_
#define LCDMAIN_H_

#include "comStruct.h"


INT initLcd(void);

unsigned int lcd_message_queue_get();

int lcd_autoFlickStart(int channel, short maxVcom);
int lcd_autoFlickWork(int channel, short index);
int lcd_autoFlickEnd(int channel);

int lcd_deviceAdd(char* devname);
int lcd_deviceRemove(char* devname);

int lcd_getListVersion();
int lcd_getList(int* channel_list);
int lcd_getStatus(int channel);


#endif

