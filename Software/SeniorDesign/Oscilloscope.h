#ifndef OSCILLOSCOPE_H_
#define OSCILLOSCOPE_H_

#include <LCD_Drivers.h>
#include <stdint.h>
#include <stdbool.h>
#include "msp.h"
#include "GraphicsProcessing.h"

//Constants
#define OSC_GRAPH_HEIGHT MAX_SCREEN_Y
#define OSC_GRAPH_MIDDLE_HEIGHT (OSC_GRAPH_HEIGHT/2)
#define OSC_GRAPH_WIDTH 255
#define OSC_GRAPH_SAMPLES OSC_GRAPH_WIDTH

//Memory
#define OSC_BUFFER_AMOUNT 8
#define OSC_BUFFER_SIZE OSC_GRAPH_SAMPLES

/***************** FRONT END CONTROLS ********************/
/* Pin Assignments */
#define OSC_HorzZoomDn  GPIO_PIN0
#define OSC_VertZoomDn  GPIO_PIN2
#define OSC_VertZoomUp  GPIO_PIN3
#define OSC_HorzZoomUp  GPIO_PIN5
#define OSC_TrigDn      GPIO_PIN6
#define OSC_TrigUp      GPIO_PIN7
#define OSC_InputGroup  OSC_HorzZoomDn | OSC_VertZoomDn | OSC_VertZoomUp | OSC_HorzZoomUp | OSC_TrigDn | OSC_TrigUp

#define OSC_VGA_Gain0   GPIO_PIN0
#define OSC_VGA_Gain1   GPIO_PIN1
#define OSC_VGA_Gain2   GPIO_PIN2
#define OSC_VGA_Gain3   GPIO_PIN3
#define OSC_VGA_Gain4   GPIO_PIN4
#define OSC_VGA_Latch   GPIO_PIN5
#define OSC_VgaGroup    OSC_VGA_Gain0 | OSC_VGA_Gain1 | OSC_VGA_Gain2 | OSC_VGA_Gain3 | OSC_VGA_Gain4 | OSC_VGA_Latch

#define OSC_1_1_Atten_En    GPIO_PIN0
#define OSC_2_1_Atten_En    GPIO_PIN1
#define OSC_5_1_Atten_En    GPIO_PIN2
#define OSC_Atten_Latch     GPIO_PIN3
#define OSC_AttenGroup      OSC_1_1_Atten_En | OSC_2_1_Atten_En | OSC_5_1_Atten_En | OSC_Atten_Latch
/* Pin Assignments */

//VGA Control
#define OSC_VGA_LATCH_EN  P4->OUT &= ~(OSC_VGA_Latch)
#define OSC_VGA_LATCH_DIS P4->OUT |= (OSC_VGA_Latch)
#define OSC_VGA_LATCH_TGL OSC_VGA_LATCH_EN; OSC_VGA_LATCH_DIS
#define OSC_VGA_MAX_GAIN 0x1F
#define OSC_VGA_MIN_GAIN 0
uint8_t Osc_VgaGain;

//Atten Control
#define OSC_ATTEN_LATCH_EN P6->OUT &= ~(OSC_Atten_Latch)
#define OSC_ATTEN_LATCH_DIS P6->OUT |= OSC_Atten_Latch
#define OSC_ATTEN_LATCH_TGL OSC_ATTEN_LATCH_EN; OSC_ATTEN_LATCH_DIS

typedef enum{
    AttenState_1_1 = 0,
    AttenState_2_1 = 1,
    AttenState_5_1 = 2,
}AttenuatorState_t;

AttenuatorState_t AttenuatorState;
/***************** FRONT END CONTROLS ********************/
/* Buffer Def*/
typedef enum {
    BufferState_Available = 0,
    BufferState_Filling = 1,
    BufferState_Full = 2,
    BufferState_Emptying = 3,
    BufferState_Dirty = 4,
}BufferState_t;


typedef struct OscBuffer_t OscBuffer_t;

struct OscBuffer_t{
    uint8_t Buffer[OSC_GRAPH_SAMPLES];
    OscBuffer_t *LastBuffer;
    OscBuffer_t *NextBuffer;
    BufferState_t BufferStatus;
    uint8_t BufferStartIndex;
    uint8_t BufferFillIndex;
};

OscBuffer_t BufferArray[OSC_BUFFER_AMOUNT];
OscBuffer_t *DirtyBuffer;
OscBuffer_t *OutputBuffer;
OscBuffer_t *InputBuffer;
/* Buffer Def*/

/* Oscilloscope State*/
typedef struct {
    //Graphics Indicators
    bool UpdateTrigger;
    bool RefreshGraphBackground;
    volatile bool TriggerValueew;
    volatile uint8_t TriggerValue;
    volatile uint8_t TriggerOldValue;
} Oscilloscope_t;

Oscilloscope_t Oscilloscope;
/* Oscilloscope State*/

void Osc_GpioInit();
void Osc_AddDataToBuffer(uint8_t data);
void Osc_UpdateInputBuffer();
void Osc_UpdateOutputBuffer();
void Osc_UpdateDirtyBuffer();
void Osc_ReinsertCleanedBuffer();
void Osc_OscilloscopeInit();



#endif /* OSCILLOSCOPE_H_ */

