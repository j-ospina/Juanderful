#include "Oscilloscope.h"
#include "driverlib.h"
#include "DebugTools.h"

/***************** FRONT END CONTROLS ********************/
/* Vga Control*/
static void Osc_InitVga(){
    Osc_VgaGain = 0;
    P4->OUT = Osc_VgaGain;
    OSC_VGA_LATCH_TGL;
}

static void Osc_IncrementVga(){
    if(Osc_VgaGain < OSC_VGA_MAX_GAIN){
        Osc_VgaGain++;
        P4->OUT = Osc_VgaGain;
        OSC_VGA_LATCH_TGL;
    }
}

static void Osc_DecrementVga(){
    if(Osc_VgaGain > OSC_VGA_MIN_GAIN){
        Osc_VgaGain--;
        P4->OUT = Osc_VgaGain;
        OSC_VGA_LATCH_TGL;
    }
}
/* Vga Control*/

/* Atten Control*/
static void Osc_SetAtten1_1(){
    P6->OUT = OSC_1_1_Atten_En;
    AttenuatorState = AttenState_1_1;
    OSC_ATTEN_LATCH_TGL;
}
static void Osc_SetAtten2_1(){
    P6->OUT = OSC_2_1_Atten_En;
    AttenuatorState = AttenState_2_1;
    OSC_ATTEN_LATCH_TGL;
}
static void Osc_SetAtten5_1(){
    P6->OUT = OSC_5_1_Atten_En;
    AttenuatorState = AttenState_5_1;
    OSC_ATTEN_LATCH_TGL;
}

static void Osc_IncrementAtten(){
    switch(AttenuatorState){
        case AttenState_1_1:
            break;
        case AttenState_2_1:
            Osc_SetAtten1_1();
            break;
        case AttenState_5_1:
            Osc_SetAtten2_1();
            break;
    }
}

static void Osc_DecrementAtten(){
    switch(AttenuatorState){
        case AttenState_1_1:
            Osc_SetAtten2_1();
            break;
        case AttenState_2_1:
            Osc_SetAtten5_1();
            break;
        case AttenState_5_1:
            break;
    }
}
/* Atten Control*/

/* Gain Controls*/
static void Osc_GpioHorzZoomIn(){
    if(AttenuatorState == AttenState_1_1){
        Osc_IncrementVga();
    }
    else{
        Osc_IncrementAtten();
    }
}
static void Osc_GpioHorzZoomOut(){
    if(Osc_VgaGain == OSC_VGA_MIN_GAIN){
        Osc_DecrementAtten();
    }
    else{
        Osc_DecrementVga();
    }
}
/* Gain Controls*/
static void Osc_GpioVertZoomOut(){

}
static void Osc_GpioVertZoomIn(){

}

/*Button Handlers*/
static void Osc_GpioHandleButton(){
    uint8_t PinStates = P3->IN;
    if(PinStates & OSC_HorzZoomDn){
        Osc_GpioHorzZoomIn();
    }
    if(PinStates & OSC_VertZoomDn){
        Osc_GpioVertZoomOut();
    }
    if(PinStates & OSC_VertZoomUp){
        Osc_GpioVertZoomIn();
    }
    if(PinStates & OSC_HorzZoomUp){
        Osc_GpioHorzZoomOut();
    }
    if(PinStates & OSC_TrigDn){
        Oscilloscope.TriggerOldValue = Oscilloscope.TriggerValue;
        Oscilloscope.TriggerValue--;
        Oscilloscope.UpdateTrigger = true;
    }
    if(PinStates & OSC_TrigUp){
        Oscilloscope.TriggerOldValue = Oscilloscope.TriggerValue;
        Oscilloscope.TriggerValue++;
        Oscilloscope.UpdateTrigger = true;
    }
}
/*Button Handlers*/

static void Osc_GpioDebounceTimerInit(){
    TIMER_A0->CTL = (TIMER_A_CTL_MC_0 | TIMER_A_CTL_CLR);
    TIMER_A0->CTL |= (TIMER_A_CTL_TASSEL_2 | TIMER_A_CTL_ID_0 | TIMER_A_CTL_IE);
    TIMER_A0->CCTL[0] = (TIMER_A_CCTLN_CCIE);
    TIMER_A0->CCR[0] = (uint16_t) 10000;

    NVIC_EnableIRQ(TA0_0_IRQn);
    Interrupt_enableInterrupt(INT_TA0_0);
}

static void Osc_GpioDebounceTimerStart(){
    TIMER_A0->CTL |= (TIMER_A_CTL_MC__UP | TIMER_A_CTL_CLR);
}

static void Osc_GpioDebounceTimerStop(){
    TIMER_A0->CTL = (TIMER_A_CTL_MC_0 | TIMER_A_CTL_TASSEL_2 | TIMER_A_CTL_ID_0 | TIMER_A_CTL_IE | TIMER_A_CTL_CLR);
}
static void Osc_GpioIntEnable(){
    NVIC_EnableIRQ(PORT3_IRQn);
}
static void Osc_GpioIntDisable(){
    NVIC_DisableIRQ(PORT3_IRQn);
}

