/*
 * pwrSerial.h
 *
 *  Created on: 2018-7-21
 *      Author: tys
 */

#ifndef PWRSERIAL_H_
#define PWRSERIAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/* It is used to process the receiving data from power model by serial device */
typedef void (*_pwrRcvProcess)(void* pData, unsigned int length);

/*It is used to initialize the serial device of the power model */
void pwrSerialInit(_pwrRcvProcess process);

/*It is used to free the serial device of the power model */
void pwrSerialDeinit();

/*It is used to send data to the power model by the serial device */
int pwrSnd2Serial(void* pData, unsigned int length);

#ifdef __cplusplus
}
#endif

#endif /* PWRSERIAL_H_ */
