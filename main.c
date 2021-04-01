#include "msp.h"
#include "time.h"
#include "stdlib.h"
#include "lcd.h"
#include "adc.h"
#include "uart.h"

#define TICKS_PER_100MS 37500
#define X_INPUT_CHAN 15 //use A15 connected to p6.0
#define Y_INPUT_CHAN 9 //use A9 connected to p4.4
#define FULL 16384.0
#define JOYSTICK_PIN P4
#define JOYSTICK_PB BIT1
#define S1_PIN P5
#define PB1 BIT1

#define ONE_CYCLE 3006 //1ms delay

int count = 0;
int button_pushed = 0;
int start = 0;

void msDelay(unsigned int ms)
{
    while (ms > 0)
    {
        //delay for one cycle
        _delay_cycles(ONE_CYCLE);
        ms--;
    }
}

void draw_symbol(int number);

void check_input(int number);

void main(void)
{
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;		// stop watchdog timer

    int start_point = 0;
    int end_point = 0;
    int loops = 0;
    volatile double duration_s = 0.0;
    int value_5;
    int value_4;
    int value_3;
    int value_2;
    int value_1;
    int value;

    //set up ports for communication
    P1->SEL0 |= BIT2 | BIT3;
    P1->SEL1 &= ~(BIT2 | BIT3 );

    //initialize the LCD
    lcdInit();

    //initialize UART communication
    uartInit();

    //set up to generate a random number
    srand(time(NULL));   // Initialization, should only be called once.
    int random_number = rand() % 5; // Returns a pseudo-random integer between 0 and 4.

    //configure Pin 4.4 as an analog input for Y-input
    P4->SEL0 |= BIT4;
    P4->SEL1 |= BIT4;

    //configure Pin 6.0 as an analog input for X-input
    P6->SEL0 |= BIT0;
    P6->SEL1 |= BIT0;

    //set up P5.1 as input for S1
    S1_PIN->DIR &= ~PB1;

    //set up P4.1 as input for the Joystick select pushbutton
    JOYSTICK_PIN->DIR &= ~JOYSTICK_PB;

    //enable interrupt for S1 and joystick select pushbutton
    JOYSTICK_PIN->IE |= JOYSTICK_PB;
    S1_PIN->IE |= PB1;

    //edge trigger on falling for S1 and joystick port
    JOYSTICK_PIN->IES |= JOYSTICK_PB;
    S1_PIN->IES |= PB1;

    //clear flag for joystick and S1
    JOYSTICK_PIN->IFG &= ~JOYSTICK_PB;
    S1_PIN->IFG &= ~PB1;

    //enable timer A2 in up mode with SMCLK divided by 8 and enable  for interrupt
    TIMER_A2->CTL = TIMER_A_CTL_SSEL__SMCLK | TIMER_A_CTL_MC__UP
            | TIMER_A_CTL_ID__8 | TIMER_A_CTL_IE;

    //set each interrupt interval to 100ms
    TIMER_A2->CCR[0] = TICKS_PER_100MS - 1;

    //enable NVIC for timer A2 all channels and joystick pushbutton port and S1
    NVIC_EnableIRQ(TA2_N_IRQn);
    NVIC_EnableIRQ(PORT4_IRQn);
    NVIC_EnableIRQ(PORT5_IRQn);

    //global interrupt enable
    __enable_interrupts();

    while (1)
    {
        //clear the LCD
        lcdClear(BLACK);

        // Returns a pseudo-random integer between 0 and 4.
        random_number = rand() % 5;

        //wait for user to push the button
        while (1)
        {
            if (start == 1)
                break;
        }

        //clear start
        start = 0;

        //draw the symbol on LCD screen according to the random number
        draw_symbol(random_number);

        //set count and loop as zero
        count = 0;
        loops = 0;

        //read the timer at the beginning
        start_point = TIMER_A2->R;

        //check for input states
        check_input(random_number);

        //take out count value
        loops = count;

        //reset button_pushed variable
        button_pushed = 0;

        //read the timer at the end
        end_point = TIMER_A2->R;

        //do math to calculate the reaction time
        if (random_number == 4)
            duration_s = (loops * 100 - 10)
                    + (end_point + 65536 - start_point) / 3000;
        else
            duration_s = (loops * 100)
                    + (end_point + 65536 - start_point) / 3000;

        //break the value into string
        value = duration_s;
        value_5 = value / 10000;
        value_4 = (value - value_5 * 10000) / 1000;
        value_3 = (value - value_5 * 10000 - value_4 * 1000) / 100;
        value_2 = (value - value_5 * 10000 - value_4 * 1000 - value_3 * 100)
                / 10;
        value_1 = (value - value_5 * 10000 - value_4 * 1000 - value_3 * 100
                - value_2 * 10);

        //transmit the result and give instructions to keep going
        uartPut_STR("You took ");
        uartPutC('0' + value_5);
        uartPutC('0' + value_4);
        uartPutC('0' + value_3);
        uartPutC('0' + value_2);
        uartPutC('0' + value_1);
        uartPut_STR("ms  ");
        uartPut_STR("Press S1 to restart ");
    }
}

