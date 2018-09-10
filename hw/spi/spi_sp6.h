#ifndef _MIPI_CMD_INTERFACE_H_
#define _MIPI_CMD_INTERFACE_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TTL_LVDS_NORMAL_MODE	(0x0)
#define TTL_LVDS_HS_VS_MODE	(0x1)
#define TTL_LVDS_DE_MODE			(0x2)

int sp6_write_8bit_data(unsigned char reg, unsigned char value);

unsigned char sp6_read_8bit_data(unsigned char reg);

int sp6_open_lvds_signal();

int sp6_close_lvds_signal();

int sp6_open_ttl_signal();

int sp6_open_ttl_buffer();

int sp6_close_ttl_signal();

void sp6_entry_open_short_mode();

void sp6_leave_open_short_mode();

unsigned char sp6_read_open_short_mode();

int sp6_set_clock_pol_mode(int is_on);

int sp6_set_open_short_data(TTL_LVDS_CHANNEL_E channel, unsigned int value);

int sp6_set_lcd_bit_and_vesa_jeida(int bit_mode, int is_vesa_mode);

#ifdef __cplusplus
}
#endif

#endif
