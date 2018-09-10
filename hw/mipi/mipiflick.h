/*
 * mipiflick.h
 *
 *  Created on: 2018-7-24
 *      Author: tys
 */

#ifndef MIPIFLICK_H_
#define MIPIFLICK_H_

#ifdef __cplusplus
extern "C" {
#endif

int mipi_channel_get_vcom_and_flick(int mipi_channel, int *p_vcom, int *p_flick);

void show_lcd_msg(int mipi_channel, int show_vcom, int show_ok, int vcom, int flick, int ok1, int ok2);
#ifdef __cplusplus
}
#endif
#endif /* MIPIFLICK_H_ */
