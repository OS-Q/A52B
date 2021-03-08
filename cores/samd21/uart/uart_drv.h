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

#ifndef UART_Drv_h
#define UART_Drv_h

#include "WString.h"
#include <inttypes.h>
#include "Arduino.h"

#define NO_LAST_PARAM       0
#define LAST_PARAM          1
#define DUMMY_DATA          0x00
#define TXBUF_LEN           BUF_SIZE

#define UART_BAUDRATE       3000000

#define ESP_SR_TIMEOUT      1000

#define SLAVESELECT      ESP_CS
#define SLAVEREADY       ACK_PIN
#define SLAVEHANDSHAKE   ESP_BOOT

typedef void (*tpDriverIsr)(uint8_t*);

static tpDriverIsr uartIsr;

class UartDrv {
  private:
	
	uint8_t _txIndex;
	uint8_t _txBuf[TXBUF_LEN];
	uint8_t _rxBuf[BUF_SIZE];
	static volatile uint8_t _interruptReq;
	uint8_t _pktType = START_CMD;

	//  Move SS pin HIGH to signal that a packet has been read and we are ready
	// to receive a new one.
	void _ack(){
		PORT->Group[PORTA].OUTSET.reg = (1ul << 18); //digitalWrite(SLAVESELECT, HIGH);
	}

	// Initializes the _txBuf buffer with the b byte and sets the buffer index to 1
	void _txBufInitWByte(uint8_t b){
		_txBuf[0] = b;
		_txIndex = 1;
		_pktType = b;
	}

	// Appends to the _txBuf buffer a new b byte if there's enough space;
	bool _txBufAppendByte(uint8_t b){
		if (_txIndex < TXBUF_LEN) {
			_txBuf[_txIndex++] = b;
			return true;
		}
		return false;
	}

	// Finalizes the packet adding the END_CMD and writing out the command
	bool _txBufFinalizePacket(){
		// add the END_CMD to the buffer
		_txBufAppendByte(END_CMD);
		_txBufSetOverallLen();
		// write out the data to companion chip
		return _writeData(_txBuf, _txIndex);
	}

	// Set the overall len of the command packet
	void _txBufSetOverallLen(){
		if(_pktType == DATA_PKT){ // len is 4 bytes
			_txBuf[2] = _txIndex & 0xFF;
			_txBuf[3] = (_txIndex >> 8) & 0xFF;
			_txBuf[4] = (_txIndex >> 16) & 0xFF;
			_txBuf[5] = (_txIndex >> 24) & 0xFF;
		}
		else // len is 1 byte
			_txBuf[2] = _txIndex;
	}

	bool _writeData(uint8_t *data, uint32_t len);
	bool _writeBuffPkt(uint8_t *data, uint32_t len, uint8_t offset);

	friend void interruptFunction();
	
  public:
	// generic functions
	UartDrv();
	void begin();
	void end();

	// The function watches the _interruptReq variable set in the _SRcallback
	// when companion fires an ISR. If the callback exists it will be called
	void handleEvents(void){
		if(_interruptReq && uartIsr){
			_interruptReq = 0;
			uartIsr(NULL);
		}
	}

	void registerCb(tpDriverIsr pfIsr){
		uartIsr = pfIsr;
	}

	// command related functions
	bool sendCmd(uint8_t cmd, uint8_t numParam);
	void sendDataPkt(uint8_t cmd);
	bool sendParam(uint8_t *param, uint8_t param_len, uint8_t lastParam = NO_LAST_PARAM);
	bool sendParam(uint16_t param, uint8_t lastParam = NO_LAST_PARAM);
	bool appendByte(uint8_t data, uint8_t lastParam = NO_LAST_PARAM);
	bool appendBuffer(const uint8_t *data, uint32_t len);
	
	// SPI Data Register functions
	uint16_t readDataISR(uint8_t *buffer, bool first = true);
};

#endif
#endif
#endif // COMMUNICATION_LIB_ENABLED