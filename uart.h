/*
 * uart.h
 *
 *  Created on: Oct 24, 2019
 *      Author: yanmi
 */
#include "msp.h"

#define NO_DATA -1

#ifndef UART_H_
#define UART_H_

void uartInit(void);

void uartPutC(char data);

int uartGetC(void);

void uartPut_STR(char *str);



#endif /* UART_H_ */
