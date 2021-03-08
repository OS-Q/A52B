/*
 Arduino.h - Main include file for the Arduino SDK
 Copyright (c) 2005-2013 Arduino Team.  All right reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef MAIN_ESP32_HAL_GPIO_H_
#define MAIN_ESP32_HAL_GPIO_H_

#include "variant.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "esp32-hal.h"

#define LOW               0x0
#define HIGH              0x1

//GPIO FUNCTIONS
#define INPUT             0x01
#define OUTPUT            0x02
#define PULLUP            0x04
#define INPUT_PULLUP      0x05
#define PULLDOWN          0x08
#define INPUT_PULLDOWN    0x09
#define OPEN_DRAIN        0x10
#define OUTPUT_OPEN_DRAIN 0x12
#define SPECIAL           0xF0
#define FUNCTION_1        0x00
#define FUNCTION_2        0x20
#define FUNCTION_3        0x40
#define FUNCTION_4        0x60
#define FUNCTION_5        0x80
#define FUNCTION_6        0xA0
#define ANALOG            0xC0

//Interrupt Modes
#define DISABLED  0x00
#define RISING    0x01
#define FALLING   0x02
#define CHANGE    0x03
#define ONLOW     0x04
#define ONHIGH    0x05
#define ONLOW_WE  0x0C
#define ONHIGH_WE 0x0D

#define PIN_MODE		0
#define WRITE_DIGITAL	1
#define WRITE_TOGGLE    2
#define WRITE_ANALOG	3
#define WRITE_ANALOG_F	4
#define READ_DIGITAL	5
#define READ_ANALOG		6
#define SET_INTERRUPT	7
#define UNSET_INTERRUPT	8

typedef void (*voidFuncPtr)(void);
typedef void (*voidFuncPtrArg)(void*);

typedef struct {
    uint8_t reg;      /*!< GPIO register offset from DR_REG_IO_MUX_BASE */
    int8_t rtc;       /*!< RTC GPIO number (-1 if not RTC GPIO pin) */
    int8_t adc;       /*!< ADC Channel number (-1 if not ADC pin) */
    int8_t touch;     /*!< Touch Channel number (-1 if not Touch pin) */
} esp32_gpioMux_t;

extern const esp32_gpioMux_t esp32_gpioMux[40];
extern const int8_t esp32_adc2gpio[20];

#define digitalPinIsValid(pin)          ((g_APinDescription[pin].ulPin) < 40 && esp32_gpioMux[(g_APinDescription[pin].ulPin)].reg)
#define digitalPinCanOutput(pin)        ((g_APinDescription[pin].ulPin) < 34 && esp32_gpioMux[(g_APinDescription[pin].ulPin)].reg)
#define digitalPinToRtcPin(pin)         (((g_APinDescription[pin].ulPin) < 40)?esp32_gpioMux[(g_APinDescription[pin].ulPin)].rtc:-1)
#define digitalPinToAnalogChannel(pin)  (((g_APinDescription[pin].ulPin) < 40)?esp32_gpioMux[(g_APinDescription[pin].ulPin)].adc:-1)
#define digitalPinToTouchChannel(pin)   (((g_APinDescription[pin].ulPin) < 40)?esp32_gpioMux[(g_APinDescription[pin].ulPin)].touch:-1)
#define digitalPinToDacChannel(pin)     (((g_APinDescription[pin].ulPin) == 25)?0:((g_APinDescription[pin].ulPin) == 26)?1:-1)

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);

void attachInterrupt(uint8_t pin, void (*)(void), int mode);
void attachInterruptArg(uint8_t pin, void (*)(void*), void * arg, int mode);
void detachInterrupt(uint8_t pin);
void digitalToggle(uint8_t pin);

void disableGPIOInterrupt();
void enableGPIOInterrupt();

#ifdef __cplusplus
}
#endif

#endif /* MAIN_ESP32_HAL_GPIO_H_ */
