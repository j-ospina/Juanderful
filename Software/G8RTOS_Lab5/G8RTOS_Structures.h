/*
 * G8RTOS_Structure.h
 *
 *  Created on: Jan 12, 2017
 *      Author: Raz Aloni
 */

#ifndef G8RTOS_STRUCTURES_H_
#define G8RTOS_STRUCTURES_H_

#include <G8RTOS_Lab5/G8RTOS.h>
#include "msp.h"
#include <stdbool.h>

#define MAX_NAME_LENGTH 16
/*********************************************** Data Structure Definitions ***********************************************************/

/*
 *  Thread Control Block:
 *      - Every thread has a Thread Control Block
 *      - The Thread Control Block holds information about the Thread Such as the Stack Pointer, Priority Level, and Blocked Status
 *      - For Lab 2 the TCB will only hold the Stack Pointer, next TCB and the previous TCB (for Round Robin Scheduling)
 */

/* Create tcb struct here */
typedef struct tcb_t{
    int32_t *stackPointer;
    struct tcb_t *nextTCB;
    struct tcb_t *prevTCB;
    semaphore_t *Blocked;
    uint32_t sleepCount;
    bool Asleep;
    uint8_t Priority;
    bool isAlive;
    char Threadname[MAX_NAME_LENGTH];
    uint32_t ThreadID;
}tcb_t;

/*
 *  Periodic Thread Control Block:
 *      - Holds a function pointer that points to the periodic thread to be executed
 *      - Has a period in us
 *      - Holds Current time
 *      - Contains pointer to the next periodic event - linked list
 */

/* Create periodic thread struct here */
typedef struct ptcb_t{
    void(*Handler)(void);
    uint32_t Period;
    uint32_t Execute_Time;
    uint32_t Current_Time;
    struct ptcb_t * prev_P_event;
    struct ptcb_t * next_P_event;
}ptcb_t;

/*********************************************** Data Structure Definitions ***********************************************************/


/*********************************************** Public Variables *********************************************************************/

tcb_t * CurrentlyRunningThread;

/*********************************************** Public Variables *********************************************************************/




#endif /* G8RTOS_STRUCTURES_H_ */
