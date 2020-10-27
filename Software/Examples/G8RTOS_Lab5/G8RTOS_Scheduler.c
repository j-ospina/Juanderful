/*
 * G8RTOS_Scheduler.c
 */

/*********************************************** Dependencies and Externs *************************************************************/

#include <G8RTOS_Lab5/G8RTOS.h>
#include <G8RTOS_Lab5/G8RTOS_Scheduler.h>
#include <stdint.h>
#include "msp.h"
#include <stdlib.h>
#include <string.h>


/*
 * G8RTOS_Start exists in asm
 */
extern void G8RTOS_Start();

/* System Core Clock From system_msp432p401r.c */
extern uint32_t SystemCoreClock;

/*
 * Pointer to the currently running Thread Control Block
 */
extern tcb_t * CurrentlyRunningThread;

/*********************************************** Dependencies and Externs *************************************************************/


/*********************************************** Defines ******************************************************************************/

/* Status Register with the Thumb-bit Set */
#define THUMBBIT 0x01000000
#define ICSR (*((volatile unsigned int*)(0xe000ed04)))   //Address of the Interrupt Control State Register
#define ICSR_PENDSVSET (1 << 28)     //Bit to set pending PendSV
#define UINT8MAX 255
/*********************************************** Defines ******************************************************************************/


/*********************************************** Data Structures Used *****************************************************************/

/* Thread Control Blocks
 *	- An array of thread control blocks to hold pertinent information for each thread
 */
static tcb_t threadControlBlocks[MAX_THREADS];

/* Thread Stacks
 *	- An array of arrays that will act as invdividual stacks for each thread
 */
static int32_t threadStacks[MAX_THREADS][STACKSIZE];

/* Periodic Event Threads
 * - An array of periodic events to hold pertinent information for each thread
 */
static ptcb_t Pthread[MAXPTHREADS];

/*********************************************** Data Structures Used *****************************************************************/


/*********************************************** Private Variables ********************************************************************/

/*
 * Current Number of Threads currently in the scheduler
 */
static uint32_t NumberOfThreads;

/*
 * Current Number of Periodic Threads currently in the scheduler
 */
static uint32_t NumberOfPthreads;

/*
 *
 */
static uint16_t IDCounter;

/*********************************************** Private Variables ********************************************************************/


/*********************************************** Private Functions ********************************************************************/

/*
 * Initializes the Systick and Systick Interrupt
 * The Systick interrupt will be responsible for starting a context switch between threads
 * Param "numCycles": Number of cycles for each systick interrupt
 */
static void InitSysTick(uint32_t numCycles)
{
    SysTick_Config(numCycles); //This will give the exact same number of ticks as the clock frequency giving 1ms
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
}

/*
 * Chooses the next thread to run.
 * Lab 2 Scheduling Algorithm:
 * 	- Simple Round Robin: Choose the next running thread by selecting the currently running thread's next pointer
 * 	- Check for sleeping and blocked threads
 */
void G8RTOS_Scheduler()
{
    uint8_t nextThreadPriority = UINT8_MAX;
    tcb_t * tempNextThread;

    tempNextThread = CurrentlyRunningThread->nextTCB;

    for(int i=0; i<NumberOfThreads; i++){
        //Check for thread that is awake and not blocked
        if((!tempNextThread->Asleep) && (!tempNextThread->Blocked)){

            //Check if priority is higher than current max
            if(tempNextThread->Priority < nextThreadPriority){

                //Set current thread to next thread to run
                CurrentlyRunningThread = tempNextThread;
                nextThreadPriority = CurrentlyRunningThread->Priority;
            }
        }
        tempNextThread = tempNextThread->nextTCB;
    }
}


/*
 * SysTick Handler
 * The Systick Handler now will increment the system time,
 * set the PendSV flag to start the scheduler,
 * and be responsible for handling sleeping and periodic threads
 */
void SysTick_Handler()
{
    tcb_t *pt;
    ptcb_t *Pptr;

    SystemTime++;   //Increment System time

    Pptr = &Pthread[0];

    for(int i=0; i<NumberOfPthreads; i++){
        if(Pptr->Execute_Time == SystemTime){
            Pptr->Execute_Time = Pptr->Period + SystemTime;
            Pptr->Handler();
        }
        Pptr = Pptr->next_P_event;
    }

    pt = CurrentlyRunningThread;

    for(int i=0; i<NumberOfThreads; i++){
        if(pt->Asleep == true){
            if(pt->sleepCount == SystemTime){
                pt->Asleep = false;
                pt->sleepCount = 0;
            }
        }
        pt = pt->nextTCB;
    }

    StartContextSwitch();

}

