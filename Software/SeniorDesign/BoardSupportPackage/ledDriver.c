#include "ledDriver.h"

static void generateLedData(uint8_t *dataArray, led_array_status_t status)
{
    uint32_t data = 0;
    uint8_t count = 0;
    led_array_status_t mask = 0x8000;

    for(count = 0; count < 8*sizeof(led_array_status_t); count++)
    {
        if(status & mask)
        {
            data |= (1 << count*2);
        }
        else
        {
            data &= ~(1 << count*2);
        }

        mask = mask >> 1;
    }

    for(count = 0; count < sizeof(uint32_t); count++)
    {
        dataArray[count] = (data & 0xFF);
        data = data >> 8;
    }
}

void ledInit()
{
    //Set UCSWRST
    UCB2CTLW0 = UCSWRST;

    //Init eUSCI_B registers
    //Master, i2c Mode, Clock Sync, SMCLK, Transmitter
    UCB2CTLW0 |= (UCMST | UCMODE_3 | UCSYNC | UCSSEL__SMCLK | UCTR);

    //Fclk to 400kHz
    UCB2BRW = 30;

    //Configure ports
        //Pin 6 and 7 P3SEL0 = 1
    P3SEL0 |= (BIT6 | BIT7);
        //Pin 6 and 7 P3SEL1 = 0
    P3SEL1 &= ~(BIT6 | BIT7);

    //Clear UCSWRST
    UCB2CTLW0 &= ~UCSWRST;

    ledModeSet(LED_GREEN, LED_OFF);
    ledModeSet(LED_BLUE, LED_OFF);
    ledModeSet(LED_RED, LED_OFF);
}

void ledModeSet(led_unit_t unit, led_array_status_t ledArrayStatus)
{
    uint8_t ledSendData[4] = {0,0,0,0};
    generateLedData(ledSendData, ledArrayStatus);

    UCB2I2CSA = unit; //Address of LED Driver

    //while(UCB2CTLW0 & UCTXSTP);//Check the stop is not being transmitted
    UCB2CTLW0 |= UCTXSTT;// Set start condition

    while(!(UCB2IFG & UCTXIFG0));
    //Transmit to first LED mode address
    //Auto increment register address after each byte transmits
    UCB2TXBUF = (LED_0_3_REG | LED_REG_AUTO_INC);


    //Transmit to all 4 LED registers + 1
    uint8_t count;
    for(count = 0; count < 5; count++)
    {
        while(!(UCB2IFG & UCTXIFG0));
        UCB2TXBUF = ledSendData[count];
    }

    //Generate STOP
    UCB2CTLW0 |= UCTXSTP;
    while((UCB2IFG & UCTXSTP));
}
