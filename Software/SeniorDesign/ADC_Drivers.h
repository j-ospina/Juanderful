#ifndef ADC_DRIVERS_H_
#define ADC_DRIVERS_H_

#include <driverlib.h>
#include <msp.h>


#define ADC_PIN 0

/*DMA Stuff*/
typedef uint32_t DmaControlTable_t[4];

/*ADC Initialization*/
typedef enum{
    ADC_WithInt = 0,
    ADC_WithDma = 1
}AdcInit_t;

void ADC_DmaInit();
void ADC_Init(AdcInit_t initType);
void ADC_Start();
void ADC_Stop();

#endif /* ADC_DRIVERS_H_ */
