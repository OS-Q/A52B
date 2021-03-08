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

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/periph_ctrl.h"
#include "driver/uart.h"
#include "Arduino.h"

static QueueHandle_t uart0_queue;
extern SemaphoreHandle_t UARTmutex;

volatile uint8_t UartDrv::_processing = 0;
uint32_t srTimer = 0;

UartDrv::UartDrv(){
  _processing = 0;
  longCmd.totalLen = 0;
  recPacket.receivedLen = 0;
  recPacket.endReceived = true;
}

void UartDrv::begin(){
	pinMode(SS, INPUT_PULLUP);
	pinMode(PIN_SPI_SR, OUTPUT);
	pinMode(PIN_SPI_HANDSHAKE, OUTPUT);
	// set sr HIGH for syncronization purpose
	GPIO.out_w1ts = ((uint32_t)1 << 22); // digitalWrite(PIN_SPI_SR, HIGH);
	periph_module_disable( PERIPH_UART0_MODULE );
	delay( 20 );
	periph_module_enable( PERIPH_UART0_MODULE );
	/* Configure parameters of an UART driver,
	* communication pins and install the driver */
	uart_config_t uart_config = {
		.baud_rate = UART_BAUDRATE,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE
	};
	uart_param_config(UART_NUM_0, &uart_config);

	//Set UART pins (using UART0 default pins ie no changes.)
	uart_set_pin(UART_NUM_0, g_APinDescription[TX].ulPin, g_APinDescription[RX].ulPin, -1, -1);
	//Install UART driver, and get the queue.
	uart_driver_install(UART_NUM_0, BUFFER_SIZE * 2, 0, 20, &uart0_queue, 0);

	// disable log to avoid spurious messages interfering with communication
	esp_log_level_set("*", ESP_LOG_NONE);
	// restore to default state
	GPIO.out_w1tc = ((uint32_t)1 << 22); // digitalWrite(PIN_SPI_SR, LOW);
}

void UartDrv::end(){
	// reset Serial module
	periph_module_disable( PERIPH_UART0_MODULE );
	delay( 20 );
	periph_module_enable( PERIPH_UART0_MODULE );
	// restore lot to ERROR level
	esp_log_level_set("*", ESP_LOG_ERROR);
}

uint8_t UartDrv::canProcess(){
	if(_processing == 0){
		uart_event_t event;
		if(UARTmutex != NULL)
			xSemaphoreTakeRecursive(UARTmutex, portMAX_DELAY); // avoid resource conflict with OTA
		if(xQueueReceive(uart0_queue, (void * )&event, (portTickType)0)) {
			if(event.type == UART_DATA){
				_recvByte = event.size;
				uart_read_bytes(UART_NUM_0, _raw_pkt, _recvByte, (portTickType)UART_TIMEOUT);
				_processing = 1;
				// ack the packet by moving HANDSHAKE pin
				GPIO.out_w1ts = ((uint32_t)1);	//digitalWrite(PIN_SPI_HANDSHAKE, HIGH);
				delayMicroseconds(10);
				GPIO.out_w1tc = ((uint32_t)1); //digitalWrite(PIN_SPI_HANDSHAKE, LOW);
			}
			else xQueueReset(uart0_queue);
		}
		if(UARTmutex != NULL)
			xSemaphoreGiveRecursive(UARTmutex);
	}
	return _processing;
}

void UartDrv::enableProcess(){}

void UartDrv::endProcessing(){
	_processing = 0;
}