/*********************************************** Private Functions ********************************************************************/


/*********************************************** Public Variables *********************************************************************/

/* Holds the current time for the whole System */
uint32_t SystemTime;

/*********************************************** Public Variables *********************************************************************/


/*********************************************** Public Functions *********************************************************************/

/*
 * Sets variables to an initial state (system time and number of threads)
 * Enables board for highest speed clock and disables watchdog
 */
void G8RTOS_Init()
{
    uint32_t newVTORTable = 0x20000000;
    memcpy((uint32_t *)newVTORTable, (uint32_t *)SCB->VTOR, 57*4);
    // 57 interrupt vectors to copy
    SCB->VTOR = newVTORTable;

    SysTick->CTRL = 0;      //Disable SysTick during setup
    SysTick->VAL = 0;       //Initialize system time to zero
    SystemTime = 0;         //Set System time to zero
    NumberOfThreads = 0;    //Set number of threads to zero
    BSP_InitBoard();        //Initialize all hardware on board
}

/*
 * Starts G8RTOS Scheduler
 * 	- Initializes the Systick
 * 	- Sets Context to first thread
 * Returns: Error Code for starting scheduler. This will only return if the scheduler fails
 */
int G8RTOS_Launch()
{
    tcb_t *tempNextThread;
    uint8_t nextThreadPriority = UINT8MAX;

    uint32_t numCycles = ClockSys_GetSysFreq();
    numCycles = numCycles/1000;     //3000 for 1 ms

    CurrentlyRunningThread = &threadControlBlocks[0];   //Set current thread to first TCB

    tempNextThread = CurrentlyRunningThread->nextTCB;

    //Set the first thread to the thread with the highest priority
    for(int i=0; i<NumberOfThreads; i++){
        //Check if priority is higher than current max
        if(tempNextThread->Priority < nextThreadPriority){

            //Set current thread to next thread to run
            CurrentlyRunningThread = tempNextThread;
            nextThreadPriority = CurrentlyRunningThread->Priority;
        }
        tempNextThread = tempNextThread->nextTCB;
    }

    InitSysTick(numCycles);  //Set for 1ms

    __NVIC_EnableIRQ(PendSV_IRQn);
    __NVIC_SetPriority(PendSV_IRQn, 7);                 //Set PendSV as lowest priority
    __NVIC_EnableIRQ(SysTick_IRQn);
    __NVIC_SetPriority(SysTick_IRQn, 6);                //Set SysTick at a low priority

    G8RTOS_Start();

    return 1;
}


/*
 * Adds threads to G8RTOS Scheduler
 * 	- Checks if there are stil available threads to insert to scheduler
 * 	- Initializes the thread control block for the provided thread
 * 	- Initializes the stack for the provided thread to hold a "fake context"
 * 	- Sets stack tcb stack pointer to top of thread stack
 * 	- Sets up the next and previous tcb pointers in a round robin fashion
 * Param "threadToAdd": Void-Void Function to add as preemptable main thread
 * Returns: Error code for adding threads
 */
