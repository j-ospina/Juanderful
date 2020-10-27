/*
 * G8RTOS_IPC.c
 *
 *  Created on: Jan 10, 2017
 *      Author: Daniel Gonzalez
 */
#include <G8RTOS_Lab5/G8RTOS.h>
#include <G8RTOS_Lab5/G8RTOS_IPC.h>
#include <G8RTOS_Lab5/G8RTOS_Semaphores.h>
#include <stdint.h>
#include "msp.h"

/*********************************************** Defines ******************************************************************************/

#define FIFOSIZE 16
#define MAX_NUMBER_OF_FIFOS 4

/*********************************************** Defines ******************************************************************************/

////BITBAND_PERI(Px->OUT, n)

/*********************************************** Data Structures Used *****************************************************************/

/*
 * FIFO struct will hold
 *  - buffer
 *  - head
 *  - tail
 *  - lost data
 *  - current size
 *  - mutex
 */

/* Create FIFO struct here */
typedef struct FIFO_t{
    int32_t Buffer[FIFOSIZE];
    int32_t *Head;
    int32_t *Tail;
    uint32_t LostData;
    semaphore_t CurrentSize;
    semaphore_t Mutex;
}FIFO_t;

/* Array of FIFOS */
static FIFO_t FIFOs[4];


/*********************************************** Data Structures Used *****************************************************************/

/*
 * Initializes FIFO Struct
 */
int G8RTOS_InitFIFO(uint32_t FIFOIndex)
{

    if(FIFOIndex >= MAX_NUMBER_OF_FIFOS){
        return -1;
    }

    for(int i=0;i<FIFOSIZE; i++){   //Initialize buffer values to be 0 initially
        FIFOs[FIFOIndex].Buffer[i] = 0;
    }

    FIFOs[FIFOIndex].Head = &FIFOs[FIFOIndex].Buffer[0];    //Set Head and Tail pointers to first
    FIFOs[FIFOIndex].Tail = &FIFOs[FIFOIndex].Buffer[0];    //position in the FIFO


    G8RTOS_InitSemaphore(&FIFOs[FIFOIndex].CurrentSize, 0);
    G8RTOS_InitSemaphore(&FIFOs[FIFOIndex].Mutex, 1);

    FIFOs[FIFOIndex].LostData = 0;

    return 1;
}

/*
 * Reads FIFO
 *  - Waits until CurrentSize semaphore is greater than zero
 *  - Gets data and increments the head ptr (wraps if necessary)
 * Param: "FIFOChoice": chooses which buffer we want to read from
 * Returns: uint32_t Data from FIFO
 */
uint32_t readFIFO(uint32_t FIFOChoice)
{
    uint32_t data = 0;

    if(FIFOs[FIFOChoice].CurrentSize == 0){
        G8RTOS_WaitSemaphore(&FIFOs[FIFOChoice].CurrentSize);
    }
    else{
        G8RTOS_WaitSemaphore(&FIFOs[FIFOChoice].CurrentSize);
    }
//    G8RTOS_WaitSemaphore(&FIFOs[FIFOChoice].CurrentSize);
    G8RTOS_WaitSemaphore(&FIFOs[FIFOChoice].Mutex);

    data = *(FIFOs[FIFOChoice].Head);     //Read data from head of FIFO
    FIFOs[FIFOChoice].Head++;           //Increment next position in buffer


    if(FIFOs[FIFOChoice].Head == &FIFOs[FIFOChoice].Buffer[FIFOSIZE]){
        FIFOs[FIFOChoice].Head = &FIFOs[FIFOChoice].Buffer[0]; //Wrap to first one
    }

    G8RTOS_SignalSemaphore(&FIFOs[FIFOChoice].Mutex);           //Release semaphore
    //G8RTOS_SignalSemaphore(&FIFOs[FIFOChoice].CurrentSize);

    return data;
}

/*
 * Writes to FIFO
 *  Writes data to Tail of the buffer if the buffer is not full
 *  Increments tail (wraps if ncessary)
 *  Param "FIFOChoice": chooses which buffer we want to read from
 *        "Data': Data being put into FIFO
 *  Returns: error code for full buffer if unable to write
 */
int writeFIFO(uint32_t FIFOChoice, uint32_t Data)
{
    //G8RTOS_WaitSemaphore(&FIFOs[FIFOChoice].CurrentSize); //Check if full
    if(FIFOs[FIFOChoice].CurrentSize == FIFOSIZE){
        FIFOs[FIFOChoice].LostData++;   //Error in fifo
        return -1;
    }

    *FIFOs[FIFOChoice].Tail = Data;     //Write data;
    FIFOs[FIFOChoice].Tail++;      //Increment to next position in fifo

    if(FIFOs[FIFOChoice].Tail == &FIFOs[FIFOChoice].Buffer[FIFOSIZE]){
        FIFOs[FIFOChoice].Tail = &FIFOs[FIFOChoice].Buffer[0];   //Wrap
    }

    G8RTOS_SignalSemaphore(&FIFOs[FIFOChoice].CurrentSize);
    return 0;
}