int8_t UartDrv::read(tsNewCmd *_pkt){
  uint8_t* pktPtr = _raw_pkt;
  
 // ---------------------------------------------------------------------------- receiving long command packets
  if (recPacket.receivedLen > 0) {
    uint8_t endOffset;
    
	// check if END_CMD is in the last received byte
	if (pktPtr[_recvByte - 1] == END_CMD) {
		endOffset = _recvByte;
		
		if (recPacket.receivedLen + endOffset == recPacket.totalLen) {
		  recPacket.endReceived = true;
		  notifyAck = true;
		  _pkt->cmdType = recPacket.cmdType;
		  _pkt->cmd = recPacket.cmd;
		  _pkt->nParam = endOffset - 1;
		  _pkt->totalLen = recPacket.totalLen - DATA_HEAD_FOOT_LEN;
		  _pkt->dataPtr = &pktPtr[0];

		  recPacket.receivedLen = recPacket.totalLen = 0;

		  return 0;
		}
	}
    
    if (recPacket.endReceived == false) {
      recPacket.receivedLen += _recvByte;

      if(recPacket.receivedLen >= recPacket.totalLen){
        recPacket.endReceived = true;
        recPacket.receivedLen = recPacket.totalLen = 0;
      }
      
      _pkt->cmdType = recPacket.cmdType;
      _pkt->cmd = recPacket.cmd;
      _pkt->totalLen = recPacket.totalLen - DATA_HEAD_FOOT_LEN;
      _pkt->nParam = _recvByte;
      _pkt->dataPtr = &pktPtr[0];

      notifyAck = true; 
      return 0;
    }
  }
  // ---------------------------------------------------------------------------- decoding command
  else if (pktPtr[0] == START_CMD || _multiPktCmd) {
    if(_multiPktCmd){
      uint8_t offset = 4; // 4 bytes were already removed from the message (command header)
      // copy the new part of the message
      memcpy((longCmd.dataPtr + _rxLen - offset), pktPtr, _recvByte);
      _rxLen += _recvByte;
      if(longCmd.totalLen <= _rxLen){ 
        // the whole message was received - check for end cmd and return the result
        _multiPktCmd = 0;
        if(longCmd.dataPtr[longCmd.totalLen - 5] != END_CMD){
          return -1;
        }
        _pkt->cmdType = longCmd.cmdType;
        _pkt->cmd = longCmd.cmd;
        _pkt->totalLen = longCmd.totalLen;
        _pkt->nParam = longCmd.nParam;
        _pkt->dataPtr = longCmd.dataPtr;
        notifyAck = true;
        return 0;
      }
      if(++_multiPktCmd == 4){
        // we reached the maximum cmd len size and we still didn't find end cmd. Maybe an error occurred
        _multiPktCmd = 0;
		return -1;
      }
    }

    _pkt->cmdType = pktPtr[0];
    _pkt->cmd = pktPtr[1];
    _pkt->totalLen = (uint32_t)pktPtr[2];
    _pkt->nParam = pktPtr[3];
    _pkt->dataPtr = &pktPtr[4];
    
    if (_pkt->totalLen > _recvByte) {
      longCmd.cmdType = _pkt->cmdType;
      longCmd.cmd = _pkt->cmd;
      longCmd.totalLen = _pkt->totalLen;
      longCmd.nParam = _pkt->nParam;

      uint16_t payLoadLen = _recvByte - 4;
      _rxLen = _recvByte;

      // copy total len - cmd header size
      memset(_longCmdPkt, 0, BUFFER_SIZE << 2); // clear buffer, max cmd len = 4 * BUFFER_SIZE
      longCmd.dataPtr = _longCmdPkt;
      memcpy(longCmd.dataPtr, _pkt->dataPtr, payLoadLen);
     
      notifyAck = true; 
	 
      _multiPktCmd = 1;

      return -1;
    }
    else {
      if (pktPtr[_pkt->totalLen - 1] != END_CMD){
        return -1;
      }
      return 0;
    }
  }
  // ---------------------------------------------------------------------------- decoding data packets
  else if (pktPtr[0] == DATA_PKT)
  {
	 
    _pkt->cmdType = recPacket.cmdType = pktPtr[0];
    _pkt->cmd = recPacket.cmd = pktPtr[1];
    recPacket.receivedLen = _recvByte;
    recPacket.totalLen = (pktPtr[5] << 24) + (pktPtr[4] << 16) + (pktPtr[3] << 8) + pktPtr[2];
    recPacket.endReceived = false;
    _pkt->totalLen = recPacket.totalLen - DATA_HEAD_FOOT_LEN;
    // payload - (cmdType (1 byte) + cmd (1 byte) + totalLen (4 byte)) = payload - 6
    _pkt->nParam = recPacket.receivedLen - (sizeof(recPacket.cmdType) + sizeof(recPacket.cmd) + sizeof(recPacket.totalLen));
    _pkt->dataPtr = &pktPtr[6];

	// check if END_CMD is in the last received byte
	if(pktPtr[_recvByte - 1] == END_CMD){
		recPacket.endReceived = true;
		recPacket.receivedLen = recPacket.totalLen = 0;
		_pkt->nParam = 255; // this value is used as flag that indicates that data packet is all contained in a single packet
	}

    notifyAck = true; 
    return 0;
  }
  // error;
  else { 
    return -1;
  }

  return 0;
}


