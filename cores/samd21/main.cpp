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

#define ARDUINO_MAIN
#include "Arduino.h"
#if defined (BRIKI_ABC)
#include "ABCNeopixel.h"
extern "C" void ledEn (void);
extern "C" void ledOff (void);
#endif

// Weak empty variant initialization function.
// May be redefined by variant files.
void initVariant() __attribute__((weak));
void initVariant() 
{
	pinMode(ESP_ON, OUTPUT);
	digitalWrite(ESP_ON, LOW);
#if defined (BRIKI_ABC)
	ledEn();
	ledOff();
#endif
}

// Initialize C library
extern "C" void __libc_init_array(void);

/*
 * \brief Main entry point of Arduino application
 */
int main( void )
{
	init();
	__libc_init_array();
	initVariant();

	#ifdef USB_ENABLED
	delay(1);
	USBDevice.init();
	USBDevice.attach();
	#endif

	#ifdef COMMUNICATION_LIB_ENABLED
	// initialize communication between this and the communication microcontroller
	communication.communicationBegin();
	#endif // COMMUNICATION_LIB_ENABLED

	setup();

	for (;;){
		loop();
		if (serialEventRun) serialEventRun();
		#ifdef COMMUNICATION_LIB_ENABLED
		communication.handleCommEvents();
		#endif // COMMUNICATION_LIB_ENABLED
	}	
	return 0;
}

#ifdef COMMUNICATION_LIB_ENABLED
// C wrapper function for communication.handleCommEvents()
extern "C" void handleCommEvents(){
	communication.handleCommEvents();
}
#endif // COMMUNICATION_LIB_ENABLED