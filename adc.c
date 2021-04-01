#include "adc.h"
//#include "msp.h"

void adcInit(int channel)
{


    ADC14->CTL0 &= ~ADC14_CTL0_ENC; //make sure the the conversion is disable so can start configuration

    ADC14->CTL0 = ADC14_CTL0_ON | ADC14_CTL0_SHP; //turn on the ADC and use the sampling timer

    ADC14->MCTL[0] = channel;  //select a specific analogy input for a specific MEM register

    ADC14->CTL0 |= ADC14_CTL0_ENC; //enable conversion


}

int adcSample()
{
    volatile int result;

    ADC14->CTL0 |= ADC14_CTL0_SC; //start conversion

    while ((ADC14->IFGR0 & ADC14_IFGR0_IFG0) == 0)
        {
        } //check for the completion of the conversion

        result = ADC14->MEM[0]; //get the conversion result

        return result;
}