void UartDrv::write(uint8_t *_pkt, uint32_t len){
	uint16_t sentBytes = 0;
	while(len > 0){ // sent all data in BUFFER_SIZE chunks
		uint16_t transferBytes = len > BUFFER_SIZE ? BUFFER_SIZE : len;
		if(UARTmutex != NULL)
			xSemaphoreTakeRecursive(UARTmutex, portMAX_DELAY); // avoid resource conflict with OTA
		// wait for Slave Select - when HIGH companion is ready for a new transaction
		while(((GPIO.in >> 5) & 0x1) != HIGH); // while(digitalRead(SS) != HIGH);
		GPIO.out_w1ts = ((uint32_t)1 << 22); //_setSR(HIGH);
		uart_write_bytes(UART_NUM_0, (const char*) &_pkt[sentBytes], transferBytes); // write out data
		if(uart_wait_tx_done(UART_NUM_0, (portTickType)UART_TIMEOUT) == ESP_ERR_TIMEOUT){ // wait until all data are sent
			// timeout occurred. Try to send message again
			uart_write_bytes(UART_NUM_0, (const char*) &_pkt[sentBytes], transferBytes);
			uart_wait_tx_done(UART_NUM_0, (portTickType)UART_TIMEOUT);
		}
		if(UARTmutex != NULL)
			xSemaphoreGiveRecursive(UARTmutex);
		sentBytes += transferBytes;
		len -= transferBytes;
		
		// set the ISR trigger signal to fire a read process
		GPIO.out_w1tc = ((uint32_t)1 << 22); //_setSR(LOW);
		// wait SS to come back low - companion received our ISR request
		while(((GPIO.in >> 5) & 0x1) == HIGH); //while(digitalRead(SS) == HIGH);
	}
}

void UartDrv::ack(void){
// packet was already acked when it was received
}

void UartDrv::enableRx(){}

/* -----------------------------------------------------------------
* Prepares the _repPkt to be sent as a new command to companion chip
* It requires also the number of attached parameters.
*/
void UartDrv::sendCmd(uint8_t cmd, uint8_t numParam){
	// init the buffer with the START_CMD
	_repPkt[0] = START_CMD;
	_txIndex = 1;
	_pktType = START_CMD;
	// attach the cmd + REPLY_FLAG
	_repPkt[_txIndex++] = cmd | REPLY_FLAG;
	// put the space for the overall len - 1 byte
	_repPkt[_txIndex++] = 0;
	// add the number of parameters
	_repPkt[_txIndex++] = numParam;
	
	// if no parameters
	if (numParam == 0) {
		_repPkt[_txIndex++] = END_CMD;
		_repPkt[2] = _txIndex;
		// write out the data to companion chip
		write(_repPkt, _txIndex);
	}
}

/* -----------------------------------------------------------------
* Prepares the _repPkt to be sent as a new data pkt to companion chip
* It requires also the number of attached parameters.
*/
void UartDrv::sendDataPkt(uint8_t cmd){
	// init the buffer with DATA_PKT
	_repPkt[0] = DATA_PKT;
	_txIndex = 1;
	_pktType = DATA_PKT;
	// attach the cmd + the complement of the REPLY_FLAG
	_repPkt[_txIndex++] = cmd | REPLY_FLAG;
	// put the space for the overall len - 4 byte
	_repPkt[_txIndex++] = 0;
	_repPkt[_txIndex++] = 0;
	_repPkt[_txIndex++] = 0;
	_repPkt[_txIndex++] = 0;
}

