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

#include "Arduino.h"
#include "wiring_private.h"

#define DMA_CHANNEL 0
__attribute__((__aligned__(16))) static DmacDescriptor // 128 bit alignment
    _descriptor[1] SECTION_DMAC_DESCRIPTOR,
    _writeback[1]  SECTION_DMAC_DESCRIPTOR;

volatile bool transInProgress = false;
volatile uint8_t UartDrv::_interruptReq = 0;

void interruptFunction(){
	if((PORT->Group[PORTA].IN.reg & (1ul << 21)) != 0){ //if (digitalRead(SLAVEREADY) == HIGH) {
		transInProgress = true;
	}
	else{
		UartDrv::_interruptReq++;
		PORT->Group[PORTA].OUTCLR.reg = (1ul << 18); //digitalWrite(SLAVESELECT, LOW);
		transInProgress = false;
	}
}

/* -----------------------------------------------------------------
* Class constructor
*/
UartDrv::UartDrv(void){
	uartIsr = NULL;
}

void UartDrv::begin(){
	NVIC_DisableIRQ(SERCOM5_IRQn);
	NVIC_ClearPendingIRQ(SERCOM5_IRQn);

	// configure DMA for Serial communication

	PM->AHBMASK.bit.DMAC_       = 1; // Initialize DMA clocks
	PM->APBBMASK.bit.DMAC_      = 1;
	DMAC->CTRL.bit.DMAENABLE    = 0; // Disable DMA controller
	DMAC->CTRL.bit.SWRST        = 1; // Perform software reset
	DMAC->BASEADDR.bit.BASEADDR = (uint32_t)_descriptor;
	DMAC->WRBADDR.bit.WRBADDR   = (uint32_t)_writeback;
	// Re-enable DMA controller with all priority levels
	DMAC->CTRL.reg = DMAC_CTRL_DMAENABLE | DMAC_CTRL_LVLEN(0xF);

	// Perform a reset for the allocated channel
	DMAC->CHID.bit.ID         = DMA_CHANNEL;
	DMAC->CHCTRLA.bit.ENABLE  = 0;
	DMAC->CHCTRLA.bit.SWRST   = 1;
	// Clear software trigger
	DMAC->SWTRIGCTRL.reg     &= ~(1 << DMA_CHANNEL);
	// Configure default behaviors
	DMAC->CHCTRLB.bit.LVL     = 0;
	DMAC->CHCTRLB.bit.TRIGSRC = 0x0B; //SERCOM5_DMAC_ID_RX;
	DMAC->CHCTRLB.bit.TRIGACT = 0x02; //DMAC_CHCTRLA_TRIGACT_BEAT;

	// set descriptor
	_descriptor->BTCTRL.bit.VALID    = true;
	_descriptor->BTCTRL.bit.EVOSEL   = 0x00; //DMA_EVENT_OUTPUT_DISABLED;
	_descriptor->BTCTRL.bit.BLOCKACT = 0; //DMA_BLOCK_ACTION_NOACT;
	_descriptor->BTCTRL.bit.BEATSIZE = 0x0; //DMA_BEAT_SIZE_BYTE;
	_descriptor->BTCTRL.bit.SRCINC   = false;
	_descriptor->BTCTRL.bit.DSTINC   = true;
	_descriptor->BTCTRL.bit.STEPSEL  = 0x0; //DMA_STEPSEL_DST;
	_descriptor->BTCTRL.bit.STEPSIZE = 0x0; //DMA_ADDRESS_INCREMENT_STEP_SIZE_1;
	_descriptor->BTCNT.reg           = BUF_SIZE; //count;
	_descriptor->SRCADDR.reg         = (uint32_t)(&SERCOM5->USART.DATA.reg);
	_descriptor->DSTADDR.reg         = (uint32_t)_rxBuf + sizeof(_rxBuf);
	_descriptor->DESCADDR.reg        = 0;

	DMAC->CHID.bit.ID    = DMA_CHANNEL;
    DMAC->CHCTRLA.bit.ENABLE = 1; // Enable the transfer channel

	// configure UART
	pinPeripheral(PIN_SERIAL2_RX, g_APinDescription[PIN_SERIAL2_RX].ulPinType);
	pinPeripheral(PIN_SERIAL2_TX, g_APinDescription[PIN_SERIAL2_TX].ulPinType);

	//Setting clock
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID( GCM_SERCOM5_CORE ) | // Generic Clock 0 (SERCOM5)
					  GCLK_CLKCTRL_GEN_GCLK0 | // Generic Clock Generator 0 is source
					  GCLK_CLKCTRL_CLKEN ;

	// Wait for synchronization
	while ( GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY ){}
	// Start the Software Reset
	SERCOM5->USART.CTRLA.bit.SWRST = 1 ;

	while ( SERCOM5->USART.CTRLA.bit.SWRST || SERCOM5->USART.SYNCBUSY.bit.SWRST )
	{
	// Wait for both bits Software Reset from CTRLA and SYNCBUSY coming back to 0
	}
	  //Setting the CTRLA register
	SERCOM5->USART.CTRLA.reg =	SERCOM_USART_CTRLA_MODE(0x1) | // UART_INT_CLOCK
                SERCOM_USART_CTRLA_SAMPR(0x1); // SAMPLE_RATE_x16

    // Asynchronous fractional mode (Table 24-2 in datasheet)
    //   BAUD = fref / (sampleRateValue * fbaud)
    // (multiply by 8, to calculate fractional piece)
    uint32_t baudTimes8 = (SystemCoreClock * 8) / (16 * UART_BAUDRATE);
    SERCOM5->USART.BAUD.FRAC.FP   = (baudTimes8 % 8);
    SERCOM5->USART.BAUD.FRAC.BAUD = (baudTimes8 / 8);

	//Setting the CTRLA register
	SERCOM5->USART.CTRLA.reg |=	SERCOM_USART_CTRLA_FORM(0) | // No parity
				0x1 << SERCOM_USART_CTRLA_DORD_Pos; // LSB_FIRST

	//Setting the CTRLB register
	SERCOM5->USART.CTRLB.reg |=	SERCOM_USART_CTRLB_CHSIZE(0) | // 8 bit
				 0 << SERCOM_USART_CTRLB_SBMODE_Pos; // 1 stop bit

	//Setting the CTRLA register
	SERCOM5->USART.CTRLA.reg |=	SERCOM_USART_CTRLA_TXPO(UART_TX_PAD_2) |
				SERCOM_USART_CTRLA_RXPO(SERCOM_RX_PAD_3);

	// Enable Transceiver and Receiver
	SERCOM5->USART.CTRLB.reg |= SERCOM_USART_CTRLB_TXEN | SERCOM_USART_CTRLB_RXEN ;

	//Setting  the enable bit to 1
	SERCOM5->USART.CTRLA.bit.ENABLE = 0x1u;

	//Wait for then enable bit from SYNCBUSY is equal to 0;
	while(SERCOM5->USART.SYNCBUSY.bit.ENABLE);

	// this sets the sr pin from companion chip to us and attaches to it a RISING interrupt.
	pinMode(SLAVEREADY, INPUT);
	pinMode(SLAVEHANDSHAKE, INPUT);
	attachInterrupt(digitalPinToInterrupt(SLAVEREADY), interruptFunction, CHANGE);
	pinMode(SLAVESELECT, OUTPUT);
	digitalWrite(SLAVESELECT, HIGH);

	// wait for synchronization signals
	uint32_t timeout = millis() + 350;
	while((digitalRead(SLAVEREADY) != HIGH) && (millis() < timeout));
	timeout = millis() + 100;
	while((digitalRead(SLAVEREADY) != LOW) && (millis() < timeout));
}

