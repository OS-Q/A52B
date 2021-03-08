/*
  Copyright (c) 2020 Meteca SA.  All right reserved.

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

#ifndef _CONFIG_SAMD_H_
#define _CONFIG_SAMD_H_


/*
 * Comment the following define to disable USB at compile time.
 * If not required by a specific application, USB can be disabled in order to reduce
 * power consumption.
 * Disabling USB also allows to save ~6KB of flash and ~1.7KB of RAM.
 *
 * Note: USB programming will not work if USB is disabled. Connect SAMD's reset pin
 * to ground twice in 500 ms to invoke bootloader and restore USB programming.
 */

#define USB_ENABLED


/*
 * Comment the following define to disable communication library at compile time.
 * Communication library is used to exchange messages between SAMD and ESP.
 * Disabling communication allows to save ~6.6KB of flash and ~1.3KB of RAM.
 *
 * Note: disabling communication library is not a recommended setting since GPIOs
 * virtualization and Control2WiFi library will stop working. However, you can still
 * use SerialCompanion to communicate with companion microcontroller.
 */
#define COMMUNICATION_LIB_ENABLED


/*
 * Comment the following define to disable SerialCompanion at compile time.
 * Disabling SerialCompanion allows to save 228 bytes of flash and 660 bytes of RAM.
 *
 * Note: disabling SerialCompanion will automatically disable double USB port.
 * You can still use communication library to communicate with companion mcu.
 */
#define SERIAL_COMPANION_ENABLED


/*
 * Comment the following define to disable Serial1 at compile time.
 * Disabling Serial1 allows to save 36 bytes of flash and 576 bytes of RAM. 
 */
#define SERIAL_1_ENABLED


/*
 * Enable the following define to reduce power consumption.
 *
 * Note: enabling this macro breaks the Arduino API. Pins are not set to INPUT,
 * only basic clocks are enabled and ADC and DAC are not initialized (they will
 * automatically be initialized when analog functions are called).
 * If you use external libraries that may need one of the above features, make
 * sure of manually set required settings.
 */
//#define VERY_LOW_POWER

/*
 * Enable one of the following defines to select the communication channel between
 * SAMD21 and ESP32. If COMMUNICATION_LIB_ENABLED at least one of the two below
 * defines must be enabled.
 *
 * Note: If you select COMM_CH_UART you cannot use SerialCompanion object inside
 * sketch. Please note that the choice of communication channel here must be compliant
 * to the choice made in ESP's configuration file or the communication will not work.
 */
// communication channel can be selected from platformio.in in vscode
// do not overwrite that selection if any
#if !defined(COMM_CH_SPI) && !defined(COMM_CH_UART)
	// comment out following row to enable UART channel
	//#define COMM_CH_UART

	// comment out following row to enable SPI channel
	#define COMM_CH_SPI
#endif

// check that just one channel is used at the same time
#if defined(COMM_CH_SPI) && defined(COMM_CH_UART)
	#error "Please select just one communication interface (COMM_CH_SPI or COMM_CH_UART) in mbc_config.h file"
#endif

// check that at least one of the two channel is enabled if communication is active
#if defined(COMMUNICATION_LIB_ENABLED) && !defined(COMM_CH_SPI) && !defined(COMM_CH_UART)
	#error "Please select one communication interface (COMM_CH_SPI or COMM_CH_UART) in mbc_config.h file"
#endif


#endif // _CONFIG_SAMD_H_