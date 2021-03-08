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
#if defined(COMM_CH_SPI)

#ifndef SPI_DRV
#define SPI_DRV

#include <stdint.h>
#include "spi_def.h"
#include "driver/spi_slave.h"
#include "Arduino.h"

#define NO_LAST_PARAM       0
#define LAST_PARAM          1

static uint32_t srTimer = 0;
/*
*
*/
class SpiDrv {

  private:
	static uint8_t _raw_pkt[2][BUFFER_SIZE]; //SPI buffer (limited to 128 bytes)
	static uint8_t _recvByte;
	uint8_t _longCmdPkt[MAX_CMD_LEN];
	uint16_t _rxLen = 0;  // size doesn't match maximum dimension of data that could be received from a DATA_PKT
	volatile static uint8_t _processing;
	uint8_t _multiPktCmd = 0;
	volatile static bool _rxEnabled;
	uint8_t _repPkt[MAX_CMD_LEN]; //response array
	uint32_t _txIndex;
	uint8_t _pktType = START_CMD;
	volatile static bool _sendingWait;
	bool _suspended = false;
	bool _isBufferValid = false;
	volatile static bool _dataArrived;

	void _setSR(bool level){
		uint32_t deltaT = esp_timer_get_time() - srTimer;
		if(deltaT < 10)
		  delayMicroseconds(10 - deltaT);
		if(level)
			GPIO.out_w1ts = ((uint32_t)1 << 22); //digitalWrite(PIN_SPI_SR, HIGH);
		else
			GPIO.out_w1tc = ((uint32_t)1 << 22); //digitalWrite(PIN_SPI_SR, LOW);
		srTimer = esp_timer_get_time();
	}

	//Initializes the _repPkt buffer with the b byte and sets the buffer index to 1;
	void _txBufInitWByte(uint8_t b){
		_repPkt[0] = b;
		_txIndex = 1;
		_pktType = b;
	}

	//Appends to the _repPkt buffer a new b byte if there's enough space;
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
		// write out the data (32 byte minimum) to companion chip
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
	
	friend void transInt(spi_slave_transaction_t * trans);

  public:
	tsDataPacket recPacket;
	tsNewCmd longCmd;
	uint32_t socketPktPtr = 0;
	bool notifyAck = false;

	SpiDrv();
	void begin();

	uint8_t canProcess(){
	  return _processing;
	}

	static void enableProcess(){
	  _processing++;
	}

	void endProcessing(){
	  if(_processing > 0)
		_processing--;
	}

	void ack(void){
		// the following operation is time critical. Suspend scheduler on this core as to avoid interruptions
		vTaskSuspendAll();
		_setSR(HIGH);
		_setSR(LOW);
		// resume scheduler
		xTaskResumeAll();
	}
    
	int8_t read(tsNewCmd *_pkt);
	void write(uint8_t *_rep, uint32_t len);

	void enableRx();

	// command related functions
	void sendCmd(uint8_t cmd, uint8_t numParam);
	void sendDataPkt(uint8_t cmd);
	void sendParam(uint8_t *param, uint8_t param_len, uint8_t lastParam = NO_LAST_PARAM);
	void sendParam(uint16_t param, uint8_t lastParam = NO_LAST_PARAM);
	void appendByte(uint8_t data, uint8_t lastParam);
	void appendBuffer(const uint8_t *data, uint32_t len);

	void clearRx(void){
		memset(_raw_pkt, 0, BUFFER_SIZE);
	}
};

#endif //SPI_DRV
#endif //COMM_CH_SPI
#endif // COMMUNICATION_LIB_ENABLED