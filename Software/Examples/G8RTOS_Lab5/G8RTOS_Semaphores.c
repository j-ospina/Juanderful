/*
 * G8RTOS_Semaphores.c
 */

/*********************************************** Dependencies and Externs *************************************************************/

#include <G8RTOS_Lab5/G8RTOS.h>
#include <G8RTOS_Lab5/G8RTOS_CriticalSection.h>
#include <G8RTOS_Lab5/G8RTOS_Semaphores.h>
#include <stdint.h>
#include "msp.h"

#define ICSR (*((volatile unsigned int*)(0xe000ed04)))   //Address of the Interrupt Control State Register
#define ICSR_PENDSVSET (1 << 28)     //Bit to set pending PendSV
/*********************************************** Dependencies and Externs *************************************************************/


/*********************************************** Public Functions *********************************************************************/

/*
 * Initializes a semaphore to a given value
 * Param "s": Pointer to semaphore
 * Param "value": Value to initialize semaphore to
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_InitSemaphore(semaphore_t *s, int32_t value)
{
    int32_t primask;

    primask = StartCriticalSection();   //Interrupts disabled
    *s = value;
    EndCriticalSection(primask);        //Enable interrupts restore PRIMASK state
}

/*
 * No longer waits for semaphore
 *  - Decrements semaphore
 *  - Blocks thread is sempahore is unavalible
 * Param "s": Pointer to semaphore to wait on
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_WaitSemaphore(semaphore_t *s)
{
    int32_t primask;

    primask = StartCriticalSection();   //Interrupts disabled
    *s = (*s) - 1;
    if((*s) < 0){
        CurrentlyRunningThread->Blocked = s;    //Reason it is blocked
//        EndCriticalSection(primask);            //Enable Interrupts

        ICSR |= ICSR_PENDSVSET;                 //Starts Context Switch
    }

    EndCriticalSection(primask);
}

/*
 * Signals the completion of the usage of a semaphore
 *  - Increments the semaphore value by 1
 *  - Unblocks any threads waiting on that semaphore
 * Param "s": Pointer to semaphore to be signaled
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_SignalSemaphore(semaphore_t *s)
{
    int32_t primask;
    tcb_t *pt;

    primask = StartCriticalSection();   //Interrupts disabled
    *s = (*s) + 1;
    if((*s) <= 0){
        pt = CurrentlyRunningThread->nextTCB; //Search for blocked TCB
        while(pt->Blocked != s){
            pt = pt->nextTCB;
        }
        pt->Blocked = 0; //Wake this thread up
    }

    EndCriticalSection(primask);    //Enable Interrupts
}

/*********************************************** Public Functions *********************************************************************/