int G8RTOS_AddThread(void (*threadToAdd)(void), uint8_t priority, char * name)
{
    int32_t primask;

    primask = StartCriticalSection();   //Interrupts disabled

    tcb_t *tempThread;

    //First check if there is available space to add thread
    if(NumberOfThreads < MAX_THREADS){
        //Check if there is a dead thread we can replace
        //Check the entire array because a dead thread is no longer a part of the linked list
        for(int i=0; i<MAX_THREADS; i++){

            tempThread = &threadControlBlocks[i];
            //check if thread is dead
            if(!tempThread->isAlive && tempThread->stackPointer != 0){
                //if dead than at position 'i' is the thread to replace

                threadControlBlocks[i].Asleep = false;                    //Intialize thread to be awake
                threadControlBlocks[i].Blocked = 0;                    //Initialize thread to not be blocked
                threadControlBlocks[i].Priority = priority;               //Initialized thread as priority given
                threadControlBlocks[i].isAlive = true;                    //Initialize thread to be alive              //Initialize thread name
                threadControlBlocks[i].ThreadID = ((IDCounter++)<< 16);   //Initialize thread ID

                for(int j = 0; j<MAX_NAME_LENGTH; j++){
                    if(*name == '\0'){
                        break;
                    }
                    threadControlBlocks[i].Threadname[j] = *name;
                    name++;
                }

                threadStacks[i][STACKSIZE-1] = (0 | THUMBBIT);         //Initialize psr with THUMBBIT
                threadStacks[i][STACKSIZE-2] = (int32_t)(threadToAdd); //Set PC to address of thread

                for(int j=3; j<17; j++){    //Initialize dummy values for r0-r15
                    threadStacks[i][STACKSIZE-j] = 3;
                }

                threadControlBlocks[i].stackPointer = &threadStacks[i][STACKSIZE-16];

                //Update thread pointers
                threadControlBlocks[i].prevTCB->nextTCB = &threadControlBlocks[i];
                threadControlBlocks[i].nextTCB->prevTCB = &threadControlBlocks[i];

                NumberOfThreads++;
                EndCriticalSection(primask);
                return 1;
            }
        }

        //Otherwise if there are no dead threads then continue adding thread the original way
        tcb_t *block = (tcb_t*)malloc(sizeof(*threadToAdd));

        threadControlBlocks[NumberOfThreads]  = *block;
        threadControlBlocks[NumberOfThreads].Asleep = false;                    //Intialize thread to be awake
        threadControlBlocks[NumberOfThreads].Blocked = 0;                       //Initialize thread to not be blocked
        threadControlBlocks[NumberOfThreads].Priority = priority;               //Initialized thread as priority given
        threadControlBlocks[NumberOfThreads].isAlive = true;                    //Initialize thread to be alive              //Initialize thread name
        threadControlBlocks[NumberOfThreads].ThreadID = ((IDCounter++)<< 16);   //Initialize thread ID

        for(int k = 0; k<MAX_NAME_LENGTH; k++){
            if(*name == '\0'){
                break;
            }
            threadControlBlocks[NumberOfThreads].Threadname[k] = *name;
            name++;
        }

        threadStacks[NumberOfThreads][STACKSIZE-1] = (0 | THUMBBIT);         //Initialize psr with THUMBBIT
        threadStacks[NumberOfThreads][STACKSIZE-2] = (int32_t)(threadToAdd); //Set PC to address of thread

        for(int i=3; i<17; i++){    //Initialize dummy values for r0-r15
            threadStacks[NumberOfThreads][STACKSIZE-i] = 3;
        }

        threadControlBlocks[NumberOfThreads].stackPointer = &threadStacks[NumberOfThreads][STACKSIZE-16];

        if(NumberOfThreads == 0){//Only one Thread Control Block therefore both prev and next look at itself
            threadControlBlocks[NumberOfThreads].prevTCB = &threadControlBlocks[NumberOfThreads];
            threadControlBlocks[NumberOfThreads].nextTCB = &threadControlBlocks[NumberOfThreads];
        }
        else{
            threadControlBlocks[0].prevTCB = &threadControlBlocks[NumberOfThreads];
            threadControlBlocks[NumberOfThreads-1].nextTCB = &threadControlBlocks[NumberOfThreads];

            threadControlBlocks[NumberOfThreads].prevTCB = &threadControlBlocks[NumberOfThreads-1];
            threadControlBlocks[NumberOfThreads].nextTCB = &threadControlBlocks[0];
        }

        NumberOfThreads++;

        EndCriticalSection(primask);
        return 1;
    }
    else {
        EndCriticalSection(primask);
        return -1;  //If thread could not be added then return ERROR CODE
    }

}


/*
 * Adds periodic threads to G8RTOS Scheduler
 * Function will initialize a periodic event struct to represent event.
 * The struct will be added to a linked list of periodic events
 * Param Pthread To Add: void-void function for P thread handler
 * Param period: period of P thread to add
 * Returns: Error code for adding threads
 */
int G8RTOS_AddPeriodicEvent(void (*PthreadToAdd)(void), uint32_t period)
{

    int32_t primask = StartCriticalSection();

    if (NumberOfPthreads < MAXPTHREADS){
       Pthread[NumberOfPthreads].Handler = PthreadToAdd;
       Pthread[NumberOfPthreads].Period = period;
       Pthread[NumberOfPthreads].Execute_Time = period + SystemTime;

       if(NumberOfPthreads == 0){//Only one Thread Control Block therefore both prev and next look at itself
           Pthread[NumberOfPthreads].prev_P_event = &Pthread[NumberOfPthreads];
           Pthread[NumberOfPthreads].next_P_event = &Pthread[NumberOfPthreads];
       }
       else{
           Pthread[0].prev_P_event = &Pthread[NumberOfPthreads];
           Pthread[NumberOfPthreads-1].next_P_event = &Pthread[NumberOfPthreads];

           Pthread[NumberOfPthreads].prev_P_event = &Pthread[NumberOfPthreads-1];
           Pthread[NumberOfPthreads].next_P_event = &Pthread[0];
       }

       NumberOfPthreads++;
       EndCriticalSection(primask);
       return 1;
   }
   else{
       EndCriticalSection(primask);
       return 0;
   }
}


