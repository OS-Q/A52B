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

#include "mbc_config.h"

#ifdef COMMUNICATION_LIB_ENABLED
#if defined(COMM_CH_UART)

#ifndef UART_DRV
#define UART_DRV

#include <stdint.h>

#define NO_LAST_PARAM       0
#define LAST_PARAM          1

#define UART_BAUDRATE       3000000

/*
*
*/
class UartDrv {

  private:
	uint8_t _pktType = START_CMD;
	uint8_t _repPkt[MAX_CMD_LEN]; //response array
	volatile static uint8_t _processing;
	uint8_t _raw_pkt[BUFFER_SIZE]; //UART buffer (limited to 255 bytes)
	uint8_t _recvByte = 0;
    uint8_t _multiPktCmd = 0;
    uint16_t _rxLen = 0;  // size doesn't match maximum dimension of data that could be received from a DATA_PKT
	uint8_t _longCmdPkt[MAX_CMD_LEN];
	uint32_t _txIndex;
	uint32_t _txPktSize;

	//Initializes the _repPkt buffer with the b byte and sets the buffer index to 1
	void _txBufInitWByte(uint8_t b){
		_repPkt[0] = b;
		_txIndex = 1;
		_pktType = b;
	}

	// Appends to the _repPkt buffer a new b byte if there's enough space
	bool _txBufAppendByte(uint8_t b){
		if (_txIndex < MAX_CMD_LEN) {
			_repPkt[_txIndex++] = b;
			return true;
		}
		return false;
	}

	//Finalizes the packet adding the END_CMD and writing out the command
	void _txBufFinalizePacket(){
		// add the END_CMD to the buffer
		_txBufAppendByte(END_CMD);
		_txBufSetOverallLen();
		// write out the data to companion chip
		write(_repPkt, _txIndex);
	}

	//Set the overall len of the command packet
	void _txBufSetOverallLen(){
		if(_pktType == DATA_PKT){ // len is 4 bytes
			_repPkt[2] = _txIndex & 0xFF;
			_repPkt[3] = (_txIndex >> 8) & 0xFF;
			_repPkt[4] = (_txIndex >> 16) & 0xFF;
			_repPkt[5] = (_txIndex >> 24) & 0xFF;
		}
		else // len is 1 byte
			_repPkt[2] = _txIndex;
	}

	void _writeBuffPkt(const uint8_t *data, uint32_t len, uint8_t offset);

  public:
    tsDataPacket recPacket;
    tsNewCmd longCmd;
    uint32_t socketPktPtr = 0;
	bool notifyAck = false;
    
    UartDrv();
    void begin();
    uint8_t canProcess();
    static void enableProcess();
    void end();
    void endProcessing();
    
    int8_t read(tsNewCmd *_pkt);
    void write(uint8_t *_pkt, uint32_t len);
	void ack(void);
	void enableRx();
  	
	// command related functions
	void sendCmd(uint8_t cmd, uint8_t numParam);
	void sendDataPkt(uint8_t cmd);
	void sendParam(uint8_t *param, uint8_t param_len, uint8_t lastParam = NO_LAST_PARAM);
	void sendParam(uint16_t param, uint8_t lastParam = NO_LAST_PARAM);
	void appendByte(uint8_t data, uint8_t lastParam);
	void appendBuffer(const uint8_t *data, uint32_t len);
	
    void clearRx(void);
};

#endif //UART_DRV
#endif //COMM_CH_UART
#endif // COMMUNICATION_LIB_ENABLED