void UartDrv::sendParam(uint8_t *param, uint8_t param_len, uint8_t lastParam){
	// NOTE for each append be sure to not insert more than buffer size byte in buffer or append function
	// won't let me do that.
	if (_txIndex + param_len + 1 < MAX_CMD_LEN) {
		// append the typ. parameter size (1 byte)
		_repPkt[_txIndex++] = param_len;

		// append all the parameters from the list to the buffer
		memcpy(&_repPkt[_txIndex], param, param_len);
		_txIndex += param_len;
	}

	// if this is the last parameter
	if (lastParam == 1) {
		_txBufFinalizePacket();
	}
}

void UartDrv::sendParam(uint16_t param, uint8_t lastParam){
	// NOTE for each append be sure to not insert more than buffer size bytes in buffer or append function
	// won't let me do that.
	if (_txIndex + 3 < MAX_CMD_LEN) {
		// since each parameter is two bytes wide, add 2 as the parameter size
		_repPkt[_txIndex++] = 2;
		// then add the parameter (big-endian mode)
		_repPkt[_txIndex++] = (uint8_t)((param & 0xff00) >> 8);
		_repPkt[_txIndex++] = (uint8_t)(param & 0xff);
	}

	// if this is the last parameter
	if (lastParam == 1) {
		_txBufFinalizePacket();
	}
}

void UartDrv::appendByte(uint8_t data, uint8_t lastParam){
	_txBufAppendByte(data);
	
	// if this is the last parameter
	if (lastParam == 1) {
		_txBufFinalizePacket();
	}
}

void UartDrv::appendBuffer(const uint8_t *data, uint32_t len){
	// calculate and send the overall length
	uint32_t totalLen = _txIndex + len + 1; // 1 byte more to store end command byte
	if(_pktType == DATA_PKT){ // len is 4 bytes
		_repPkt[2] = totalLen & 0xFF;
		_repPkt[3] = (totalLen >> 8) & 0xFF;
		_repPkt[4] = (totalLen >> 16) & 0xFF;
		_repPkt[5] = (totalLen >> 24) & 0xFF;
	}
	else // len is 1 byte
		_repPkt[2] = totalLen;
		
	_writeBuffPkt(data, totalLen, _txIndex);
}

void UartDrv::clearRx(void){
	memset(_raw_pkt, 0, BUFFER_SIZE);
}


void UartDrv::_writeBuffPkt(const uint8_t *data, uint32_t len, uint8_t offset){	
	len -= offset; // remove size of the bytes already in the buffer;

	// if len is greater than _interface.repPkt size, split message in different writes
	uint16_t availDataSpace = MAX_CMD_LEN - (offset + 1); // don't consider header (offset) and footer for the first write
	uint32_t byteWritten = 0;
	while(len > availDataSpace){
		// be sure to send a BUFFER_SIZE multiple in order to avoid 0 padding for this communication
		// in this way control chip will not notice difference between sending the whole message and split message in multiple writes
		uint16_t writeLen = ((uint16_t)(availDataSpace + 1) / BUFFER_SIZE) * BUFFER_SIZE;
		memcpy((_repPkt + _txIndex), (data + byteWritten), writeLen - offset); // remove the already used space for header
		write(_repPkt, writeLen);
		byteWritten += writeLen;
		len -= writeLen;
		_txIndex = 0;
		availDataSpace = MAX_CMD_LEN - 1; // take into account space for end command
		offset = 0; // don't consider offset for next iterations	
	}
	
	// at this point _interface.repPkt has enough space to store the whole message (or the remaining part)
	memcpy((_repPkt + _txIndex), (data + byteWritten), len);
	_txIndex += len;
	_repPkt[_txIndex - 1] = END_CMD;

	write(_repPkt, _txIndex);
}

#endif
#endif //COMMUNICATION_LIB_ENABLED