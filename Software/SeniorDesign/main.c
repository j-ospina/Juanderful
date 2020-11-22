#include "msp.h"
#include "driverlib.h"
#include "ADC_Drivers.h"
#include "LCD_Drivers.h"
#include "GraphicsProcessing.h"
#include "Oscilloscope.h"
#include "DebugTools.h"


/**
 * main.c
 */

static void ClockInit(){
    PJ->SEL0 |= GPIO_PIN3 | GPIO_PIN2;
    PJ->SEL1 &= ~(GPIO_PIN3 | GPIO_PIN2);

    while(!PCM_setCoreVoltageLevel(PCM_VCORE1));
    CS_setExternalClockSourceFrequency(32000, 48000000);

    FlashCtl_setWaitState(FLASH_BANK0, 2);
    FlashCtl_setWaitState(FLASH_BANK1, 2);

    FLCTL->BANK0_RDCTL |= (FLCTL_BANK0_RDCTL_BUFI | FLCTL_BANK0_RDCTL_BUFD );
    FLCTL->BANK1_RDCTL |= (FLCTL_BANK1_RDCTL_BUFI | FLCTL_BANK1_RDCTL_BUFD );

    CS_startHFXT(0);
    CS_initClockSignal(CS_MCLK, CS_HFXTCLK_SELECT, CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_HSMCLK, CS_HFXTCLK_SELECT, CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_SMCLK, CS_HFXTCLK_SELECT, CS_CLOCK_DIVIDER_4);
}

void main(void)
{
    /* Disable Watchdog */
    WDT_A_clearTimer();
    WDT_A_holdTimer();

    DebugToolsInit();

    /* Initialize Clock */
    ClockInit();
    LCD_Init();
    Osc_OscilloscopeInit();
    ADC_Init(ADC_WithDma);
    ADC_Start();

    while(1){
        GP_RunGraphics();
    }
}
