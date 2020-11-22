#ifndef GRAPHICSPROCESSING_H_
#define GRAPHICSPROCESSING_H_

#include <stdlib.h>
#include <stdint.h>
#include "LCD_Drivers.h"
#include "Oscilloscope.h"


#define GP_GRAPH_MIN_X MIN_SCREEN_X
#define GP_GRAPH_1_4_X (OSC_GRAPH_WIDTH/4 + GP_GRAPH_MIN_X)
#define GP_GRAPH_MID_X (OSC_GRAPH_WIDTH/2 + GP_GRAPH_MIN_X)
#define GP_GRAPH_3_4_X (OSC_GRAPH_WIDTH/4 + OSC_GRAPH_WIDTH/2 + GP_GRAPH_MIN_X)
#define GP_GRAPH_MAX_X (OSC_GRAPH_WIDTH + GP_GRAPH_MIN_X)

#define GP_GRAPH_MIN_Y MIN_SCREEN_Y
#define GP_GRAPH_1_4_Y (OSC_GRAPH_WIDTH/4 + GP_GRAPH_MIN_Y)
#define GP_GRAPH_MID_Y (OSC_GRAPH_HEIGHT/2 + GP_GRAPH_MIN_Y)
#define GP_GRAPH_3_4_Y (OSC_GRAPH_HEIGHT/4 + OSC_GRAPH_HEIGHT/2 + GP_GRAPH_MIN_Y)
#define GP_GRAPH_MAX_Y (OSC_GRAPH_HEIGHT  + GP_GRAPH_MIN_Y)

#define GP_GRAPH_WIDTH (GP_GRAPH_MAX_X - GP_GRAPH_MIN_X)
#define GP_GRAPH_HEIGHT (GP_GRAPH_MAX_Y - GP_GRAPH_MIN_Y)

void GP_ClearValues();
void GP_DrawGraphValues();
void GP_DrawGraph();
void GP_DrawOverlay();
void GP_RunGraphics();



#endif /* GRAPHICSPROCESSING_H_ */
