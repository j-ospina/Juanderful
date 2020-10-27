/*
 * i2cDriver.h
 *
 *  Created on: Jan 21, 2020
 *      Author: jdosp
 */

#ifndef PARTB_LEDDRIVER_H_
#define PARTB_LEDDRIVER_H_

#include "msp.h"

typedef enum uint8_t
{
    LED_BLUE = 0x60,
    LED_GREEN = 0x61,
    LED_RED = 0x62,
}led_unit_t;

typedef enum
{
    LED_OFF = 0x0000,
    LED_ON = 0xFFFF,
    LED_0 = 0x0001,
    LED_1 = 0x0002,
    LED_2 = 0x0004,
    LED_3 = 0x0008,
    LED_4 = 0x0010,
    LED_5 = 0x0020,
    LED_6 = 0x0040,
    LED_7 = 0x0080,
    LED_8 = 0x0100,
    LED_9 = 0x0200,
    LED_10 = 0x0400,
    LED_11 = 0x0800,
    LED_12 = 0x1000,
    LED_13 = 0x2000,
    LED_14 = 0x4000,
    LED_15 = 0x8000,
}led_mode_t;

typedef enum
{
    LED_0_3_REG = 0x06,
    LED_4_7_REG = 0x07,
    LED_8_11_REG = 0x08,
    LED_12_15_REG = 0x09,
    LED_REG_AUTO_INC = 0x10,
}led_address_t;

typedef uint16_t led_array_status_t;

//Initialize LED drivers
void ledInit();

//Set frequency and pwm duty cycle
void ledColorSet(uint32_t unit, uint32_t pwmData);

//Sets LEDs on or off based on
void ledModeSet(led_unit_t unit, led_array_status_t ledArrayStatus);

#endif /* PARTB_LEDDRIVER_H_ */