///////////////////////////////////////////////////////////////////////
// interrupt handler for joystick push button - set a value to indicate the button is pressed
// Arguments: none
// Return Value: none
///////////////////////////////////////////////////////////////////////
void PORT4_IRQHandler(void)
{
    //delay 10ms and clear the flag
    msDelay(10);
    JOYSTICK_PIN->IFG &= ~JOYSTICK_PB;

    //debounce the push button by checking if the button is still pushed after 10ms of delay
    if ((JOYSTICK_PIN->IN & JOYSTICK_PB ) == 0)
    {
        button_pushed = 1;//set a value to indicate the button is pressed
    }
}

///////////////////////////////////////////////////////////////////////
// interrupt handler for S1 pushbutton - set a value to start the game
// Arguments: none
// Return Value: none
///////////////////////////////////////////////////////////////////////
void PORT5_IRQHandler(void)
{

    msDelay(10);
    //S1 toggle the blue LED and clear the flag after 10ms of delay
    S1_PIN->IFG &= ~PB1;

    //debounce the push button by checking if the button is still pushed after 10ms of delay
    if ((S1_PIN->IN & PB1 ) == 0)
    {
        start = 1;//set a value to start the game
    }
}

///////////////////////////////////////////////////////////////////////
// interrupt handler for timer 2 - increase count by 1
// Arguments: none
// Return Value: none
///////////////////////////////////////////////////////////////////////
void TA2_N_IRQHandler(void)
{
    //clear flag
    TIMER_A2->CTL &= ~TIMER_A_CTL_IFG;
    TIMER_A2->CTL &= ~TIMER_A_CTL_IFG;

    count++;
}

