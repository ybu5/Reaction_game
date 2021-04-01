// Filename: lcd.c
// Description: Module to control the color 128x128 TFT LCD display on Educational BoosterPack MKII
// Author(s): Robert Trost
// Set: N/A
// Date: Feb 20, 2016 (created)

#include "msp.h"
#include "lcd.h"

// ST7735 LCD controller Command Set (copied from TI sample code)
#define CM_NOP             0x00
#define CM_SWRESET         0x01
#define CM_RDDID           0x04
#define CM_RDDST           0x09
#define CM_SLPIN           0x10
#define CM_SLPOUT          0x11
#define CM_PTLON           0x12
#define CM_NORON           0x13
#define CM_INVOFF          0x20
#define CM_INVON           0x21
#define CM_GAMSET          0x26
#define CM_DISPOFF         0x28
#define CM_DISPON          0x29
#define CM_CASET           0x2A
#define CM_RASET           0x2B
#define CM_RAMWR           0x2C
#define CM_RGBSET          0x2d
#define CM_RAMRD           0x2E
#define CM_PTLAR           0x30
#define CM_MADCTL          0x36
#define CM_COLMOD          0x3A
#define CM_SETPWCTR        0xB1
#define CM_SETDISPL        0xB2
#define CM_FRMCTR3         0xB3
#define CM_SETCYC          0xB4
#define CM_SETBGP          0xb5
#define CM_SETVCOM         0xB6
#define CM_SETSTBA         0xC0
#define CM_SETID           0xC3
#define CM_GETHID          0xd0
#define CM_SETGAMMA        0xE0
#define CM_MADCTL_MY       0x80
#define CM_MADCTL_MX       0x40
#define CM_MADCTL_MV       0x20
#define CM_MADCTL_ML       0x10
#define CM_MADCTL_BGR      0x08
#define CM_MADCTL_MH       0x04

// Correction factors for display offsets (deduced from TI sample code, but not completely sure why)
#define X_CORRECTION_OFFSET 2
#define Y_CORRECTION_OFFSET 1

//  LCD pins
#define LCD_SCK           BIT5  // P1.5 for UCB0 SCK connects to LCD SCK
#define LCD_MOSI          BIT6  // P1.6 for UCB0 MOSI connects to LCD SDA
#define LCD_RST           BIT7  // P5.7 connects to LCD RST pin
#define LCD_CS            BIT0  // P5.0 connects to LCD CS pin
#define LCD_DC            BIT7  // P3.7 connects to LCD DC pin

// define # CPU cycles for delays
#define _1ms (3018)
#define _10ms (10 * _1ms)
#define _50ms (50 * _1ms)
#define _120ms (120 * _1ms)
#define _200ms (200 * _1ms)

// Argument for isData in lcdWrite function
#define DATA 1
#define CMD 0

///////////////////////////////////////////////////////////////////////
// lcdWrite - Sends a command/data byte to the LCD.
// Arguments: byte - value to be sent to the LCD
//            isData - true if byte is data, false if byte is a command
// Return Value: none
///////////////////////////////////////////////////////////////////////
void lcdWrite(char byte, int isData);

///////////////////////////////////////////////////////////////////////
// lcdInit - Initializes ports, SPI settings and LCD.
//           This function assumes CPU clock is less than 15MHz.
// Arguments: none
// Return Value: none
///////////////////////////////////////////////////////////////////////
void lcdInit()
{
    // enable pins for UCB0 SPI MOSI/SCK 
    P1->SEL0 |= (LCD_SCK | LCD_MOSI );
    P1->SEL1 &= ~(LCD_SCK | LCD_MOSI );

    // enable CS, RST and DC outputs
    P5->DIR |= (LCD_RST | LCD_CS );
    P3->DIR |= LCD_DC;

    // activate CS
    P5->OUT &= ~LCD_CS;

    // ensure UCB0 SPI module in software reset
    EUSCI_B0->CTLW0 |= EUSCI_B_CTLW0_SWRST;

    // configure SPI as master, MSB first, SMCLK as clock source,
    //	data captured on first clock edge, low clock inactive state
    EUSCI_B0->CTLW0 = EUSCI_B_CTLW0_SSEL__SMCLK | EUSCI_B_CTLW0_MST
            | EUSCI_B_CTLW0_MSB | EUSCI_B_CTLW0_SYNC | EUSCI_B_CTLW0_CKPH;

    // take UCB0 SPI module out of software reset
    EUSCI_B0->CTLW0 &= ~EUSCI_B_CTLW0_SWRST;

    // Reset the display with hardware reset
    P5->OUT &= ~LCD_RST;
    __delay_cycles(_50ms);
    P5->OUT |= LCD_RST;
    __delay_cycles(_120ms);

    // turn off sleep mode and wait for 120ms
    lcdWrite(CM_SLPOUT, CMD);
    __delay_cycles(_120ms);

    // enable Gamma Curve and select 3 curves
    lcdWrite(CM_GAMSET, CMD);
    lcdWrite(0x04, DATA);

    //set the frame frequency of the full colors normal mode to 850kHz/(4*2+40)*(LINE+8)
    lcdWrite(CM_SETPWCTR, CMD);
    lcdWrite(0x0A, DATA);
    lcdWrite(0x14, DATA);

    // *** ADD COMMENT HERE ***
    lcdWrite(CM_SETSTBA, CMD);
    lcdWrite(0x0A, DATA);
    lcdWrite(0x00, DATA);

    // define the RGB picture to be 16-bit data
    lcdWrite(CM_COLMOD, CMD);
    lcdWrite(0x05, DATA);
    __delay_cycles(_10ms);

    //defines read/write scanning direction to be color selector switch control
    lcdWrite(CM_MADCTL, CMD);
    lcdWrite(CM_MADCTL_BGR, DATA);

    // returns the display to normal mode
    lcdWrite(CM_NORON, CMD);

    // clear the display
    lcdClear(BLACK);

    // turn on the display
    __delay_cycles(_10ms);
    lcdWrite(CM_DISPON, CMD);
}

