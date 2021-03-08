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
#include "variant.h"
#include "wiring_private.h"

#define TX_DMA_CHANNEL 0
#define RX_DMA_CHANNEL 1
__attribute__((__aligned__(16))) static DmacDescriptor // 128 bit alignment
    _descriptor[2] SECTION_DMAC_DESCRIPTOR,
    _writeback[2]  SECTION_DMAC_DESCRIPTOR;

volatile bool transferComplete = false;

void DMAC_Handler(void) {
	DMAC->CHID.bit.ID    = RX_DMA_CHANNEL;
	uint8_t flags = DMAC->CHINTFLAG.reg;
	if(flags & DMAC_CHINTENCLR_TCMPL)
		DMAC->CHINTFLAG.bit.TCMPL = 0x01; // clear interrupt flag
	else if(flags & DMAC_CHINTENCLR_TERR)
		DMAC->CHINTFLAG.bit.TERR = 0x01; // clear interrupt flag

	transferComplete = true;
}

uint8_t _emptyBuf[BUF_SIZE] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint8_t readBuf[SPI_MAX_TX_SIZE];

volatile bool srLevel = false;
volatile bool SpiDrv::_interruptReq = false;

void interruptFunction(void){
	if (digitalRead(SLAVEREADY) == HIGH) {
		SpiDrv::_interruptReq = true;
		srLevel = HIGH;
	}
	else {
		srLevel = LOW;
	}
}

/* -----------------------------------------------------------------
* Class constructor
*/
SpiDrv::SpiDrv(void)
{
	pinMode(SLAVESELECT, OUTPUT);
	digitalWrite(SLAVESELECT, LOW);

	pinMode(SLAVEHANDSHAKE, INPUT);

	spiIsr = NULL;
}

