#include <ClockSys.h>
#include "msp.h"
#include "driverlib.h"
#include "LCD_Drivers.h"


/**
 * main.c
 */
void main(void)
{
    /* Disable Watchdog */
    WDT_A_clearTimer();
    WDT_A_holdTimer();

    /* Initialize Clock */
    ClockSys_SetMaxFreq();


    LCD_Init();

    while(1);

}