///////////////////////////////////////////////////////////////////////
// lcdClear - Clears the entire LCD display
// Arguments: colour - A 16 bit value to represent the RGB colour to be
//                     clear all pixels to
// Return Value: none
///////////////////////////////////////////////////////////////////////
void lcdClear(int colour)
{
    int row, col;  // variables used to track current pixel while clearing

    // set x range
    lcdWrite(CM_CASET, CMD);
    lcdWrite(0, DATA);
    lcdWrite(X_CORRECTION_OFFSET, DATA);
    lcdWrite(0, DATA);
    lcdWrite(LCD_MAX_X + X_CORRECTION_OFFSET, DATA);

    // set y range
    lcdWrite(CM_RASET, CMD);
    lcdWrite(0, DATA);
    lcdWrite(Y_CORRECTION_OFFSET, DATA);
    lcdWrite(0, DATA);
    lcdWrite(LCD_MAX_Y + Y_CORRECTION_OFFSET, DATA);

    // clear all pixels in range
    lcdWrite(CM_RAMWR, CMD);
    for (row = 0; row <= LCD_MAX_Y; row++)
    {
        for (col = 0; col <= LCD_MAX_X; col++)
        {
            // clear to desired colour
            lcdWrite(colour >> 8, DATA);
            lcdWrite(colour, DATA);
        }
    }

}

///////////////////////////////////////////////////////////////////////
// lcdSetPixel - Sets the pixel colour at the specified display location.
//               The bottom left corner of the LCD is (0,0)
// Arguments: x - The horizontal location of the pixel
//            y - The vertical location of the pixel
//            colour - A 16 bit value to represent the RGB colour to be
//                     output at the pixel location
// Return Value: none
///////////////////////////////////////////////////////////////////////
void lcdSetPixel(int x, int y, int colour)
{

    // set x co-ordinate for the pixel
    lcdWrite(CM_CASET, CMD);
    lcdWrite(0, DATA);
    lcdWrite(x + X_CORRECTION_OFFSET, DATA);
    lcdWrite(0, DATA);
    lcdWrite(x + X_CORRECTION_OFFSET, DATA);

    // set y co-ordinate for the pixel
    lcdWrite(CM_RASET, CMD);
    lcdWrite(0, DATA);
    lcdWrite(y + Y_CORRECTION_OFFSET, DATA);
    lcdWrite(0, DATA);
    lcdWrite(y + Y_CORRECTION_OFFSET, DATA);

    // output colour to the pixel location
    lcdWrite(CM_RAMWR, CMD);
    lcdWrite(colour >> 8, DATA);
    lcdWrite(colour, DATA);
}

///////////////////////////////////////////////////////////////////////
// lcdWrite - Sends a command/data byte to the LCD.
// Arguments: byte - value to be sent to the LCD
//            isData - true if byte is data, false if byte is a command
// Return Value: none
///////////////////////////////////////////////////////////////////////
void lcdWrite(char byte, int isData)
{
    // poll UCBUSY register bit to determine if finished transmitting previous byte 
    while ((EUSCI_B0->STATW & EUSCI_A_STATW_SPI_BUSY))
    {
    }
    // if byte is data
    if (isData)
    {
        // set data/command pin
        P3->OUT |= LCD_DC;
    }
    else
        // else clear data/command pin
        P3->OUT &= ~LCD_DC;

    // Transmit byte
    EUSCI_B0->TXBUF = byte;
}
