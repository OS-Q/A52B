/*
  Copyright (c) 2015 Arduino LLC.  All right reserved.

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

#include "Arduino.h"

#ifdef __cplusplus
 extern "C" {
#endif

void pinMode( uint32_t ulPin, uint32_t ulMode )
{
  // Handle the case the pin isn't usable as PIO
  if ( g_APinDescription[ulPin].ulPinType == PIO_NOT_A_PIN ) {
    return ;
  }
  
  // Handle mode on pins connected to communication chip
  if(g_APinDescription[ulPin].ulPinType == PIO_EXTERNAL){
	  #ifdef COMMUNICATION_LIB_ENABLED
	  sendGPIOMode(PIN_MODE, (uint8_t)ulPin, ulMode);
	  #endif
	  return;
  }

  // Set pin mode according to chapter '22.6.3 I/O Pin Configuration'
  switch ( ulMode ) {
    case INPUT:
      // Set pin to input mode
      PORT->Group[g_APinDescription[ulPin].ulPort].PINCFG[g_APinDescription[ulPin].ulPin].reg=(uint8_t)(PORT_PINCFG_INEN) ;
      PORT->Group[g_APinDescription[ulPin].ulPort].DIRCLR.reg = (uint32_t)(1<<g_APinDescription[ulPin].ulPin) ;
    break ;

    case INPUT_PULLUP:
      // Set pin to input mode with pull-up resistor enabled
      PORT->Group[g_APinDescription[ulPin].ulPort].PINCFG[g_APinDescription[ulPin].ulPin].reg=(uint8_t)(PORT_PINCFG_INEN|PORT_PINCFG_PULLEN) ;
      PORT->Group[g_APinDescription[ulPin].ulPort].DIRCLR.reg = (uint32_t)(1<<g_APinDescription[ulPin].ulPin) ;

      // Enable pull level (cf '22.6.3.2 Input Configuration' and '22.8.7 Data Output Value Set')
      PORT->Group[g_APinDescription[ulPin].ulPort].OUTSET.reg = (uint32_t)(1<<g_APinDescription[ulPin].ulPin) ;
    break ;

    case INPUT_PULLDOWN:
      // Set pin to input mode with pull-down resistor enabled
      PORT->Group[g_APinDescription[ulPin].ulPort].PINCFG[g_APinDescription[ulPin].ulPin].reg=(uint8_t)(PORT_PINCFG_INEN|PORT_PINCFG_PULLEN) ;
      PORT->Group[g_APinDescription[ulPin].ulPort].DIRCLR.reg = (uint32_t)(1<<g_APinDescription[ulPin].ulPin) ;

      // Enable pull level (cf '22.6.3.2 Input Configuration' and '22.8.6 Data Output Value Clear')
      PORT->Group[g_APinDescription[ulPin].ulPort].OUTCLR.reg = (uint32_t)(1<<g_APinDescription[ulPin].ulPin) ;
    break ;

    case OUTPUT:
      // enable input, to support reading back values, with pullups disabled
      PORT->Group[g_APinDescription[ulPin].ulPort].PINCFG[g_APinDescription[ulPin].ulPin].reg=(uint8_t)(PORT_PINCFG_INEN) ;

      // Set pin to output mode
      PORT->Group[g_APinDescription[ulPin].ulPort].DIRSET.reg = (uint32_t)(1<<g_APinDescription[ulPin].ulPin) ;
    break ;

    default:
      // do nothing
    break ;
  }

  #if defined (BRIKI_ABC)
  if(ulPin == LED_BUILTIN && ulMode == OUTPUT){
    ledEn();
    return;
  }
  #endif
}

void digitalWrite( uint32_t ulPin, uint32_t ulVal )
{
  // Handle the case the pin isn't usable as PIO
  if ( g_APinDescription[ulPin].ulPinType == PIO_NOT_A_PIN ) {
    return ;
  }

  // Handle write on pins connected to communication chip
  if(g_APinDescription[ulPin].ulPinType == PIO_EXTERNAL){
	  #ifdef COMMUNICATION_LIB_ENABLED
	  sendGPIOWrite(WRITE_DIGITAL, (uint8_t)ulPin, ulVal);
	  #endif
	  return;
  }
  
  EPortType port = g_APinDescription[ulPin].ulPort;
  uint32_t pin = g_APinDescription[ulPin].ulPin;
  uint32_t pinMask = (1ul << pin);

  #if defined (BRIKI_ABC)
  if(ulPin == LED_BUILTIN){
    if(ulVal == LOW)
      ledOff();
    else
      ledOn();
    return;
  }
  #endif 

  if ( (PORT->Group[port].DIRSET.reg & pinMask) == 0 ) {
    // the pin is not an output, disable pull-up if val is LOW, otherwise enable pull-up
    PORT->Group[port].PINCFG[pin].bit.PULLEN = ((ulVal == LOW) ? 0 : 1) ;
  }

  switch ( ulVal )
  {
    case LOW:
      PORT->Group[port].OUTCLR.reg = pinMask;
    break ;

    default:
      PORT->Group[port].OUTSET.reg = pinMask;
    break ;
  }

  return ;
}

void digitalToggle(uint32_t ulPin)
{
  // Handle the case the pin isn't usable as PIO
  if ( g_APinDescription[ulPin].ulPinType == PIO_NOT_A_PIN ) {
    return ;
  }

  // Handle write on pins connected to communication chip
  if(g_APinDescription[ulPin].ulPinType == PIO_EXTERNAL){
	  #ifdef COMMUNICATION_LIB_ENABLED
	  sendGPIOWrite(WRITE_TOGGLE, (uint8_t)ulPin);
	  #endif
	  return;
  }
  
  EPortType port = g_APinDescription[ulPin].ulPort;
  uint32_t pin = g_APinDescription[ulPin].ulPin;
  uint32_t pinMask = (1ul << pin);

  #if defined (BRIKI_ABC)
  if(ulPin == LED_BUILTIN){
    ledToggle();
    return;
  }
  #endif

  PORT->Group[port].OUTTGL.reg = pinMask;
}

int digitalRead( uint32_t ulPin )
{
  // Handle the case the pin isn't usable as PIO
  if ( g_APinDescription[ulPin].ulPinType == PIO_NOT_A_PIN )
  {
    return LOW ;
  }

  // Handle write on pins connected to communication chip
  if(g_APinDescription[ulPin].ulPinType == PIO_EXTERNAL){
	#ifdef COMMUNICATION_LIB_ENABLED
		return sendGPIORead(READ_DIGITAL, (uint8_t)ulPin);
	#else
		return 0;
	#endif
  }
  
  if ( (PORT->Group[g_APinDescription[ulPin].ulPort].IN.reg & (1ul << g_APinDescription[ulPin].ulPin)) != 0 )
  {
    return HIGH ;
  }

  return LOW ;
}

#if defined (BRIKI_ABC)
void ledBuiltinSetColor(uint8_t r, uint8_t g, uint8_t b)
{
  ledSetColor(r, g, b);
}

void ledBuiltinSetBrightness(uint8_t br)
{
  ledSetBrightness(br);
}
#endif

#ifdef __cplusplus
}
#endif

