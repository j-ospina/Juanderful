#ifndef GRAPHICSPROCESSING_H_
#define GRAPHICSPROCESSING_H_

#include <stdlib.h>
#include <stdint.h>
#include <LCD_Drivers.h>

#define GP_GRAPH_MAX_X MAX_SCREEN_X
#define GP_GRAPH_MAX_Y MAX_SCREEN_Y
#define GP_GRAPH_MIN_X MIN_SCREEN_X
#define GP_GRAPH_MIN_Y MIN_SCREEN_Y
#define GP_GRAPH_WIDTH GP_GRAPH_MAX_X - GP_GRAPH_MIN_X
#define GP_GRAPH_HEIGHT GP_GRAPH_MAX_Y - GP_GRAPH_MIN_Y

typedef uint16_t Graph_t[GP_GRAPH_WIDTH];

Graph_t *GraphArrayStart;

void GP_InstantiateGraph();
void GP_DrawGraphBackground();
void GP_DrawGraph();
void GP_DrawOverlay();



#endif /* GRAPHICSPROCESSING_H_ */
