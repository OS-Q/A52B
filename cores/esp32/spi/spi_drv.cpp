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

#include "Arduino.h"
#include "spi_drv.h"
#include "driver/periph_ctrl.h"

volatile bool SpiDrv::_rxEnabled = true;
volatile uint8_t SpiDrv::_processing;
volatile bool SpiDrv::_sendingWait;
volatile bool SpiDrv::_dataArrived = false;
uint8_t SpiDrv::_raw_pkt[2][BUFFER_SIZE];
uint8_t SpiDrv::_recvByte = 0;

spi_slave_transaction_t trn;
spi_slave_transaction_t mtrn;
uint8_t rx_buffer[BUFFER_SIZE];
uint8_t emptyPacket[BUFFER_SIZE];

// -------------------------------------------------- CALLBACK --------------------------------------------------
/*
 * ---------------------------------------------------- SPI transfer complete handler
 */
 
void transInt(spi_slave_transaction_t * trans){
	// lower handshake pin to signal that our buffer is empty - we are not ready to receive a new transaction
	GPIO.out_w1tc = ((uint32_t)1); // digitalWrite(PIN_SPI_HANDSHAKE, LOW);

	uint8_t transferredBytes = (trans->trans_len / 8);

	if(transferredBytes == 0){ // occasionally occurs
		// enable next packet reception
		mtrn.length=128*8;
		mtrn.rx_buffer=rx_buffer;
		mtrn.tx_buffer=emptyPacket;
		// timeout 0 helps if another transaction was already being set outside
		spi_slave_queue_trans(HSPI_HOST, &mtrn, 0);
		SpiDrv::_rxEnabled = true;
		return;
	}

	if(((uint8_t*)trans->rx_buffer)[0] != 0){
		// new data are available for processing
		memcpy((uint8_t*)SpiDrv::_raw_pkt[0], trans->rx_buffer, transferredBytes);
		SpiDrv::_dataArrived = true;
		SpiDrv::_recvByte = transferredBytes;
		SpiDrv::enableProcess();
	}

	SpiDrv::_sendingWait = false;
	SpiDrv::_rxEnabled = false;
}

void setupInt(spi_slave_transaction_t * trans){
	// raise handshake pin to signal that our buffer is full - we are ready to receive a new transaction
	GPIO.out_w1ts = ((uint32_t)1); // digitalWrite(PIN_SPI_HANDSHAKE, HIGH);
}

