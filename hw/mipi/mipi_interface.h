/*
 * mipi_interface.h
 *
 *  Created on: 2018-7-24
 *      Author: tys
 */

#ifndef MIPI_INTERFACE_H_
#define MIPI_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

void WriteSSD2828(int spiNo, char addr, unsigned short data);

void WriteCMDSSD2828(int spiNo, unsigned char command);

void WriteDATASSD2828(int spiNo, unsigned short data);

unsigned short ReadSSD2828Reg(int spiNo, char addr);

unsigned short ReadDATASSD2828Reg(int spiNo, char addr);

void WriteFPGATo2828_8bit(int spiNo,char addr,unsigned short data);

void WriteCMDFPGATo2828_8bit(int spiNo, char command);

void WriteDATAFPGATo2828_8bit(int spiNo,unsigned short data);

void SSD2828_read_reg(int spiNo, char reg);

void SSD2828_write_reg(int spiNo, char reg, unsigned short data);

void mtp_power_on(int channel);

void mtp_power_off(int channel);

#ifdef __cplusplus
}
#endif
#endif /* MIPI_INTERFACE_H_ */