void UartDrv::end(){
	// Start the Software Reset
	SERCOM5->USART.CTRLA.bit.SWRST = 1 ;

	while ( SERCOM5->USART.CTRLA.bit.SWRST || SERCOM5->USART.SYNCBUSY.bit.SWRST )
	{
	// Wait for both bits Software Reset from CTRLA and SYNCBUSY coming back to 0
	}
}

/* -----------------------------------------------------------------
* Prepares the _txBuf to be sent as a new command to companion chip
* It requires also the number of attached parameters.
*/
bool UartDrv::sendCmd(uint8_t cmd, uint8_t numParam){
	// init the buffer with the START_CMD
	_txBuf[0] = START_CMD;
	_txIndex = 1;
	_pktType = START_CMD;
	// attach the cmd + the complement of the REPLY_FLAG
	_txBuf[_txIndex++] = cmd & ~(REPLY_FLAG);
	// put the space for the overall len - 1 byte
	_txBuf[_txIndex++] = 0;
	// add the number of parameters
	_txBuf[_txIndex++] = numParam;
	
	// if no parameters
	if (numParam == 0) {
		// add the END_CMD to the buffer
		_txBuf[_txIndex++] = END_CMD;
		// len is 1 byte
		_txBuf[2] = _txIndex;
		// write out the data to companion chip
		return _writeData(_txBuf, _txIndex);
	}

	return true;
}

