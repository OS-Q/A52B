/*
  Meteca SA.  All right reserved.
  created by Dario Trimarchi and Chiara Ruggeri 2018-2019
  email: support@meteca.org

  This library is distributed under creative commons license CC BY-NC-SA
  https://creativecommons.org/licenses/by-nc-sa/4.0/
  You are free to redistribute this code and/or modify it under
  the following terms:
  - Attribution: You must give appropriate credit, provide a link
  to the license, and indicate if changes were made.
  - Non Commercial: You may not use the material for commercial purposes.
  - Share Alike:  If you remix, transform, or build upon the material,
  you must distribute your contributions under the same license as the original.
  
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef UART_DEF_H
#define UART_DEF_H

#define PIN_SPI_SR          ACK_PIN
#define PIN_SPI_HANDSHAKE   BOOT


#define BUFFER_SIZE         255
#define UART_TIMEOUT        1000

#define START_CMD           0xe0
#define DATA_PKT            0xf0
#define END_CMD             0xee
#define ERR_CMD             0xef

#define CMD_FLAG            0
#define REPLY_FLAG          1<<7
#define DATA_FLAG           0x40

#endif