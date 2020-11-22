/*
 * DebugTools.c
 *
 *  Created on: Nov 16, 2020
 *      Author: jdosp
 */

#include "DebugTools.h"


void inline DebugToolsInit(){
    P2->DIR = BIT7;
    P2->SEL0 = 0;
    P2->SEL1 = 0;
}

void inline DebugToolsTgl(){
    P2->OUT = BIT7;
    P2->OUT = 0;
}
void inline DebugToolsUp(){
    P2->OUT = BIT7;
}
void inline DebugToolsDown(){
    P2->OUT = 0;
}

void DebugToolsCount(int count){
    if (DebugToolsCountVar >= count) {
        DebugToolsCountVar = 0;
        DebugToolsCountDone = 1;
    }
    else{
        DebugToolsCountVar++;
    }
}