///////////////////////////////////////////////////////////////////////
// draw_something - according to the number, draw a symbol on LCD
//                  and set up the proper channel for adc
// Arguments: number - a ramdom_number that is generated and indicates what
//                     symbol show be displayed. ;
//                     0 - arrow up
//                     1 - arrow down
//                     2 - arrow left
//                     3 - arrow right
//                     4 - cross
// Return Value: none
///////////////////////////////////////////////////////////////////////
void draw_symbol(int number)
{
    //initial all the variables
    int standard_length;
    int arrow_size;
    int y_value;
    int x_value;
    int size_index;

    //set all the initial values
    standard_length = 100;
    arrow_size = 10;
    size_index = 0;

    //arrow is up or down
    if (number == 0 | number == 1)
    {
        //draw a vertical line
        y_value = 22;
        while (y_value < standard_length)
        {
            lcdSetPixel(LCD_MAX_X / 2, y_value, MAGENTA);
            y_value++;
        }

        //draw arrow pointing up
        if (number == 0)
        {
            while (size_index <= arrow_size)
            {
                size_index++;
                lcdSetPixel((LCD_MAX_X / 2) + size_index, y_value - size_index,
                MAGENTA);
                lcdSetPixel((LCD_MAX_X / 2) - size_index, y_value - size_index,
                MAGENTA);
            }
        }
        //draw arrow pointing down
        else
        {
            while (size_index <= arrow_size)
            {
                y_value = 22;
                size_index++;
                lcdSetPixel((LCD_MAX_X / 2) + size_index, y_value + size_index,
                MAGENTA);
                lcdSetPixel((LCD_MAX_X / 2) - size_index, y_value + size_index,
                MAGENTA);
            }
        }
        //set up adc for y-axial
        adcInit(Y_INPUT_CHAN);
    }

    //if arrow is left or right
    else if (number == 2 | number == 3)
    {
        //draw a horizontal line
        x_value = 22;
        while (x_value < standard_length)
        {
            lcdSetPixel(x_value, LCD_MAX_Y / 2, MAGENTA);
            x_value++;
        }

        //arrow pointing left
        if (number == 2)
        {
            while (size_index <= arrow_size)
            {
                size_index++;
                lcdSetPixel(x_value - size_index, (LCD_MAX_Y / 2) + size_index,
                MAGENTA);
                lcdSetPixel(x_value - size_index, (LCD_MAX_Y / 2) - size_index,
                MAGENTA);
            }
        }
        //arrow pointing right
        else
        {
            while (size_index <= arrow_size)
            {
                x_value = 22;
                size_index++;
                lcdSetPixel(x_value + size_index, (LCD_MAX_Y / 2) + size_index,
                MAGENTA);
                lcdSetPixel(x_value + size_index, (LCD_MAX_Y / 2) - size_index,
                MAGENTA);
            }
        }
        //set up adc for x-axial
        adcInit(X_INPUT_CHAN);
    }

    //draw a cross
    else if (number == 4)
    {
        x_value = 0;
        y_value = 0;
        //draw horizontal line
        while (x_value < LCD_MAX_X)
        {
            lcdSetPixel(x_value, LCD_MAX_Y / 2, MAGENTA);
            x_value++;
        }
        //draw vertical line
        while (y_value < LCD_MAX_Y)
        {
            lcdSetPixel(LCD_MAX_X / 2, y_value, MAGENTA);
            y_value++;
        }
    }
}

///////////////////////////////////////////////////////////////////////
// check_input - checking the position of the joystick to see whether it matches
//              with the the arrow
// Arguments: number - a ramdom_number that is generated and indicates what
//                     symbol show be displayed.
//                     0 - arrow up - joystick needs to move down
//                     1 - arrow down - joystick needs to move up
//                     2 - arrow left - joystick needs to move right
//                     3 - arrow right - joystick needs to move left
//                     4 - cross -      need to push the button of joystick
// Return Value: none
///////////////////////////////////////////////////////////////////////
void check_input(int number)
{
    volatile int result_x;
    volatile int result_y;

    //when the arrow is pointing up
    while (number == 0)
    {
        //get y-position value
        result_y = adcSample();

        //check if joystick position is down and break out of while loop
        if ((result_y) < (0.2 * FULL))
            break;
    }

    //when the arrow is pointing down
    while (number == 1)
    {
        //get y-position value
        result_y = adcSample();

        //check if joystick position is up and break out of while loop
        if ((result_y) > (0.8 * FULL))
            break;
    }

    //when the arrow is pointing left
    while (number == 2)
    {
        //get x-position value
        result_x = adcSample();

        //check if joystick position is right and break out of while loop
        if ((result_x) > (0.8 * FULL))
            break;
    }

    //when the arrow is pointing right
    while (number == 3)
    {
        //get x-position value
        result_x = adcSample();

        //check if joystick position is left and break out of while loop
        if ((result_x) < (0.2 * FULL))
            break;
    }

    //when it is a cross
    while (number == 4)
    {
        //check if joystick bushbutton is pressed
        if (button_pushed == 1)
            break;
    }
}