/* -----------------------------------------------------------------
* Prepares the _txBuf to be sent as a new data pkt to companion chip
*/
void UartDrv::sendDataPkt(uint8_t cmd){
	// init the buffer with the DATA_PKT
	_txBuf[0] = DATA_PKT;
	_txIndex = 1;
	_pktType = DATA_PKT;
	// attach the cmd + the complement of the REPLY_FLAG
	_txBuf[_txIndex++] = cmd & ~(REPLY_FLAG);
	// put the space for the overall len - 4 byte
	_txBuf[_txIndex++] = 0;
	_txBuf[_txIndex++] = 0;
	_txBuf[_txIndex++] = 0;
	_txBuf[_txIndex++] = 0;
}

/* -----------------------------------------------------------------
* Add a list of parameters to the command buffer for the companion chip (overload)
* It requires the parameter's list (*param), the length of each parameter (param_len)
* and also the lastParam parameter to understand if this is the last one.
*/
bool UartDrv::sendParam(uint8_t *param, uint8_t param_len, uint8_t lastParam){
	if(_txIndex + param_len + 1 < TXBUF_LEN){
		// append parameter size (1 byte)
		_txBuf[_txIndex++] = param_len;
		// append all the parameters from the list to the buffer
		memcpy(&_txBuf[_txIndex], param, param_len);
		_txIndex += param_len;
	}
	else
		return false;

	// if this is the last parameter
	if (lastParam == 1) {
		return _txBufFinalizePacket();
	}

	return true;
}

/* -----------------------------------------------------------------
* Add parameters to the command buffer for the companion chip
* It requires also the lastParam parameter to understand which is the last one.
*/
bool UartDrv::sendParam(uint16_t param, uint8_t lastParam){
	if(_txIndex + 3 < TXBUF_LEN){
		// since each parameter is two bytes wide, add 2 as the parameter size
		_txBuf[_txIndex++] = 2;
		// then add the parameter (big-endian mode) 
		_txBuf[_txIndex++] = (uint8_t)((param & 0xff00) >> 8);
		_txBuf[_txIndex++] = (uint8_t)(param & 0xff);
	}
	else
		return false;

	// if this is the last parameter
	if (lastParam == 1) {
		return _txBufFinalizePacket();
	}

	return true;
}

/* -----------------------------------------------------------------
* Add byte to the command buffer for the companion chip
* It requires also the lastParam parameter to understand which is the last one.
*/
bool UartDrv::appendByte(uint8_t data, uint8_t lastParam){
	_txBufAppendByte(data);
	
	// if this is the last parameter
	if (lastParam == 1) {
		return _txBufFinalizePacket();
	}

	return true;
}

