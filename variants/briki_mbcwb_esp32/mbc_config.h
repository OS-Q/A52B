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

#ifndef _CONFIG_ESP_H_
#define _CONFIG_ESP_H_

/*
 * Comment the following define to disable communication library at compile time.
 * Communication library is used to exchange messages between SAMD and ESP.
 * Disabling communication allows to save ~38.3KB of flash and ~2.3KB of RAM.
 *
 * Note: disabling communication library is not a recommended setting since GPIOs
 * virtualization and WiFi2Control library will stop working. However, you can still
 * use Serial object to communicate with companion microcontroller.
 */
#define COMMUNICATION_LIB_ENABLED

/*
 * Comment the following define to disable WiFi Over The Air update at compile time.
 * Disabling OTA allows to save ~686.5KB of flash and ~28.2KB of RAM.
 *
 * Note: Disabling OTA will automatically disable MBC AP.
 * You can still use ArduinoOTA library to manually restore this feature.
 */
#define OTA_RUNNING

/*
 * Comment the following define to disable MBC Access Point at compile time.
 * Disabling AP allows to save ~305.8KB of flash and ~18.7KB of RAM.
 *
 * Note: WiFi OTA is still possible if, inside sketch, ESP is properly configured
 * as station and connected to a network.
 * You can still raise your own Access Point by using WiFi library.
 */
#define MBC_AP_ENABLED

/*
 * Enable the following define to change the default AP name. Currently it defaults
 * to MBC-WB-XXXXXX, where XXXXXX represents the final part of the MAC address.
 */
//#define MBC_AP_NAME "myAP"

/*
 * Enable the following define to set a password for connecting the default AP.
 * Currently no password is required.
 */
//#define MBC_AP_PASSWORD "11223344"


/*
 * Comment the following define to disable Serial1 at compile time.
 * Disabling Serial1 allows to save 12 bytes of flash and 24 bytes of RAM.
 */
#define SERIAL_1_ENABLED


/*
 * Comment the following define to disable Serial2 at compile time.
 * Disabling Serial2 allows to save 12 bytes of flash and 24 bytes of RAM. 
 */
#define SERIAL_2_ENABLED


/*
 * Enable one of the following defines to select the communication channel between
 * SAMD21 and ESP32. If COMMUNICATION_LIB_ENABLED at least one of the two below
 * defines must be enabled.
 *
 * Note: If you select COMM_CH_UART you cannot use Serial object inside
 * sketch. Please note that the choice of communication channel here must be compliant
 * to the choice made in SAMD's configuration file or the communication will not work.
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


#endif // _CONFIG_ESP_H_