void Osc_GpioInit(){
    /* GPIO For Oscilloscope:
     * Inputs:
     * P3.0 - HorzZoomDn
     * P3.1 - VertZoomDn
     * P3.2 - VertZoomUp
     * P3.3 - HorzZoomUp
     * P3.4 - TrigDn
     * P3.5 - TrigUp
     *
     * Outputs:
     * P4.0 - VGA_Gain0
     * P4.1 - VGA_Gain1
     * P4.2 - VGA_Gain2
     * P4.3 - VGA_Gain3
     * P4.4 - VGA_Gain4
     * P4.5 - VGA_Latch
     *
     * P6.0 - 1:1 Atten_En
     * P6.1 - 2:1 Atten_En
     * P6.2 - 5:1 Atten_En
     * P6.3 - Atten_Latch
     */

    //Enable Outputs
    P4->OUT &= ~(OSC_VgaGroup);
    P4->DIR |= (OSC_VgaGroup);
    P4->SEL0 &= ~(OSC_VgaGroup);
    P4->SEL1 &= ~(OSC_VgaGroup);
    Osc_InitVga();


    P6->OUT &= ~(OSC_AttenGroup);
    P6->DIR |= (OSC_AttenGroup);
    P6->SEL0 &= ~(OSC_AttenGroup);
    P6->SEL1 &= ~(OSC_AttenGroup);
    Osc_SetAtten1_1();

    //Enable Inputs
    P3->DIR = 0x00;
    P3->SEL0 &= ~(OSC_InputGroup);
    P3->SEL1 &= ~(OSC_InputGroup);
    P3->IE |= OSC_InputGroup;
    P3->IES &= ~(OSC_InputGroup);

    NVIC_EnableIRQ(PORT3_IRQn);
    Interrupt_enableInterrupt(INT_PORT3);
    Osc_GpioDebounceTimerInit();
}

__interrupt void PORT3_IRQHandler(){
    Osc_GpioIntDisable();
    P3->IE &= (OSC_InputGroup);
    P3->IFG = 0;

    Osc_GpioDebounceTimerStart();
}

__interrupt void TA0_0_IRQHandler(){
    Osc_GpioDebounceTimerStop();
    Osc_GpioIntEnable();
    TIMER_A0->CTL &= ~(TIMER_A_CTL_IFG);
    TIMER_A0->CCTL[0] &= ~(TIMER_A_CCTLN_CCIFG);

    Osc_GpioHandleButton();
}
/***************** FRONT END CONTROLS ********************/

/***************** OSCILLOSCOPE STRUCTURE ********************/
/* Data Management */
void Osc_AddDataToBuffer(uint8_t data){
    data -= 8;
    if(data >= 248) {data = 0;}
    if(data > 240)  {data = 240;}

    if(InputBuffer->BufferStatus == BufferState_Filling){
        InputBuffer->Buffer[InputBuffer->BufferFillIndex++] = data;

        if(InputBuffer->BufferFillIndex >= OSC_GRAPH_WIDTH){
            InputBuffer->BufferStatus = BufferState_Full;
            Osc_UpdateInputBuffer();
        }
        return;
    }

    Osc_UpdateInputBuffer();
}

void Osc_UpdateInputBuffer(){
    //Update Old Input Buffer Status
    InputBuffer->BufferStatus = BufferState_Full;

    //Update Input Buffer pointer
    InputBuffer = InputBuffer->NextBuffer;

    //Update New Input Buffer Status
    InputBuffer->BufferStatus = BufferState_Filling;
}

void Osc_UpdateOutputBuffer(){
    //Update Output Buffer to Latest filled buffer
    OutputBuffer = InputBuffer->LastBuffer;

    //Remove from Linked List
    OutputBuffer->NextBuffer->LastBuffer = OutputBuffer->LastBuffer;
    OutputBuffer->LastBuffer->NextBuffer = OutputBuffer->NextBuffer;

    //Update new Output Buffer status
    OutputBuffer->BufferStatus = BufferState_Emptying;
}

void Osc_UpdateDirtyBuffer(){
    //Point to Dirty Buffer to Old output buffer
    DirtyBuffer = OutputBuffer;

    //Update Dirty buffer status
    DirtyBuffer->BufferStatus = BufferState_Dirty;
}

void Osc_ReinsertCleanedBuffer(){

    //Place behind Input buffer
    DirtyBuffer->NextBuffer = InputBuffer;
    DirtyBuffer->LastBuffer = InputBuffer->LastBuffer;
    DirtyBuffer->NextBuffer->LastBuffer = DirtyBuffer;
    DirtyBuffer->LastBuffer->NextBuffer = DirtyBuffer;

    //Update buffer state
    DirtyBuffer->BufferStatus = BufferState_Available;
}

void Osc_BuffersInit(){
    //Connect Buffers Forward
    for(int i = 0; i < OSC_BUFFER_AMOUNT - 1; i++){
        BufferArray[i].NextBuffer = &BufferArray[i+1];
    }
    BufferArray[OSC_BUFFER_AMOUNT - 1].NextBuffer = &BufferArray[0];

    //Connect Buffers Backward;
    for(int i = OSC_BUFFER_AMOUNT - 1; i > 0; i--){
        BufferArray[i].LastBuffer = &BufferArray[i-1];
    }
    BufferArray[0].LastBuffer = &BufferArray[OSC_BUFFER_AMOUNT - 1];

    //Initialize Input Buffer
    InputBuffer = &BufferArray[0];
    InputBuffer->BufferStatus = BufferState_Filling;
}

void Osc_OscilloscopeInit(){
    Osc_GpioInit();
    Osc_BuffersInit();
    Oscilloscope.TriggerValue = 128;
    Oscilloscope.RefreshGraphBackground = true;
    Oscilloscope.UpdateTrigger = true;
}
/* Data Management */
/***************** OSCILLOSCOPE STRUCTURE ********************/