/* -----------------------------------------------------------------
* Add a buffer to the command for the companion chip and send the so created packet
*/
bool UartDrv::appendBuffer(const uint8_t *data, uint32_t len){
	// calculate and send the overall length
	uint32_t totalLen = _txIndex + len + 1; // 1 byte more to store end command byte
	if(_pktType == DATA_PKT){ // len is 4 bytes
		_txBuf[2] = totalLen & 0xFF;
		_txBuf[3] = (totalLen >> 8) & 0xFF;
		_txBuf[4] = (totalLen >> 16) & 0xFF;
		_txBuf[5] = (totalLen >> 24) & 0xFF;
	}
	else // len is 1 byte
		_txBuf[2] = totalLen;
		
	// copy memory address
	memcpy(&_txBuf[_txIndex], &data, 4);

	return _writeBuffPkt(_txBuf, totalLen, _txIndex);
}

/* -----------------------------------------------------------------
* Read data from the companion chip after an ISR event
* params: uint8_t* buffer:	data buffer to store the received data
*         bool first: if true (default) recursion will be enabled if needed
* 
* return: (uint16_t)
*		   byte read.
*/
uint16_t UartDrv::readDataISR(uint8_t *buffer, bool first){
	while(transInProgress); // wait for the current transmission to be complete
	// read data
	DMAC->CHID.bit.ID    = DMA_CHANNEL;
	DMAC->CHCTRLA.bit.ENABLE  = 0;
	// get number of bytes transferred
	uint32_t total_size = _descriptor[0].BTCNT.reg;
	uint32_t write_size = _writeback[0].BTCNT.reg;
	uint16_t available = total_size - write_size;

	// enable DMA channel to be able to receive new data
	DMAC->CHCTRLA.bit.ENABLE  = 1;
	memcpy(buffer, _rxBuf, available);
	// it is rare, but it could happen that DMA has not completed its transfer when this function is called
	if(available == 0){
		if(first) // allow only one level of recursion
			readDataISR(buffer, false);
	}
	// signal that we are ready to receive a new packet
	_ack();
	return available;
}

/* -----------------------------------------------------------------
* Writes to the companion a pre-allocated buffer (mainly used for _txBuf)
*
* params: uint8_t* data:	the buffer pointer
*		  uint16_t len:		number of parameters to read
*/
bool UartDrv::_writeData(uint8_t *data, uint32_t len){
	PORT->Group[PORTA].OUTCLR.reg = (1ul << 18); //digitalWrite(SLAVESELECT, LOW);
	uint32_t t = millis() + 10;
	bool doNotRaise = false;
	size_t i = 0, current_size = 0;
	// wait for the current transmission to be complete if any
	while(transInProgress && (t > millis())){
		// if a transmission happened, raise SS only when this packet has been read
		doNotRaise = true;
	}
	if(transInProgress) // companion is still writing - return with an error
		return false;
	uint32_t byteWritten = 0;
	// send all the packets
	while (len > 0) {
		if(len > UART_MAX_TX_SIZE){
			// make sure to send a UART_MAX_TX_SIZE multiple as to avoid loosing data on the other micro
			i = UART_MAX_TX_SIZE;
		}
		else{ // write the last chunk
			i = len;
		}
		current_size = i;
		while(i){
			// Wait for data register to be empty
			while(!SERCOM5->USART.INTFLAG.bit.DRE);
			//Put data into DATA register
			SERCOM5->USART.DATA.reg = (uint16_t)(data[byteWritten + (current_size - i)]);
			i--;
		}
		byteWritten += current_size;
		len -= current_size;
	}
	if(!doNotRaise && _interruptReq == 0){
		PORT->Group[PORTA].OUTSET.reg = (1ul << 18); //digitalWrite(SLAVESELECT, HIGH);
		
		// check ack
		uint32_t timeout = millis() + 10000;
		// wait for the handshake to go HIGH
		while(((PORT->Group[PORTA].IN.reg & (1ul << 15)) == 0) && timeout > millis()); // while(digitalRead(SLAVEHANDSHAKE) == 0 && timeout > millis());
		if(timeout <= millis()){
			return false;
		}
	}
	return true;
}