/* -----------------------------------------------------------------
* Class begin
*/
void SpiDrv::begin()
{
	digitalWrite(SLAVESELECT, HIGH);
	_interruptReq = false;
	_txIndex = 0;

	memset(_txBuf, 0, TXBUF_LEN);

	// this sets the sr pin from companion chip to us and attaches to it a PINCHANGE interrupt.
	pinMode(SLAVEREADY, INPUT);
	attachInterrupt(digitalPinToInterrupt(SLAVEREADY), interruptFunction, CHANGE);

	// use DMA for SPI communication: Channel 0 for TX - Channel 1 for RX
	// configure DMA
	PM->AHBMASK.bit.DMAC_       = 1; // Initialize DMA clocks
	PM->APBBMASK.bit.DMAC_      = 1;
	DMAC->CTRL.bit.DMAENABLE    = 0; // Disable DMA controller
	DMAC->CTRL.bit.SWRST        = 1; // Perform software reset
	DMAC->BASEADDR.bit.BASEADDR = (uint32_t)_descriptor;
	DMAC->WRBADDR.bit.WRBADDR   = (uint32_t)_writeback;
	// Re-enable DMA controller with all priority levels
	DMAC->CTRL.reg = DMAC_CTRL_DMAENABLE | DMAC_CTRL_LVLEN(0xF);

	// configure channel 0 - TX
	// Perform a reset
	DMAC->CHID.bit.ID         = TX_DMA_CHANNEL;
	DMAC->CHCTRLA.bit.ENABLE  = 0;
	DMAC->CHCTRLA.bit.SWRST   = 1;
	// Clear software trigger
	DMAC->SWTRIGCTRL.reg     &= ~(1 << TX_DMA_CHANNEL);
	// Configure default behaviors
	DMAC->CHCTRLB.bit.LVL     = 0;
	DMAC->CHCTRLB.bit.TRIGSRC = 0x04; //SERCOM1_DMAC_ID_TX;
	DMAC->CHCTRLB.bit.TRIGACT = 0x02; //DMAC_CHCTRLA_TRIGACT_BEAT;

	// configure channel 1 - RX
	// Perform a reset for the allocated channel
	DMAC->CHID.bit.ID         = RX_DMA_CHANNEL;
	DMAC->CHCTRLA.bit.ENABLE  = 0;
	DMAC->CHCTRLA.bit.SWRST   = 1;
	// Clear software trigger
	DMAC->SWTRIGCTRL.reg     &= ~(1 << RX_DMA_CHANNEL);
	// Configure default behaviors
	DMAC->CHCTRLB.bit.LVL     = 0;
	DMAC->CHCTRLB.bit.TRIGSRC = 0x03; //SERCOM1_DMAC_ID_RX;
	DMAC->CHCTRLB.bit.TRIGACT = 0x02; //DMAC_CHCTRLA_TRIGACT_BEAT;
	DMAC->CHINTENSET.bit.TCMPL = 0x01; // enable interrupt on transfer complete
	DMAC->CHINTENSET.bit.TERR = 0x01; // enable interrupt on transfer error

	// set descriptors
	_descriptor[0].BTCTRL.bit.VALID    = true;
	_descriptor[0].BTCTRL.bit.EVOSEL   = 0x00; //DMA_EVENT_OUTPUT_DISABLED;
	_descriptor[0].BTCTRL.bit.BLOCKACT = 0; //DMA_BLOCK_ACTION_NOACT;
	_descriptor[0].BTCTRL.bit.BEATSIZE = 0x0; //DMA_BEAT_SIZE_BYTE;
	_descriptor[0].BTCTRL.bit.SRCINC   = true;
	_descriptor[0].BTCTRL.bit.DSTINC   = false;
	_descriptor[0].BTCTRL.bit.STEPSEL  = 0x0; //DMA_STEPSEL_DST;
	_descriptor[0].BTCTRL.bit.STEPSIZE = 0x0; //DMA_ADDRESS_INCREMENT_STEP_SIZE_1;

	_descriptor[1].BTCTRL.bit.VALID    = true;
	_descriptor[1].BTCTRL.bit.EVOSEL   = 0x00; //DMA_EVENT_OUTPUT_DISABLED;
	_descriptor[1].BTCTRL.bit.BLOCKACT = 0; //DMA_BLOCK_ACTION_NOACT;
	_descriptor[1].BTCTRL.bit.BEATSIZE = 0x0; //DMA_BEAT_SIZE_BYTE;
	_descriptor[1].BTCTRL.bit.SRCINC   = false;
	_descriptor[1].BTCTRL.bit.DSTINC   = true;
	_descriptor[1].BTCTRL.bit.STEPSEL  = 0x0; //DMA_STEPSEL_DST;
	_descriptor[1].BTCTRL.bit.STEPSIZE = 0x0; //DMA_ADDRESS_INCREMENT_STEP_SIZE_1;

	// enable DMA interrupt
	NVIC_EnableIRQ(DMAC_IRQn);
	NVIC_SetPriority(DMAC_IRQn, (1<<__NVIC_PRIO_BITS)-1);

	// init master SPI interface
	//intSPI.begin();
	// PIO init
	pinPeripheral(PIN_SPI1_MISO, g_APinDescription[PIN_SPI1_MISO].ulPinType);
	pinPeripheral(PIN_SPI1_SCK, g_APinDescription[PIN_SPI1_SCK].ulPinType);
	pinPeripheral(PIN_SPI1_MOSI, g_APinDescription[PIN_SPI1_MOSI].ulPinType);
	//disable SPI
	while(SERCOM1->SPI.SYNCBUSY.bit.ENABLE){
		//Waiting then enable bit from SYNCBUSY is equal to 0;
	}
	//Setting the enable bit to 0
	SERCOM1->SPI.CTRLA.bit.ENABLE = 0;
	//init SPI
	//Setting the Software Reset bit to 1
	SERCOM1->SPI.CTRLA.bit.SWRST = 1;
	//Wait both bits Software Reset from CTRLA and SYNCBUSY are equal to 0
	while(SERCOM1->SPI.CTRLA.bit.SWRST || SERCOM1->SPI.SYNCBUSY.bit.SWRST);
	// Setting NVIC
	NVIC_EnableIRQ(SERCOM1_IRQn);
	NVIC_SetPriority(SERCOM1_IRQn, SERCOM_NVIC_PRIORITY);  // set Priority

	//Setting clock
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID( GCM_SERCOM1_CORE ) | // Generic Clock 0 (SERCOMx)
					  GCLK_CLKCTRL_GEN_GCLK0 | // Generic Clock Generator 0 is source
					  GCLK_CLKCTRL_CLKEN ;

	while ( GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY ){
		// Wait for synchronization
	}
	//Setting the CTRLA register
	SERCOM1->SPI.CTRLA.reg =	SERCOM_SPI_CTRLA_MODE_SPI_MASTER |
						  SERCOM_SPI_CTRLA_DOPO(PAD_SPI1_TX) |
						  SERCOM_SPI_CTRLA_DIPO(PAD_SPI1_RX) |
						  0 << SERCOM_SPI_CTRLA_DORD_Pos; //MSBFIRST

	//Setting the CTRLB register
	SERCOM1->SPI.CTRLB.reg = SERCOM_SPI_CTRLB_CHSIZE(SPI_CHAR_SIZE_8_BITS) |
						  SERCOM_SPI_CTRLB_RXEN;	//Active the SPI receiver.
	// Init SPI clock
	//Setting the CTRLA register
	SERCOM1->SPI.CTRLA.reg |=( 0 << SERCOM_SPI_CTRLA_CPHA_Pos ) |
							( 0 << SERCOM_SPI_CTRLA_CPOL_Pos );

	//SPI frequency 8MHz
	SERCOM1->SPI.BAUD.reg = SERCOM_FREQ_REF / (2 * (SERCOM_FREQ_REF / (1 + ((F_CPU - 1) / 9500000)))) - 1;
	// enable SPI
	//Setting the enable bit to 1
	SERCOM1->SPI.CTRLA.bit.ENABLE = 1;
	while(SERCOM1->SPI.SYNCBUSY.bit.ENABLE){
	//Waiting then enable bit from SYNCBUSY is equal to 0;
	}

	// wait for synchronization signals
	uint32_t timeout = millis() + 1300;
	while((digitalRead(SLAVEREADY) != HIGH) && (millis() < timeout));
	timeout = millis() + 100;
	while((digitalRead(SLAVEREADY) != LOW) && (millis() < timeout));
}

