/*
  Copyright (c) 2014 Arduino.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _WIRING_CONSTANTS_
#define _WIRING_CONSTANTS_

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

#define LOW             (0x0)
#define HIGH            (0x1)

/*
#define INPUT           (0x0)
#define OUTPUT          (0x1)
#define INPUT_PULLUP    (0x2)
#define INPUT_PULLDOWN  (0x3)
*/
// Generic pin modes - here only INPUT, OUTPUT, INPUT_PULLUP and INPUT_PULLDOWN can be used
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

#define PIN_MODE		0
#define WRITE_DIGITAL	1
#define WRITE_TOGGLE    2
#define WRITE_ANALOG	3
#define WRITE_ANALOG_F	4
#define READ_DIGITAL	5
#define READ_ANALOG		6
#define SET_INTERRUPT	7
#define UNSET_INTERRUPT	8

#define PI 3.1415926535897932384626433832795
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105
#define EULER 2.718281828459045235360287471352

#define SERIAL  0x0
#define DISPLAY 0x1

enum BitOrder {
	LSBFIRST = 0,
	MSBFIRST = 1
};

// moved to WInterrupts.h
////      LOW 0
////      HIGH 1
//#define CHANGE 2
//#define FALLING 3
//#define RISING 4
//
//#define DEFAULT 1
//#define EXTERNAL 0

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif /* _WIRING_CONSTANTS_ */