bool UartDrv::_writeBuffPkt(uint8_t *data, uint32_t len, uint8_t dataOffset){	
	PORT->Group[PORTA].OUTCLR.reg = (1ul << 18); //digitalWrite(SLAVESELECT, LOW);
	bool ret = true;
	uint16_t j = 0;
	bool firstWrite = true;
	size_t i = 0, current_size = 0;
	bool doNotRaise = false;

	uint8_t* addrMem = (uint8_t*)((uint32_t)(data[dataOffset] + (data[dataOffset + 1] << 8) + (data[dataOffset + 2] << 16) + (data[dataOffset + 3] << 24)));

	// write out the data
	while (len > 0){
		uint32_t t = millis() + 10;
		// wait for the current transmission to be complete if any
		while(transInProgress && (t > millis())){
			// if a transmission happened, raise SS only when this packet has been read
			doNotRaise = true;
		}
		if(transInProgress){ // companion is still writing - return with an error
			PORT->Group[PORTA].OUTSET.reg = (1ul << 18); //digitalWrite(SLAVESELECT, HIGH);
			return false;
		}

		if(firstWrite){
			// send header
			i = dataOffset;
			while(i){
				// Wait for data register to be empty
				while(!SERCOM5->USART.INTFLAG.bit.DRE);
				//Put data into DATA register
				SERCOM5->USART.DATA.reg = (uint16_t)(data[dataOffset - i]);
				i--;
			}
			firstWrite = false;
		}
		if(len > UART_MAX_TX_SIZE){
			// data are greater than UART_MAX_TX_SIZE. Multiple writes are requested
			i = UART_MAX_TX_SIZE - dataOffset;
		}
		else{
			// data fit UART_MAX_TX_SIZE
			i = len - dataOffset - 1;
		}
		current_size = i;
		while(i){
			// Wait for data register to be empty
			while(!SERCOM5->USART.INTFLAG.bit.DRE);
			//Put data into DATA register
			SERCOM5->USART.DATA.reg = (uint16_t)(addrMem[j + (current_size - i)]);
			i--;
		}
		j += current_size;
		len -= current_size + dataOffset; // consider also dataOffset that we previously transferred
		if(len == 1){
			// send END_CMD
			while(!SERCOM5->USART.INTFLAG.bit.DRE);
			SERCOM5->USART.DATA.reg = (uint16_t)END_CMD;
		
			len = 0;
		}
		dataOffset = 0;

		if(!doNotRaise && _interruptReq == 0){
			PORT->Group[PORTA].OUTSET.reg = (1ul << 18); //digitalWrite(SLAVESELECT, HIGH);
			// check ack
			uint32_t timeout = millis() + 2000;
			// wait until handshake go to HIGH
			while(((PORT->Group[PORTA].IN.reg & (1ul << 15)) == 0) && timeout > millis());	//while(digitalRead(SLAVEHANDSHAKE) == 0 && timeout > millis());
			if(timeout <= millis()){
				return false;
			}
			if(len > 0)
				PORT->Group[PORTA].OUTCLR.reg = (1ul << 18); //digitalWrite(SLAVESELECT, LOW);
		}else
		if(len > 0){
			// wait for ack
			while((PORT->Group[PORTA].IN.reg & (1ul << 15)) == 0); //while(digitalRead(SLAVEHANDSHAKE) == 0);
		}
	}
	return ret;
}

#endif
#endif //COMMUNICATION_LIB_ENABLED