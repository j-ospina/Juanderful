#ifndef ADC_DRIVERS_H_
#define ADC_DRIVERS_H_

#include <driverlib.h>
#include <msp.h>

#define ADC_PIN 0

void ADC_Init();
void ADC_Start();
void ADC_Stop();

#endif /* ADC_DRIVERS_H_ */
