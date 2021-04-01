/*
 * adc.h
 *description: contain a certain sequence of setting up ACD14 to do a conversion
 *  Created on: Oct 16, 2019
 *      Author: Yanming Bu
 */

#include "msp.h"

#ifndef ADC_H_
#define ADC_H_

void adcInit(int channel);//perform a conversion and returns the digital result

int adcSample();//



#endif /* ADC_H_ */
