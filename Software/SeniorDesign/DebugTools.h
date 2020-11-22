/*
 * DebugTools.h
 *
 *  Created on: Nov 16, 2020
 *      Author: jdosp
 */

#ifndef DEBUGTOOLS_H_
#define DEBUGTOOLS_H_

#include "msp.h"

int DebugToolsCountDone;
int DebugToolsCountVar;
void inline DebugToolsInit();
void inline DebugToolsUp();
void inline DebugToolsDown();
void inline DebugToolsTgl();
void DebugToolsCount(int count);

#endif /* DEBUGTOOLS_H_ */
