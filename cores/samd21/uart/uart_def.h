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

#define START_CMD   	0xe0
#define DATA_PKT		0xf0
#define END_CMD     	0xee
#define ERR_CMD   		0xef
#define CMD_POS			1		// Position of Command OpCode on UART stream
#define PARAM_LEN_POS 	2		// Position of Param len on UART stream

#define BUF_SIZE            255
#define UART_MAX_TX_SIZE    255

#endif // UART_DEF_H