// ----------------------------------------------- SpiDrv -----------------------------------------------
// ----------------------------------------------------- PRIVATE
/*
* Prepare and send a data packet with command and data passed as argument.
*
* param cmdCode: command code that will be put in packet header
* param data: pointer to the data to be sent
* param len: data length
*/
void SpiDrv::_writeBuffPkt(const uint8_t *data, uint32_t len, uint8_t offset){	
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

// ----------------------------------------------------- PUBLIC
/*
*
*/
SpiDrv::SpiDrv()
{
  _processing = 0;
  longCmd.totalLen = 0;
  recPacket.receivedLen = 0;
  recPacket.endReceived = true;
}


/*
*
*/
void SpiDrv::begin()
{
	pinMode(PIN_SPI_SR, OUTPUT);
	digitalWrite(PIN_SPI_SR, HIGH);
	pinMode(SS, INPUT);
	uint32_t timeout = millis() + 350;
	while((digitalRead(SS) != HIGH) && (millis() < timeout));
	Serial.begin(115200); // part of workaround to use GPIO0 as handshake pin.
	//SPISlave.begin(MOSI, MISO, SCK, SS, PIN_SPI_HANDSHAKE);
	//Configuration for the SPI bus
    spi_bus_config_t buscfg={
        mosi_io_num : (gpio_num_t)g_APinDescription[MOSI].ulPin,
        miso_io_num : (gpio_num_t)g_APinDescription[MISO].ulPin,
        sclk_io_num : (gpio_num_t)g_APinDescription[SCK].ulPin
    };

    //Configuration for the SPI slave interface
    spi_slave_interface_config_t slvcfg={
        spics_io_num : (gpio_num_t)g_APinDescription[SS].ulPin,
        flags : 0,
        queue_size : 1,
        mode : 0,
        post_setup_cb : setupInt,
        post_trans_cb : transInt
    };

	//Configuration for the handshake line
	gpio_config_t io_conf;
	io_conf.pin_bit_mask= (1); // PIN_SPI_HANDSHAKE
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.intr_type = GPIO_INTR_DISABLE;

	//Configure handshake line as output
	gpio_config(&io_conf);

    //Enable pull-ups on SPI lines so we don't detect rogue pulses when no master is connected.
    gpio_set_pull_mode((gpio_num_t)g_APinDescription[MOSI].ulPin, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode((gpio_num_t)g_APinDescription[SCK].ulPin, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode((gpio_num_t)g_APinDescription[SS].ulPin, GPIO_PULLUP_ONLY);

    //Initialize SPI slave interface
    esp_err_t ret = spi_slave_initialize(HSPI_HOST, &buscfg, &slvcfg, 1);
    assert(ret==ESP_OK);
	
	// set a transaction in the queue to be able to receive messages from master
	memset(emptyPacket, 0, 128);

	trn.length=128*8;
    trn.rx_buffer=rx_buffer;
	trn.tx_buffer=emptyPacket;
	
	spi_slave_queue_trans(HSPI_HOST, &trn, 0);
	// The following workaround is needed to use GPIO0 as handshake pin in SPI communication.
	// It seems that if SPISlave library is used GPIO0 is always HIGH,
	// but if serial communication is initialized before and after the SPISlave.begin function this issue does not occur.
	// Why?
	Serial.begin(115200);
	Serial.end();
	periph_module_disable( PERIPH_UART0_MODULE );
	delay( 20 );
	periph_module_enable( PERIPH_UART0_MODULE );
	periph_module_disable( PERIPH_UART1_MODULE );
	delay( 20 );
	periph_module_enable( PERIPH_UART1_MODULE );
	periph_module_disable( PERIPH_UART2_MODULE );
	delay( 20 );
	periph_module_enable( PERIPH_UART2_MODULE );

	digitalWrite(PIN_SPI_SR, LOW);
}

/*
*
*/
void SpiDrv::enableRx(){
	if(!_rxEnabled){
	  mtrn.length=128*8;
	  mtrn.rx_buffer=rx_buffer;
	  mtrn.tx_buffer=emptyPacket;
	  // timeout 0 helps if another transaction was already being set outside
	  if(spi_slave_queue_trans(HSPI_HOST, &mtrn, 0) == 0)
		_rxEnabled = true;
	}
	if(_suspended){
		_suspended = false;
		xTaskResumeAll();
	}
}

/*
*
*/
int8_t SpiDrv::read(tsNewCmd *_pkt){
  uint8_t* pktPtr = (uint8_t*)_raw_pkt[0];
  if(_processing > 0){ // if there's more than a packet to process, start with the oldest one
	pktPtr = (uint8_t*)_raw_pkt[1];
	_isBufferValid = true; // signal that _raw_pkt[0] still contains valid data
  }
  // ---------------------------------------------------------------------------- receiving long command packets
  if (recPacket.receivedLen > 0) {
	uint8_t endOffset;

	// check if END_CMD is in the last 4 received bytes (in the worst case there's a 4-byte 0 padding)
	for (uint8_t i = 1; i <= 4; i++) {
	  if (pktPtr[_recvByte - i] == END_CMD) {
		endOffset = _recvByte - i;

		if (recPacket.receivedLen + endOffset == recPacket.totalLen - 1) {
		  recPacket.endReceived = true;
		  notifyAck = true;
		  _pkt->cmdType = recPacket.cmdType;
		  _pkt->cmd = recPacket.cmd;
		  _pkt->nParam = endOffset;
		  _pkt->totalLen = recPacket.totalLen - DATA_HEAD_FOOT_LEN;
		  _pkt->dataPtr = &pktPtr[0];

		  recPacket.receivedLen = recPacket.totalLen = 0;

		  return 0;
		}
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
		  enableRx(); // enable next packet reception
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
		enableRx(); // enable next packet reception
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
	  enableRx(); // enable next packet reception

	  _multiPktCmd = 1;

	  return -1;
	}
	else {
	  if (pktPtr[_pkt->totalLen - 1] != END_CMD){
		enableRx(); // enable next packet reception
		return -1;
	  }
	  return 0;
	}
  }
  // ---------------------------------------------------------------------------- decoding data packets
  else if (pktPtr[0] == DATA_PKT){
	_pkt->cmdType = recPacket.cmdType = pktPtr[0];
	_pkt->cmd = recPacket.cmd = pktPtr[1];
	recPacket.receivedLen = _recvByte;
	recPacket.totalLen = (pktPtr[5] << 24) + (pktPtr[4] << 16) + (pktPtr[3] << 8) + pktPtr[2];
	recPacket.endReceived = false;
	_pkt->totalLen = recPacket.totalLen - DATA_HEAD_FOOT_LEN;
		// 32 byte - (cmdType (1 byte) + cmd (1 byte) + totalLen (4 byte)) = 32 - 6 = 26
	_pkt->nParam = recPacket.receivedLen - (sizeof(recPacket.cmdType) + sizeof(recPacket.cmd) + sizeof(recPacket.totalLen)); //_pkt->nParam = 26;
	_pkt->dataPtr = &pktPtr[6];

	// check if END_CMD is in the last 4 received bytes (at least there's a 4-byte 0 padding)
	for(uint8_t i = 1; i <= 4; i++){
	  if(pktPtr[_recvByte - i] == END_CMD){
		recPacket.endReceived = true;
		recPacket.receivedLen = recPacket.totalLen = 0;
		_pkt->nParam = 255; // this value is used as flag that indicates that data packet is all contained in a single packet
		break;
	  }
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

/*
*
*/
void SpiDrv::write(uint8_t *_rep, uint32_t len){
  // safety check
  if(_suspended){
	xTaskResumeAll();
	_suspended = false;
  }

  // first of all check if the spi buffer needs to be empty
  // (probably it has been filled before in order to be able to receive new packet from master)
  if(_rxEnabled == true){
	_dataArrived = false; // used to check if valid data arrive in the next transaction
	// if the SPI buffer was already been set, trigger a read to flush it  
	_sendingWait = true;
	uint32_t timeout = millis() + 1000;
	_setSR(HIGH);
	while(_sendingWait && timeout > millis());
	_setSR(LOW);
	// save packet received during this transaction for further processing, if needed
	if(_dataArrived)
	  memcpy((void*)_raw_pkt[1], (void*)_raw_pkt[0], 128);
	}
  
  if(_isBufferValid){
	  // in the last processing we parsed _raw_pkt[1]. Save _raw_pkt[0] into _raw_pkt[1] to avoid _raw_pkt[0] be overridden by the next transaction
	  _isBufferValid = false;
	  memcpy((void*)_raw_pkt[1], (void*)_raw_pkt[0], 128);
  }
  
  // calculate the number of packets to be sent - each packet is 32 byte long
  uint8_t nPacket = (uint8_t)(len >> 5);
  uint8_t remainingByte = 0;
  uint32_t pktOffset = 0;
  uint32_t t = 0;
  uint16_t sentByte = 0 ;

  if ((remainingByte = (len % 32)) > 0)
	nPacket++;

  while (nPacket > 0) {
	if(nPacket == 1 && remainingByte > 0) {
	  trn.length=32*8;
      trn.rx_buffer=rx_buffer;
	  trn.tx_buffer=(uint8_t *)(_rep + pktOffset);
	  esp_err_t err_code = spi_slave_queue_trans(HSPI_HOST, &trn, SPI_TIMEOUT / portTICK_PERIOD_MS);
	  _rxEnabled = true;
	  if(err_code != ESP_OK){
		#ifdef DEBUG_SERIALE
		  Serial.println("Stop waiting write 1");
		  Serial.println("Byte written so far: " + String(sentByte));
		#endif
		return;
	  }
	  sentByte += remainingByte;
	}
	else{
	  trn.length = 32*8;
      trn.rx_buffer = rx_buffer;
	  trn.tx_buffer = (uint8_t *)(_rep + pktOffset);
	  esp_err_t err_code = spi_slave_queue_trans(HSPI_HOST, &trn, SPI_TIMEOUT / portTICK_PERIOD_MS);
	  _rxEnabled = true;
	  if(err_code != ESP_OK){
		#ifdef DEBUG_SERIALE
		  Serial.println("Stop waiting write");
		  Serial.println("Byte written so far: " + String(sentByte));
		#endif
		return;
	  }
	  sentByte += 32;
	}

	// between setting the buffer and raising SR high, master could have already read the message during a write.
	// in this case don't set SR but go ahead.
	if(_rxEnabled){
	  _sendingWait = true;
	  // set the ISR trigger signal to fire a read process
	  _setSR(HIGH);

	  pktOffset += 32;
	  nPacket--;

	  t = millis() + SPI_TIMEOUT;
	  while (_sendingWait) {
  
		if (t <= millis()) {
  #ifdef DEBUG_SERIALE
		  Serial.println("Stop waiting write 3");
		  Serial.println("Byte written so far: " + String(sentByte));
  #endif
		  // lower the ISR signal to let the 328 know that ESP is already idle
		  _setSR(LOW);
		  return;
		}
	  }
	  // lower the ISR signal to let the 328 know that ESP is already idle
	  _setSR(LOW);
	}
  }
  vTaskSuspendAll();
  _suspended = true;
  #ifdef DEBUG_SERIALE
//  Serial.println("byte written: " + String(sentByte));
  #endif
}

/* Cmd Struct Message
*  _________________________________________________________________________________ 
* | START CMD | C/R  | CMD  |[TOT LEN]| N.PARAM | PARAM LEN | PARAM  | .. | END CMD |
* |___________|______|______|_________|_________|___________|________|____|_________|
* |   8 bit   | 1bit | 7bit |  8bit   |  8bit   |   8bit    | nbytes | .. |   8bit  |
* |___________|______|______|_________|_________|___________|________|____|_________|
*/
/* Data Struct Message
* __________________________________________________________________________________
*| DATA PKT  | C/R  | CMD  |  OVERALL  |  SOCKET  |  PAYLOAD  | .. |  END DATA PKT |
*|___________|______|______|___ LEN____|__________|___________|____|_______________|
*|   8 bit   | 1bit | 7bit |   32bit   |  8 bit   |   nbytes  | .. |      8bit     |
*|___________|______|______|___________|__________|___________|____|_______________|
*/


/* -----------------------------------------------------------------
* Prepares the _repPkt to be sent as a new command to companion chip
* It requires also the number of attached parameters.
*/
void SpiDrv::sendCmd(uint8_t cmd, uint8_t numParam){
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
void SpiDrv::sendDataPkt(uint8_t cmd){
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

/* -----------------------------------------------------------------
* Add parameters to the command buffer for the companion chip
* It requires also the lastParam parameter to understand which is the last one.
*/
void SpiDrv::sendParam(uint16_t param, uint8_t lastParam){
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

/* -----------------------------------------------------------------
* Add a list of parameters to the command buffer for the companion chip (overload)
* It requires the parameter's list (*param), the length of each parameter (param_len)
* and also the lastParam parameter to understand if this is the last one.
*/
void SpiDrv::sendParam(uint8_t *param, uint8_t param_len, uint8_t lastParam){
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

/* -----------------------------------------------------------------
* Add byte to the command buffer for the companion chip
* It requires also the lastParam parameter to understand which is the last one.
*/
void SpiDrv::appendByte(uint8_t data, uint8_t lastParam){
	_txBufAppendByte(data);

	// if this is the last parameter
	if (lastParam == 1) {
		_txBufFinalizePacket();
	}
}

/* -----------------------------------------------------------------
* Add a buffer to the command for the companion chip and send the so created packet
*/
void SpiDrv::appendBuffer(const uint8_t *data, uint32_t len){
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

#endif
#endif //COMMUNICATION_LIB_ENABLED