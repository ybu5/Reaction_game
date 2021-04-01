/*
 * uart.c
 *
 *  Created on: Oct 24, 2019
 *      Author: yanmi
 */

#include "uart.h"
#include "msp.h"

#define BR_DIV 13
#define FS_MOD 1

void uartInit(void)
{



    EUSCI_A0->CTLW0 |= EUSCI_A_CTLW0_SWRST; //place EUSCIA0 in reset for configuration

    EUSCI_A0->CTLW0 = EUSCI_A_CTLW0_SSEL__SMCLK | EUSCI_A_CTLW0_SWRST; //select SMCLK as a source while holding the reset

    //set bit rate divisor for 16x over-sampling mode with first stage modulation
    EUSCI_A0->BRW = BR_DIV;
    EUSCI_A0->MCTLW = EUSCI_A_MCTLW_OS16 | (FS_MOD << EUSCI_A_MCTLW_BRF_OFS);

    EUSCI_A0->CTLW0 &= ~EUSCI_A_CTLW0_SWRST; //disable the reset
}

void uartPutC(char data)
{

    while (!(EUSCI_A0->IFG & EUSCI_A_IFG_TXIFG))
    {
    } //wait for space in the transmit buffer

    //putting a character into the transmit buffer
    EUSCI_A0->TXBUF = data;
}

int uartGetC(void)
{
    int data;
    while (!(EUSCI_A0->IFG & EUSCI_A_IFG_RXIFG))
    {
    } //wait for byte in receive buffer



    data=EUSCI_A0->RXBUF;
    return data;
}

void uartPut_STR(char *str)
{
    while (*str)
    {
        uartPutC(*str++);//increment to next char in the string
    }
}
