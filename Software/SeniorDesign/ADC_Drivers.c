#include "ADC_Drivers.h"
#include "Oscilloscope.h"
#include "DebugTools.h"


#pragma DATA_ALIGN(DmaControlTable, 1024)
static DmaControlTable_t DmaControlTable[1];

void ADC_DmaInit(){
    /*
     * Allow Configuration
     */
    DMA_enableModule();

    //Point to allocated control space
    DMA_setControlBase(DmaControlTable);

    //Assign channel to allocated space
    DMA_assignChannel(DMA_CH7_ADC14);

    //Remove unwanted attributes
    DMA_disableChannelAttribute(DMA_CH7_ADC14, UDMA_ATTR_ALTSELECT | UDMA_ATTR_USEBURST | UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK);

    /* DMA Settings
     * -8-bit transfer
     * -No source increment
     * -8-bit destination increment
     * -Arbitration 1 (?)
     */
    DMA_setChannelControl(DMA_CH7_ADC14, UDMA_SIZE_8 | UDMA_SRC_INC_NONE | UDMA_DST_INC_8 | UDMA_ARB_1);

    /* DMA Transfer Settings
     * -Primary Structure (Won't use alternates)
     * -Basic transfer mode
     * -Src: ADC Mem0 Register
     * -Dst: InputBuffer
     * -Size: Size of buffer;
     */
    DMA_setChannelTransfer(UDMA_PRI_SELECT | DMA_CH7_ADC14, UDMA_MODE_BASIC, (void *)(&ADC14->MEM[0]), InputBuffer, OSC_BUFFER_SIZE);

    //Assign/Enable Interrupt
    DMA_assignInterrupt(DMA_INT0, 7);
    Interrupt_enableInterrupt(INT_DMA_INT0);

    //Enable Channel
    DMA_enableChannel(7);
}

void ADC_DmaStart(){
    //Update DMA settings (mostly Input Buffer)
    DMA_setChannelTransfer(UDMA_PRI_SELECT | DMA_CH7_ADC14, UDMA_MODE_BASIC, (void *)(&ADC14->MEM[0]), InputBuffer, OSC_BUFFER_SIZE);

    //Enable Channel
    DMA_enableChannel(7);
}

void ADC_Init(AdcInit_t initType){
    //GPIO Settings
    P5->SEL1 |= (BIT7 | BIT6);
    P5->SEL0 |= (BIT7 | BIT6);

    //ADC Stop and Disable
    ADC14->CTL0 &= ~(ADC14_CTL0_ON | ADC14_CTL0_ENC);

    /*ADC Settings 0:
     * - No Predivision on ClockSource
     * - Sample and Hold source based on ADC14SC bit
     * - SAMPCON is sourced from sample-input signal
     * - ADC ClockDiv is /1
     * - ADC Clocksource is SMCLK
     * - ADC Consequence is Repeat-Single-channel
     */
    ADC14->CTL0 = ADC14_CTL0_PDIV0 | ADC14_CTL0_SHS_0 | ADC14_CTL0_SHP | ADC14_CTL0_DIV_0 | ADC14_CTL0_SSEL__SMCLK | ADC14_CTL0_CONSEQ_2 | ADC14_CTL0_MSC;

    /*ADC Settings 1:
     * - Input channel 0 for ADC Input Channel
     * - 8-bit resolution
     */
    ADC14->CTL1 = ADC14_CTL1_CH0MAP | ADC14_CTL1_RES__8BIT;

    /*ADC Settings MCTL:
     * - Differential mode
     * - AVCC and AVSS for V(R+) and V(R-) respectively
     * - ADC14_MCTLN_INCH_4
     */
    ADC14->MCTL[ADC_PIN] = ADC14_MCTLN_DIF | ADC14_MCTLN_INCH_4 | ADC14_MCTLN_VRSEL_0;

    /*
     * Memory retrieval
     */
    switch(initType){
    case ADC_WithInt:
        //Interrupt based retrieval
        ADC14->IER0 |= (1 << ADC_PIN);
        NVIC_EnableIRQ(ADC14_IRQn);
        Interrupt_enableInterrupt(INT_ADC14);
    case ADC_WithDma:
        //DMA based retrieval
        ADC_DmaInit();
        break;
    }
}

void ADC_Start(){
    //Enable and start ADC
    ADC14->CTL0 |= ADC14_CTL0_ON | ADC14_CTL0_ENC;
    //First Trigger
    ADC14->CTL0 |= ADC14_CTL0_SC;
}

void ADC_Stop(){
    //Disable ADC
    ADC14->CTL0 &= ~(ADC14_CTL0_ON | ADC14_CTL0_ENC);
}
__interrupt void DMA_INT0_IRQHandler(void){
    //Clear Flag
    DMA_Channel->INT0_CLRFLG &= ~(DMA_INT0_CLRFLG_CH7);

    //Update buffer status
    InputBuffer->BufferStatus = BufferState_Full;

    //Update Input Buffer
    Osc_UpdateInputBuffer();

    //Reset DMA
    ADC_DmaStart();
}
__interrupt void ADC14_IRQHandler(void){
    Osc_AddDataToBuffer((int)ADC14->MEM[ADC_PIN]);
    ADC14->CLRIFGR0 |= (1 << ADC_PIN);
}