/* -----------------------------------------------------------------
* Closes SPI interface
*/
void SpiDrv::end(){
	//Setting the Software Reset bit to 1
	SERCOM1->SPI.CTRLA.bit.SWRST = 1;

	//Wait both bits Software Reset from CTRLA and SYNCBUSY are equal to 0
	while(SERCOM1->SPI.CTRLA.bit.SWRST || SERCOM1->SPI.SYNCBUSY.bit.SWRST);
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
* Prepares the _txBuf to be sent as a new command to companion chip
* It requires also the number of attached parameters.
*/
bool SpiDrv::sendCmd(uint8_t cmd, uint8_t numParam){
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
void SpiDrv::sendDataPkt(uint8_t cmd) {
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
* Add parameters to the command buffer for the companion chip
* It requires also the lastParam parameter to understand which is the last one.
*/
bool SpiDrv::sendParam(uint16_t param, uint8_t lastParam){
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
* Add a list of parameters to the command buffer for the companion chip (overload)
* It requires the parameter's list (*param), the length of each parameter (param_len)
* and also the lastParam parameter to understand if this is the last one.
*/
bool SpiDrv::sendParam(uint8_t *param, uint8_t param_len, uint8_t lastParam){
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
* Add byte to the command buffer for the companion chip
* It requires also the lastParam parameter to understand which is the last one.
*/
bool SpiDrv::appendByte(uint8_t data, uint8_t lastParam){
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
bool SpiDrv::appendBuffer(const uint8_t *data, uint32_t len){
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
* Writes to the companion a pre-allocated buffer (mainly used for _txBuf)
*
* params: uint8_t* data:	the buffer pointer
*		  uint16_t len:		number of parameters to read
*/
bool SpiDrv::_writeData(uint8_t *data, uint32_t len){
	uint32_t byteWritten = 0;

	// send all the packets
	while (len > 0) {
		// wait for handshake - when HIGH companion is ready for a new transaction
		while((PORT->Group[PORTA].IN.reg & (1ul << 15)) == 0); // while(digitalRead(SLAVEHANDSHAKE) != HIGH);
		_enableDevice();

		// make sure to send a SPI_MAX_TX_SIZE multiple as to avoid loosing data on the other micro
		if(len > SPI_MAX_TX_SIZE){
			transferComplete = false;
			// write is performed automatically when DMA starts a transaction on channel 0
			_descriptor[0].BTCNT.reg           = SPI_MAX_TX_SIZE; //count;
			_descriptor[0].SRCADDR.reg         = (uint32_t)(data + byteWritten) + SPI_MAX_TX_SIZE;
			_descriptor[0].DSTADDR.reg         = (uint32_t)(&SERCOM1->SPI.DATA.reg);
			_descriptor[0].DESCADDR.reg        = 0;

			// when data are available in the SPI RX register, DMA will start a transaction on channel 1 to save them in readBuf
			_descriptor[1].BTCNT.reg           = SPI_MAX_TX_SIZE; //count;
			_descriptor[1].SRCADDR.reg         = (uint32_t)(&SERCOM1->SPI.DATA.reg);
			_descriptor[1].DSTADDR.reg         = (uint32_t)readBuf + sizeof(readBuf);
			_descriptor[1].DESCADDR.reg        = 0;

			// enable DMA to start SPI communication
			DMAC->CHID.bit.ID    = RX_DMA_CHANNEL;
			DMAC->CHCTRLA.bit.ENABLE = 1; // Enable the transfer channel 1
			DMAC->CHID.bit.ID    = TX_DMA_CHANNEL;
			DMAC->CHCTRLA.bit.ENABLE = 1; // Enable the transfer channel 0
			while(!transferComplete);
			transferComplete = false;

			len -= SPI_MAX_TX_SIZE;
			byteWritten += SPI_MAX_TX_SIZE;
		}
		else{ // write the last chunk
			size_t i = len;
			while((i % 4) != 0) // make sure transmission length is a 4-byte multiple
				i++;
			transferComplete = false;
			// write is performed automatically when DMA starts a transaction on channel 0
			_descriptor[0].BTCNT.reg           = i; //count;
			_descriptor[0].SRCADDR.reg         = (uint32_t)(data + byteWritten) + i;
			_descriptor[0].DSTADDR.reg         = (uint32_t)(&SERCOM1->SPI.DATA.reg);
			_descriptor[0].DESCADDR.reg        = 0;

			// when data are available in the SPI RX register, DMA will start a transaction on channel 1 to save them in readBuf
			_descriptor[1].BTCNT.reg           = i; //count;
			_descriptor[1].SRCADDR.reg         = (uint32_t)(&SERCOM1->SPI.DATA.reg);
			_descriptor[1].DSTADDR.reg         = (uint32_t)readBuf + i;
			_descriptor[1].DESCADDR.reg        = 0;

			// enable DMA to start SPI communication
			DMAC->CHID.bit.ID    = RX_DMA_CHANNEL;
			DMAC->CHCTRLA.bit.ENABLE = 1; // Enable the transfer channel 1
			DMAC->CHID.bit.ID    = TX_DMA_CHANNEL;
			DMAC->CHCTRLA.bit.ENABLE = 1; // Enable the transfer channel 0
			while(!transferComplete);
			transferComplete = false;

			len -= len;
		}

		_disableDevice();
		// check ack
		uint32_t timeout = millis() + 10000;
		// wait for the SR to go HIGH
		while(srLevel != HIGH && timeout > millis());
		if(timeout <= millis()){
			return false;
		}

		if(readBuf[1] != 0){
			if(spiIsr != NULL)
				spiIsr(readBuf);
		}
	}

	return true;
}

/*
*
*/
bool SpiDrv::_writeBuffPkt(uint8_t *data, uint32_t len, uint8_t dataOffset){
	uint16_t j = 0;
	bool firstWrite = true;

	uint32_t addrMem = (uint32_t)(data[dataOffset] + (data[dataOffset + 1] << 8) + (data[dataOffset + 2] << 16) + (data[dataOffset + 3] << 24));
	// write out the data
	while (len > 0){
		// wait for handshake - when HIGH companion is ready for a new transaction
		while((PORT->Group[PORTA].IN.reg & (1ul << 15)) == 0); // while(digitalRead(SLAVEHANDSHAKE) != HIGH);
		_enableDevice();
		
		if(firstWrite){
			// send header
			transferComplete = false;
			// write is performed automatically when DMA starts a transaction on channel 0
			_descriptor[0].BTCNT.reg           = dataOffset; //count;
			_descriptor[0].SRCADDR.reg         = (uint32_t)data + dataOffset;
			_descriptor[0].DSTADDR.reg         = (uint32_t)(&SERCOM1->SPI.DATA.reg);
			_descriptor[0].DESCADDR.reg        = 0;

			// when data are available in the SPI RX register, DMA will start a transaction on channel 1 to save them in readBuf
			_descriptor[1].BTCNT.reg           = dataOffset; //count;
			_descriptor[1].SRCADDR.reg         = (uint32_t)(&SERCOM1->SPI.DATA.reg);
			_descriptor[1].DSTADDR.reg         = (uint32_t)readBuf + dataOffset;
			_descriptor[1].DESCADDR.reg        = 0;

			// enable DMA to start SPI communication
			DMAC->CHID.bit.ID    = RX_DMA_CHANNEL;
			DMAC->CHCTRLA.bit.ENABLE = 1; // Enable the transfer channel 1
			DMAC->CHID.bit.ID    = TX_DMA_CHANNEL;
			DMAC->CHCTRLA.bit.ENABLE = 1; // Enable the transfer channel 0
			while(!transferComplete);
			transferComplete = false;
		}
		if(len > SPI_MAX_TX_SIZE){
			// data are greater than SPI_MAX_TX_SIZE. Multiple writes are requested
			size_t i = SPI_MAX_TX_SIZE - dataOffset;

			transferComplete = false;
			// write is performed automatically when DMA starts a transaction on channel 0
			_descriptor[0].BTCNT.reg           = i; //count;
			_descriptor[0].SRCADDR.reg         = (uint32_t)(addrMem + j) + i;
			_descriptor[0].DSTADDR.reg         = (uint32_t)(&SERCOM1->SPI.DATA.reg);
			_descriptor[0].DESCADDR.reg        = 0;

			// when data are available in the SPI RX register, DMA will start a transaction on channel 1 to save them in readBuf
			_descriptor[1].BTCNT.reg           = i; //count;
			_descriptor[1].SRCADDR.reg         = (uint32_t)(&SERCOM1->SPI.DATA.reg);
			_descriptor[1].DSTADDR.reg         = (uint32_t)readBuf + dataOffset + i;
			_descriptor[1].DESCADDR.reg        = 0;

			// enable DMA to start SPI communication
			DMAC->CHID.bit.ID    = RX_DMA_CHANNEL;
			DMAC->CHCTRLA.bit.ENABLE = 1; // Enable the transfer channel 1
			DMAC->CHID.bit.ID    = TX_DMA_CHANNEL;
			DMAC->CHCTRLA.bit.ENABLE = 1; // Enable the transfer channel 0
			while(!transferComplete);
			transferComplete = false;

			j += SPI_MAX_TX_SIZE - dataOffset;
			len -= SPI_MAX_TX_SIZE; // consider also dataOffset that we previously transferred

			if(firstWrite){
				firstWrite = false;
			}
		}
		else{
			// data fit SPI_MAX_TX_SIZE
			size_t i = len - dataOffset - 1;
			transferComplete = false;
			// write is performed automatically when DMA starts a transaction on channel 0
			_descriptor[0].BTCNT.reg           = i; //count;
			_descriptor[0].SRCADDR.reg         = (uint32_t)(addrMem + j) + i;
			_descriptor[0].DSTADDR.reg         = (uint32_t)(&SERCOM1->SPI.DATA.reg);
			_descriptor[0].DESCADDR.reg        = 0;

			// when data are available in the SPI RX register, DMA will start a transaction on channel 1 to save them in readBuf
			_descriptor[1].BTCNT.reg           = i; //count;
			_descriptor[1].SRCADDR.reg         = (uint32_t)(&SERCOM1->SPI.DATA.reg);
			_descriptor[1].DSTADDR.reg         = (uint32_t)readBuf + dataOffset + i;
			_descriptor[1].DESCADDR.reg        = 0;

			// enable DMA to start SPI communication
			DMAC->CHID.bit.ID    = RX_DMA_CHANNEL;
			DMAC->CHCTRLA.bit.ENABLE = 1; // Enable the transfer channel 1
			DMAC->CHID.bit.ID    = TX_DMA_CHANNEL;
			DMAC->CHCTRLA.bit.ENABLE = 1; // Enable the transfer channel 0
			while(!transferComplete);
			transferComplete = false;

			// send END_CMD and 0 padding to a 4-byte multiple
			uint8_t remaining = ((len % 4) != 0) ? (4 - (len % 4)) : 0;
			static uint8_t endPacket[] = {END_CMD, 0, 0, 0};
			remaining++; // END_CMD

			// write is performed automatically when DMA starts a transaction on channel 0
			_descriptor[0].BTCNT.reg           = remaining; //count;
			_descriptor[0].SRCADDR.reg         = (uint32_t)endPacket + remaining;
			_descriptor[0].DSTADDR.reg         = (uint32_t)(&SERCOM1->SPI.DATA.reg);
			_descriptor[0].DESCADDR.reg        = 0;

			// when data are available in the SPI RX register, DMA will start a transaction on channel 1 to save them in readBuf
			_descriptor[1].BTCNT.reg           = remaining; //count;
			_descriptor[1].SRCADDR.reg         = (uint32_t)(&SERCOM1->SPI.DATA.reg);
			_descriptor[1].DSTADDR.reg         = (uint32_t)readBuf + dataOffset + remaining;
			_descriptor[1].DESCADDR.reg        = 0;

			// enable DMA to start SPI communication
			DMAC->CHID.bit.ID    = RX_DMA_CHANNEL;
			DMAC->CHCTRLA.bit.ENABLE = 1; // Enable the transfer channel 1
			DMAC->CHID.bit.ID    = TX_DMA_CHANNEL;
			DMAC->CHCTRLA.bit.ENABLE = 1; // Enable the transfer channel 0
			while(!transferComplete);
			transferComplete = false;

			if(firstWrite){
				firstWrite = false;
			}
			len -= len;
		}
		dataOffset = 0;

		_disableDevice();

		// check ack
		uint32_t timeout = millis() + 2000;
		// wait for the SR to go HIGH
		while(srLevel != HIGH && timeout > millis());

		if(timeout <= millis()){
			return false;
		}

		if(readBuf[1] != 0){
			if(spiIsr != NULL)
				spiIsr(readBuf);
		}
	}
	return true;
}

/* -----------------------------------------------------------------
* Read data from the companion chip after an ISR event (32 bytes per time)
* params: uint8_t* buffer:	data buffer to store the received data
* 
* return: (uint16_t)
*		   byte read.
*/
uint16_t SpiDrv::readDataISR(uint8_t *buffer){
	delayMicroseconds(12); // useful to make sure this is not an ack
	// wait for the SRsr signal HIGH - companion is ready
	if((PORT->Group[PORTA].IN.reg & (1ul << 21)) != 0){ // if (digitalRead(SLAVEREADY) == HIGH) {
		// wait for handshake - when HIGH companion is ready for a new transaction
		while((PORT->Group[PORTA].IN.reg & (1ul << 15)) == 0); // while(digitalRead(SLAVEHANDSHAKE) != HIGH);
		_enableDevice();

		// read all the 32 bytes available in the companion data out buffer
		transferComplete = false;
		// write is performed automatically when DMA starts a transaction on channel 0
		_descriptor[0].BTCNT.reg           = BUF_SIZE; //count;
		_descriptor[0].SRCADDR.reg         = (uint32_t)_emptyBuf + sizeof(_emptyBuf);
		_descriptor[0].DSTADDR.reg         = (uint32_t)(&SERCOM1->SPI.DATA.reg);
		_descriptor[0].DESCADDR.reg        = 0;

		// when data are available in the SPI RX register, DMA will start a transaction on channel 1 to save them in buffer
		_descriptor[1].BTCNT.reg           = BUF_SIZE; //count;
		_descriptor[1].SRCADDR.reg         = (uint32_t)(&SERCOM1->SPI.DATA.reg);
		_descriptor[1].DSTADDR.reg         = (uint32_t)buffer + BUF_SIZE;
		_descriptor[1].DESCADDR.reg        = 0;

		// enable DMA to start SPI communication
		DMAC->CHID.bit.ID    = RX_DMA_CHANNEL;
		DMAC->CHCTRLA.bit.ENABLE = 1; // Enable the transfer channel 1
		DMAC->CHID.bit.ID    = TX_DMA_CHANNEL;
		DMAC->CHCTRLA.bit.ENABLE = 1; // Enable the transfer channel 0
		while(!transferComplete);
		transferComplete = false;

		_disableDevice();
	} 
	else {
		// if the SR is still low, return with an error	
		return 0;
	}
			
	// after reading all the 32 byte long message we need to ensure that the SR goes low again
	uint32_t t_hold = millis() + 10;
	while(t_hold > millis()){

		// if we are in a multipacket case...
		if(srLevel == LOW){
			return BUF_SIZE;
		}
	}
		
	return 0;
}

#endif
#endif //COMMUNICATION_LIB_ENABLED