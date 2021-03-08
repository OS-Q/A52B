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

#ifndef COMM_DEF_H
#define COMM_DEF_H

#include "mbc_config.h"

#if defined(COMM_CH_UART)
  #include "uart/uart_def.h"
#elif defined(COMM_CH_SPI)
  #include "spi/spi_def.h"
#endif

#include <inttypes.h>

#define MAX_CMD_LEN         600
#define DATA_HEAD_FOOT_LEN  8 //7 header: cmdType(1), cmd(1), size(4), sock(1); 1 footer: end(1)

enum {
	// Generic
	NONE				= 0x00,
	GPIO_REQUEST		= 0x01,
	GPIO_RESPONSE		= 0x02,
	GPIO_INTERRUPT		= 0x03,
	PUT_CUSTOM_DATA		= 0x04,
	SERIAL_STATUS		= 0x05,
	SYSTEMCMD_REQUEST	= 0x06,
	SYSTEMCMD_RESPONSE	= 0x07,
	AUTOBAUD_UPD		= 0x08
};

enum numParams{
    PARAM_NUMS_0 = 0,
    PARAM_NUMS_1,
    PARAM_NUMS_2,
    PARAM_NUMS_3,
    PARAM_NUMS_4,
    PARAM_NUMS_5,
    MAX_PARAM_NUMS
};

enum sizeParams{
    PARAM_SIZE_0 = 0,
    PARAM_SIZE_1,
    PARAM_SIZE_2,
    PARAM_SIZE_3,
    PARAM_SIZE_4,
    PARAM_SIZE_5,
    PARAM_SIZE_6
};

typedef struct __attribute__((__packed__))
{
  uint8_t cmdType;
  uint8_t cmd;
  uint8_t nParam;
  uint32_t totalLen;
  uint8_t* dataPtr;
} tsNewCmd;

typedef struct __attribute__((__packed__))
{
  uint8_t cmdType;
  uint8_t cmd;
  uint32_t totalLen;
  uint32_t receivedLen;
  bool endReceived;
} tsDataPacket;

#endif // COMM_DEF_H