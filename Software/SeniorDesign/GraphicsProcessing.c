#include "GraphicsProcessing.h"

static void GP_DrawGraphLines(){
    //External Lines
    LCD_DrawHorizontalLine(GP_GRAPH_MIN_X, GP_GRAPH_MAX_X-1, GP_GRAPH_MIN_Y, LCD_WHITE);
    LCD_DrawHorizontalLine(GP_GRAPH_MIN_X, GP_GRAPH_MAX_X, GP_GRAPH_MAX_Y-1, LCD_WHITE);
    LCD_DrawVerticalLine(GP_GRAPH_MIN_X, GP_GRAPH_MIN_Y, GP_GRAPH_MAX_Y-1, LCD_WHITE);
    LCD_DrawVerticalLine(GP_GRAPH_MAX_X-1, GP_GRAPH_MIN_Y, GP_GRAPH_MAX_Y-1, LCD_WHITE);

    //Primary Internal Lines
    LCD_DrawHorizontalLine(GP_GRAPH_MIN_X, GP_GRAPH_MAX_X-1, GP_GRAPH_MID_Y, LCD_GRAY);
    LCD_DrawVerticalLine(GP_GRAPH_MID_X, GP_GRAPH_MIN_Y, GP_GRAPH_MAX_Y-1, LCD_GRAY);

    //Secondary Internal Lines
    LCD_DrawHorizontalLine(GP_GRAPH_MIN_X, GP_GRAPH_MAX_X-1, GP_GRAPH_1_4_Y, LCD_GRAY);
    LCD_DrawHorizontalLine(GP_GRAPH_MIN_X, GP_GRAPH_MAX_X-1, GP_GRAPH_3_4_Y, LCD_GRAY);
    LCD_DrawVerticalLine(GP_GRAPH_1_4_X, GP_GRAPH_MIN_Y, GP_GRAPH_MAX_Y-1, LCD_GRAY);
    LCD_DrawVerticalLine(GP_GRAPH_3_4_X, GP_GRAPH_MIN_Y, GP_GRAPH_MAX_Y-1, LCD_GRAY);

    //Reset LCD GRAM
    LCD_ResetRamArea();
    Oscilloscope.RefreshGraphBackground = false;
}

static void GP_EraseTrigger(){
    //Erase trigger
    for(int i = 0; i < 5; i++){
        LCD_DrawVerticalLine(GP_GRAPH_MAX_X+1+i, Oscilloscope.TriggerValue - i, Oscilloscope.TriggerValue + i, LCD_BLACK);
    }
}

static void GP_DrawTrigger(){
    //Draw trigger
    for(int i = 0; i < 5; i++){
        LCD_DrawVerticalLine(GP_GRAPH_MAX_X+1+i, Oscilloscope.TriggerValue - i, Oscilloscope.TriggerValue + i, LCD_YELLOW);
    }

    //Reset LCD GRAM
    LCD_ResetRamArea();

    //No need to redraw
    Oscilloscope.UpdateTrigger = false;
}
void GP_ClearValues(){
    //Remove dirty buffer values from graph
    for(uint8_t i = 0; i < OSC_GRAPH_WIDTH; i++){
        LCD_SetPoint(i + GP_GRAPH_MIN_X, DirtyBuffer->Buffer[i + DirtyBuffer->BufferStartIndex], LCD_BLACK);
    }
//    for(uint8_t i = 0; i < OSC_GRAPH_WIDTH - 1; i++){
//        LCD_DrawLine(i + GP_GRAPH_MIN_X, OutputBuffer->Buffer[i + OutputBuffer->BufferStartIndex], i + GP_GRAPH_MIN_X+1, OutputBuffer->Buffer[i + OutputBuffer->BufferStartIndex + 1], LCD_BLACK);
//    }

    Osc_ReinsertCleanedBuffer();
}

void GP_DrawGraphValues(){
    //Get latest values
    Osc_UpdateOutputBuffer();

    //Draw on graph
    for(uint8_t i = 0; i < OSC_GRAPH_WIDTH; i++){
        LCD_SetPoint(i + GP_GRAPH_MIN_X, OutputBuffer->Buffer[i + OutputBuffer->BufferStartIndex], LCD_YELLOW);
    }
//    for(uint8_t i = 0; i < OSC_GRAPH_WIDTH - 1; i++){
//        LCD_DrawLine(i + GP_GRAPH_MIN_X, OutputBuffer->Buffer[i + OutputBuffer->BufferStartIndex], i + GP_GRAPH_MIN_X+1, OutputBuffer->Buffer[i + OutputBuffer->BufferStartIndex + 1], LCD_YELLOW);
//    }


    //Update Dirty Buffer
    Osc_UpdateDirtyBuffer();
}

void GP_RunGraphics(){
    if(Oscilloscope.RefreshGraphBackground){
        GP_DrawGraphLines();
    }

    if(Oscilloscope.UpdateTrigger){
        GP_EraseTrigger();
        GP_DrawTrigger();
    }

    GP_DrawGraphValues();
    GP_ClearValues();
}