/*
 * Starts a context Switch
 */
void StartContextSwitch(){
    ICSR |= ICSR_PENDSVSET; //Set the pending PendSV bit This starts context switch
}



/*
 * Puts the current thread into a sleep state.
 *  param durationMS: Duration of sleep time in ms
 */
void sleep(uint32_t durationMS)
{
    /* Implement this */
    CurrentlyRunningThread->sleepCount = durationMS + SystemTime;
    CurrentlyRunningThread->Asleep = true;
    StartContextSwitch();

}


/*
 * Returns the CurrentlyRunningThreads thread ID
 */
uint32_t G8RTOS_GetThreadID(){
    return CurrentlyRunningThread->ThreadID;
}


/*
 * Thread will kill itself
 */
sched_ErrCode_t G8RTOS_KillSelf(){
    int32_t primask;

    primask = StartCriticalSection();       //Interrupts disabled

    if(NumberOfThreads == 1){
        return CANNOT_KILL_LAST_THREAD;
    }

    CurrentlyRunningThread->isAlive = false;   //Kill currently running thread;

    //update thread pointers
    CurrentlyRunningThread->nextTCB->prevTCB = CurrentlyRunningThread->prevTCB;
    CurrentlyRunningThread->prevTCB->nextTCB = CurrentlyRunningThread->nextTCB;

    StartContextSwitch();

    NumberOfThreads--;
    EndCriticalSection(primask);
    return NO_ERROR;
}


/*
 * Function to kill a certain thread
 */
sched_ErrCode_t G8RTOS_KillThread(uint32_t threadID){
    int32_t primask;
    tcb_t * tempthread;


    primask = StartCriticalSection();       //Interrupts disabled
    tempthread = CurrentlyRunningThread;   //Set thread to current tcb in linked list

    if(NumberOfThreads == 1){
        //Return error code if only one thread is running
        return CANNOT_KILL_LAST_THREAD;
    }

    //Search for thread with the same threadID
    for(int i=0; i<NumberOfThreads; i++){
        if(tempthread->ThreadID == threadID){
            tempthread->isAlive = false;

            //update thread pointers
            tempthread->nextTCB->prevTCB = tempthread->prevTCB;
            tempthread->prevTCB->nextTCB = tempthread->nextTCB;

            NumberOfThreads--;           //decrement number of threads
            EndCriticalSection(primask); //Once found end critical section

            //If current thread is being killed, context switch once critical section ends
            if(tempthread == CurrentlyRunningThread){
                StartContextSwitch();
            }

            return NO_ERROR;
        }
        //Continue and check the next TCB
        tempthread = tempthread->nextTCB;
    }

    //If thread was not found return ERROR CODE
    EndCriticalSection(primask);

    return THREAD_DOES_NOT_EXIST;
}


/*
 * Kill All threads
 */
sched_ErrCode_t G8RTOS_KillAllThreads(){
    for(int i=0; i<NumberOfThreads; i++){
        G8RTOS_KillThread(threadControlBlocks[i].ThreadID);
    }
}



/*
 * Aperiodic event thread
 */
sched_ErrCode_t G8RTOS_AddAPeriodicEvent(void (*AthreadToAdd)(void), uint8_t priority, IRQn_Type IRQn){

//    int32_t primask;
//    primask = StartCriticalSection();

//    uint32_t* address = (uint32_t*)malloc(sizeof(*AthreadToAdd));
    //Highest allowable priority is 0 and lowest priority is 40 in vector
    //Verify the IRQn is less than the last exception (PSS_IRQn)
    //and greater than last acceptable user IRQn (PORT6_IRQn)
    if(IRQn > PSS_IRQn && IRQn < PORT6_IRQn){

        if(priority > 6){
//            EndCriticalSection(primask);
            return HWI_PRIORITY_INVALID;
        }

        __NVIC_SetVector(IRQn, (uint32_t)AthreadToAdd);
        __NVIC_SetPriority(IRQn, priority);
        __NVIC_EnableIRQ(IRQn);

//        EndCriticalSection(primask);
        return NO_ERROR;
    }
    else{
//        EndCriticalSection(primask);
        return IRQn_INVALID;
    }

}



/*********************************************** Public Functions *********************************************************************/
