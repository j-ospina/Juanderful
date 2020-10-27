#include "ADC_Drivers.h"

void (*AdcFunction)(void);
volatile int ReadValue;

/*
static void ADC_DmaInit(){

}
*/

void ADC_Init(){
        P6SEL1 |= BIT0;
        P6SEL0 |= BIT0;
        P6->DIR &= ~BIT0;

        ADC14->CTL0 &= ~(ADC14_CTL0_ON | ADC14_CTL0_ENC);

        ADC14->CTL0 = ADC14_CTL0_PDIV0 | ADC14_CTL0_SHS_0 | ADC14_CTL0_SHP | ADC14_CTL0_DIV_0 | ADC14_CTL0_SSEL__SMCLK | ADC14_CTL0_CONSEQ_2 | ADC14_CTL0_MSC;

        ADC14->CTL1 = ADC14_CTL1_CH0MAP | ADC14_CTL1_RES__8BIT;

        ADC14->MCTL[ADC_PIN] = ADC14_MCTLN_DIF | ADC14_MCTLN_VRSEL_0;

        /*WITH INTERRUPT*/
        ADC14->IER0 |= (1 << ADC_PIN);

        NVIC_EnableIRQ(ADC14_IRQn);
        Interrupt_enableInterrupt(INT_ADC14);
}

void ADC_Start(){
    ADC14->CTL0 |= ADC14_CTL0_ON | ADC14_CTL0_ENC;
    ADC14->CTL0 |= ADC14_CTL0_SC;
}

void ADC_Stop(){
    ADC14->CTL0 &= ~(ADC14_CTL0_ON | ADC14_CTL0_ENC);
}

__interrupt void ADC14_IRQHandler(void){
    ReadValue = ADC14->MEM[ADC_PIN];
    ADC14->CLRIFGR0 |= (1 << ADC_PIN);
}
