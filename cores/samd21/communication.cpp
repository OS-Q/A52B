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

#include "communication.h"
#include "Arduino.h"

#ifdef COMMUNICATION_LIB_ENABLED

#define INT_BUFF_SIZE 40

uint8_t intPin[13] = {11, 12, 13, 14, 16, 17, 18, 19, 20, 21, 22, 23, 24};
volatile uint8_t intBuffer[INT_BUFF_SIZE];
volatile uint8_t buffHead = 0;
volatile uint8_t buffTail = 0;

void virtualInt(void* arg){
	// store interrupt in HEAD
	intBuffer[buffHead] = *(uint8_t*)arg;
	// move HEAD ahead
	buffHead = ++buffHead % INT_BUFF_SIZE;
	// if after insertion HEAD and TAIL are the same, move TAIL ahead
	if(buffTail == buffHead)
		buffTail = ++buffTail % INT_BUFF_SIZE;
}

void communicationEvtCb(uint8_t* pkt){
    if (pkt != NULL) { // data was received during a write. copy data into commonDrv buffer before to decode them
        memcpy(communication._rxBuf, pkt, BUF_SIZE);
    } else {
        // read the data out from the communication interface and put them into a buffer
		uint8_t ret = communication._interface.readDataISR(communication._rxBuf);
		
        // decode the BUF_SIZE bytes long packet
        if (ret == 0){
            return;
		}
    }
	uint8_t cmd = communication._rxBuf[1] - 0x80;

	if(communication._endReceived == false)
		cmd = communication._currentCmd; // retrieve the command we are processing
	
	// check if command belongs to the user-specified ones
	if(cmd == CUSTOM_CMD_CODE){
		if(communication._cmdIndex){
			uint8_t customCmd;
			if(communication._endReceived == false)
				customCmd = communication._customCurrentCmd;
			else
				customCmd = communication._rxBuf[6];
			for(uint8_t i = 0; i < communication._cmdIndex; i++){
				if(customCmd == communication._customCmdCode[i]){
					if(communication._endReceived){ // new command
						communication._totPktLen = (uint32_t)(communication._rxBuf[2] + (communication._rxBuf[3] << 8) + (communication._rxBuf[4] << 16) + (communication._rxBuf[5] << 24));
						uint8_t copyLen;
						if(communication._totPktLen <= BUF_SIZE){ // data are contained in a single packet
							copyLen = communication._totPktLen - 8;
							communication._endReceived = true;
							// control the packet footer.
							if(communication._rxBuf[communication._totPktLen - 1] != END_CMD){
								return; // error
							}
							
							// copy data
							if(communication._custom_ext_buff[i] != NULL)
								memcpy(&communication._custom_ext_buff[i][0], &communication._rxBuf[7], copyLen);
							communication._custom_buff_index = copyLen;
							
							// call user defined callback
							if(communication._userCallbacks[i] != NULL)
								communication._userCallbacks[i](communication._custom_buff_index);
							
							communication._custom_buff_index = 0;
							return;
							
						}
						else{
							copyLen = BUF_SIZE - 7; // buffer size - header
							communication._endReceived = false;
							communication._currentCmd = cmd;
							communication._customCurrentCmd = customCmd;
	
							// copy the first chunk
							if(communication._custom_ext_buff[i] != NULL)
								memcpy(&communication._custom_ext_buff[i][0], &communication._rxBuf[7], copyLen);
							communication._custom_buff_index = copyLen;			
							return;
						}
					}
					else{ // command continuation
						if((communication._custom_buff_index + BUF_SIZE) >= (communication._totPktLen - 8)){ // last chunk
							uint8_t copyLen = BUF_SIZE - ((communication._custom_buff_index + BUF_SIZE) - (communication._totPktLen - 8));
							if(communication._custom_ext_buff[i] != NULL)
								memcpy(&communication._custom_ext_buff[i][communication._custom_buff_index], communication._rxBuf, copyLen);
							communication._endReceived = true;
							communication._custom_buff_index += copyLen;
							
							communication._totPktLen = 0;
							// call user defined callback
							if(communication._userCallbacks[i] != NULL)
								communication._userCallbacks[i](communication._custom_buff_index);
							communication._custom_buff_index = 0;
							communication._currentCmd = 0;
							return;
						}
						else{ // intermediate chunk
							if(communication._custom_ext_buff[i] != NULL)
								memcpy(&communication._custom_ext_buff[i][communication._custom_buff_index], communication._rxBuf, BUF_SIZE);
							communication._custom_buff_index += BUF_SIZE;
							return;
						}
					}
					break;
				}
			}
		}
		return;
	}

	// check all other commands
	switch(cmd){
		case SYSTEMCMD_RESPONSE:
			communication._responseType = SYSTEMCMD_RESPONSE;
		break;

		case SYSTEMCMD_REQUEST:
			communication._systemCommand();
		break;

#if not defined(COMM_CH_UART)
		case AUTOBAUD_UPD:{
			// get baudrate value
			uint8_t *dataBuf = communication.getRxBuffer();
			dataBuf += 5; // skip packet header
			uint32_t baud = dataBuf[0] + (dataBuf[1] << 8) + (dataBuf[2] << 16) +
							(dataBuf[3] << 24);

			#ifdef SERIAL_COMPANION_ENABLED
			SerialCompanion.begin(baud);
			#endif
		}
		break;
#endif

		case GPIO_RESPONSE:
			communication._responseType = GPIO_RESPONSE;
		break;
		
		case GPIO_REQUEST:
			communication._moveGpio();
		break;

		case GPIO_INTERRUPT:
			for(uint8_t i = 0; i < communication._extIntNumber; i++){
				if(communication._extInt[i].pin == communication._rxBuf[5]){
					if(communication._extInt[i].args != NULL)
						communication._extInt[i].func(communication._extInt[i].args);
					else
						((voidFuncPtr)communication._extInt[i].func)();
					break;
				}
			}
		break;

		case SERIAL_STATUS:{
			#ifdef USB_ENABLED
				#if not defined(COMM_CH_UART) && not defined(SINGLE_CDC) && defined(SERIAL_COMPANION_ENABLED)
					bool isSerial = (USBCompanion == true);
				#else
					bool isSerial = (Serial == true);
				#endif
			#else
				bool isSerial = false;
			#endif
			communication.sendCmdPkt(SERIAL_STATUS, PARAM_NUMS_1);
			communication.sendParam((uint8_t*)&isSerial, 1, LAST_PARAM);
			}
		break;

		// process PUT_CUSTOM_DATA command sent from the other micro
		case PUT_CUSTOM_DATA:
			if(communication._endReceived){ // new command
				communication._totPktLen = (uint32_t)(communication._rxBuf[2] + (communication._rxBuf[3] << 8) + (communication._rxBuf[4] << 16) + (communication._rxBuf[5] << 24));
				if(communication._user_ext_buff == NULL){ // create a safe place where storing data
					// note: what follows is not a safe instruction for a microcontroller and should be avoided by using initExternalBuffer function.
					communication._user_ext_buff = (uint8_t*)malloc(communication._totPktLen);
				}
				uint8_t copyLen;
				if(communication._totPktLen <= 32){ // data are contained in a single packet
					copyLen = communication._totPktLen - 7;
					communication._endReceived = true;
					// control the packet footer.
					if(communication._rxBuf[communication._totPktLen - 1] != END_CMD){
						return; // error
					}
					
					// copy data
					if(communication._user_ext_buff != NULL)
						memcpy(communication._user_ext_buff, &communication._rxBuf[6], copyLen);
					communication._user_ext_buff_sz = copyLen;
					
					return;
					
				}
				else{
					copyLen = BUF_SIZE - 6; // buffer size - header
					communication._endReceived = false;
					communication._currentCmd = cmd;

					// copy the first chunk
					if(communication._user_ext_buff != NULL)
						memcpy(communication._user_ext_buff, &communication._rxBuf[6], copyLen);
					communication._user_ext_buff_sz = copyLen;			
					return;
				}
			}
			else{ // command continuation
				if((communication._user_ext_buff_sz + BUF_SIZE) >= (communication._totPktLen - 7)){ // last chunk
					uint8_t copyLen = BUF_SIZE - ((communication._user_ext_buff_sz + BUF_SIZE) - (communication._totPktLen - 7));
					if(communication._user_ext_buff != NULL)
						memcpy(&communication._user_ext_buff[communication._user_ext_buff_sz], communication._rxBuf, copyLen);
					communication._endReceived = true;
					communication._user_ext_buff_sz += copyLen;
					
					communication._totPktLen = 0;
					
					communication._currentCmd = 0;
					return;
				}
				else{ // intermediate chunk
					if(communication._user_ext_buff != NULL)
						memcpy(&communication._user_ext_buff[communication._user_ext_buff_sz], communication._rxBuf, BUF_SIZE);
					communication._user_ext_buff_sz += BUF_SIZE;
					return;
				}
			}
		break;

		default:
			// call user defined handlers
			communication._responseType = NONE;
			for(uint8_t i = 0; i < EXT_HANDLES; i++)
				if(communication._handleExt[i] != NULL)
					communication._handleExt[i](communication._rxBuf);
		break;
	}
}

void registerInterrupt(uint8_t pin, voidFuncPtrArg userFunc, void * arg){
	// check if a callback has already been registered for this pin
	for(uint8_t i = 0; i < communication._extIntNumber; i++){
		if(communication._extInt[i].pin == pin){
			// just replace callback and args in this case
			communication._extInt[i].func = userFunc;
			communication._extInt[i].args = arg;
			return;
		}
	}
	if(communication._extIntNumber == COMPANION_INTERRUPTS)
		return;

	communication._extInt[communication._extIntNumber].pin = pin;
	communication._extInt[communication._extIntNumber].func = userFunc;
	communication._extInt[communication._extIntNumber].args = arg;
	communication._extIntNumber++;
}

void deleteInterrupt(uint8_t pin){
	// check if an interrupt has been registered for this pin
	uint8_t i;
	for(i = 0; i < communication._extIntNumber; i++){
		if(communication._extInt[i].pin == pin){ // pin found
			break;
		}
	}

	if(i == communication._extIntNumber) // pin not found
		return;

	// overwrite pin to be deleted by moving the remaining ones one position back
	for(; i < communication._extIntNumber - 1; i++){
		communication._extInt[i].pin = communication._extInt[i + 1].pin;
		communication._extInt[i].func = communication._extInt[i + 1].func;
		communication._extInt[i].args = communication._extInt[i + 1].args;
	}
	communication._extIntNumber--;
}

void Communication::communicationBegin(){
	_interface.begin();
	_interface.registerCb(communicationEvtCb);
#if not defined(COMM_CH_UART) && not defined(SINGLE_CDC) && defined(SERIAL_COMPANION_ENABLED)
	SerialCompanion.begin(AUTOBAUD);
#endif
}


bool Communication::registerCommand(uint8_t cmdCode, ext_callback function, void* buffer){
	handleCommEvents();
	
	if(_cmdIndex == CUSTOM_CMD_NR)
		return false; // do not accept more than CUSTOM_CMD_NR commands

	for(uint8_t i = 0; i < _cmdIndex; i++){
		if(_customCmdCode[i] == cmdCode)
			return false; // do not accept an already assigned command code
	}
		
	if(buffer != NULL)
		_custom_ext_buff[_cmdIndex] = (uint8_t*)buffer;
	
	_customCmdCode[_cmdIndex] = cmdCode;
	_userCallbacks[_cmdIndex] = function;
	_cmdIndex++;
	return true;
}

void Communication::sendCommand(uint8_t cmdCode, void* data, uint32_t len){
	handleCommEvents();
	bool existing = false;
	
	// check if cmdCode is an already registered one
	for(uint8_t i = 0; i < _cmdIndex; i++){
		if(_customCmdCode[i] == cmdCode){
			existing = true;
			break;
		}
	}
	if(!existing) // do not send a command that has not been previously registered
		return;

	communication.sendDataPkt(CUSTOM_CMD_CODE);
	communication.appendByte(cmdCode);
	communication.appendBuffer((uint8_t*)data, len);
}

void Communication::initExternalBuffer(void* extBuf){
    _user_ext_buff = (uint8_t*)extBuf;
}

int32_t Communication::availGenericData(void){
    handleCommEvents();

    // check if there are new data for our socket
    if (_endReceived && _user_ext_buff_sz > 0) {
        return _user_ext_buff_sz;
    }
    return 0;
}

uint32_t Communication::getGenericData(void *data, uint32_t len){
	uint32_t size = _user_ext_buff_sz;
	if(data != NULL){
		// get minimum between len and _user_ext_buff_sz
		if(len > 0)
			size = (len > _user_ext_buff_sz) ? _user_ext_buff_sz : len;
		memcpy(data, _user_ext_buff, size);
	}
	_user_ext_buff_sz = 0;
	return size;
}

void Communication::sendGenericData(void* data, uint32_t len){
    handleCommEvents();
    
	communication.sendDataPkt(PUT_CUSTOM_DATA);
	communication.appendBuffer((uint8_t*)data, len);
}


bool Communication::registerHandle(void(*func)(uint8_t*)){
	bool registered = false;
	for(uint8_t i = 0; i < EXT_HANDLES; i++){
		// look for the first available space for the handle
		if(_handleExt[i] == NULL){
			_handleExt[i] = func;
			registered = true;
			break;
		}
	}
	return registered;
}

void Communication::handleCommEvents(void){
	while(buffTail != buffHead){ // empty the interrupt buffer
		_interface.sendCmd(GPIO_INTERRUPT, PARAM_NUMS_1);
		_interface.sendParam((uint8_t*)&intBuffer[buffTail], 1, LAST_PARAM);
		buffTail = ++buffTail % INT_BUFF_SIZE;
	}

#ifndef COMM_CH_UART
	// check for baudate changes if companion required autobaud
	if(autobaud){
		if(currentBaud != getBaudRateReg()){
			currentBaud = getBaudRateReg();
			sendCmdPkt(AUTOBAUD_UPD, PARAM_NUMS_1);
			sendParam((uint8_t*)&currentBaud, 4, LAST_PARAM);
		}
	}
#endif

	_interface.handleEvents();

#ifdef USB_ENABLED
	#if not defined(COMM_CH_UART) && not defined(SINGLE_CDC) && defined(SERIAL_COMPANION_ENABLED)
		// handle events on usb interface
		uint32_t timeout = millis() + 50;
		// read data while available for a maximum of 50 ms to avoid being stuck here
		while(USBCompanion.available() && (timeout > millis())){
			SerialCompanion.write(USBCompanion.read());
		}
		timeout = millis() + 50;
		// read data while available for a maximum of 50 ms to avoid being stuck here
		while(SerialCompanion.available() && (timeout > millis())){
			USBCompanion.write(SerialCompanion.read());
		}
	#endif
#endif
}

/*
* Process SYSTEMCMD_REQUEST command
*/
void Communication::_systemCommand(){
	bool     rep     = false;
	uint8_t *dataBuf = &_rxBuf[5]; // skip packet header
	uint8_t  op      = dataBuf[0];
	uint32_t val     = 0;
	uint32_t result  = 0;

	switch (op){
		case SYSTEMCMD_GET_BAUD:
			// get baud rate of our companion-connected serial port
			result = getBaudRateReg();
			currentBaud = result;
			autobaud = true;
			rep = true;
		break;

		default:
			rep = false;
		break;
	}

	if (rep == true){
		// send reply
		_interface.sendCmd(SYSTEMCMD_RESPONSE, PARAM_NUMS_1);
		_interface.sendParam((uint8_t *)&result, 4, LAST_PARAM);
	}
	return;
}

/*
* Process GPIO_REQUEST command by actuating the selected operation on the specified pin
*/
void Communication::_moveGpio(){
	bool rep = false; // no reply needed
	uint8_t* dataBuf = &_rxBuf[5]; // skip packet header
	uint8_t op = dataBuf[0];
	uint8_t pin = dataBuf[2];
	uint8_t mode;
	uint32_t val = 0;
	uint32_t freq = 0;
	uint32_t result = 0;
		
	switch(op){
		case PIN_MODE: // pinMode command
			mode = dataBuf[4]; 
			pinMode(pin, mode);
		break;
		case WRITE_DIGITAL: // digitalWrite command
			val = dataBuf[4] + (dataBuf[5] << 8) + (dataBuf[6] << 16) + (dataBuf[7] << 24);
			digitalWrite(pin, val);
		break;
		case WRITE_TOGGLE: // digitalToggle command
			digitalToggle(pin);
		break;
		case WRITE_ANALOG: // analogWrite command
			val = dataBuf[4] + (dataBuf[5] << 8) + (dataBuf[6] << 16) + (dataBuf[7] << 24);
			analogWrite(pin, val);
		break;
		case WRITE_ANALOG_F: // analogWriteFreq command
			val = dataBuf[4] + (dataBuf[5] << 8) + (dataBuf[6] << 16) + (dataBuf[7] << 24);
			freq = dataBuf[9] + (dataBuf[10] << 8) + (dataBuf[11] << 16) + (dataBuf[12] << 24);
			analogWriteFreq(pin, val, freq);
		break;
		case READ_DIGITAL: // digitalRead command	
			result = digitalRead(pin);
			rep = true;
		break;
		case READ_ANALOG: // analogRead command
			result = analogRead(pin);
			rep = true;
		break;
		case SET_INTERRUPT:{ // attachInterrupt command
			mode = dataBuf[4];
			if(pin < 11 || pin > 24 || pin == 15)
				return;
			uint8_t index = (pin < 15) ? (pin - 11) : (pin - 12);
			pinMode(pin, INPUT);
			attachInterruptArg(pin, virtualInt, &intPin[index], mode);
		}
		break;
		case UNSET_INTERRUPT: // detachInterrupt command
			detachInterrupt(pin);
		break;

		default:
		break;
	}
	
	if(rep == true){
		//send reply
		_interface.sendCmd(GPIO_RESPONSE, PARAM_NUMS_1);
		_interface.sendParam((uint8_t*)&result, 4, LAST_PARAM);
	}
	return;
}

void Communication::clearResponseType(){
	_responseType = NONE;
}

uint8_t Communication::getResponseType(){
	return _responseType;
}


bool Communication::sendCmdPkt(uint8_t cmd, uint8_t numParam){
	return _interface.sendCmd(cmd, numParam);
}

void Communication::sendDataPkt(uint8_t cmd){
	_interface.sendDataPkt(cmd);
}

bool Communication::sendParam(uint8_t *param, uint8_t param_len, uint8_t lastParam){
	return _interface.sendParam(param, param_len, lastParam);
}

bool Communication::sendParam(uint16_t param, uint8_t lastParam){
	return _interface.sendParam(param, lastParam);
}

bool Communication::appendByte(uint8_t data, uint8_t lastParam){
	return _interface.appendByte(data, lastParam);
}

bool Communication::appendBuffer(const uint8_t *data, uint32_t len){
	return _interface.appendBuffer(data, len);
}

uint8_t* Communication::getRxBuffer(){
	return _rxBuf;
}

#endif // COMMUNICATION_LIB_ENABLED
// switchCompanion function is always defined

void Communication::switchCompanion(uint8_t status){
	PORT->Group[PORTA].DIRSET.reg = PORT_PA20;
	PORT->Group[PORTA].DIRSET.reg = PORT_PA28;
	if(status == LOW){
		PORT->Group[PORTA].OUTCLR.reg = PORT_PA20;  // RST
		PORT->Group[PORTA].OUTSET.reg = PORT_PA28;  // PWR
	}
	else{
		PORT->Group[PORTA].OUTCLR.reg = PORT_PA28;  // PwR
		delay(10);
		PORT->Group[PORTA].OUTSET.reg = PORT_PA20;  // RST
	}
}

Communication communication;

#ifdef COMMUNICATION_LIB_ENABLED
/*
* Activate pinMode function on one of the pin connected to the companion chip
*
* param op: a code representing the operation to be performed
* param pin: pin on which operation has to take effect
* param mod: mode applied to pin (e.g. OUTPUT, INPUT and so on)
*/
extern "C" void sendGPIOMode(uint8_t op, uint8_t pin, uint8_t mod){
	communication.handleCommEvents();
	
	communication.sendCmdPkt(GPIO_REQUEST, PARAM_NUMS_3);
	communication.sendParam(&op, 1, NO_LAST_PARAM);
	communication.sendParam(&pin, 1, NO_LAST_PARAM);
	communication.sendParam(&mod, 1, LAST_PARAM);
}

/*
* Activate digitalWrite or analogWrite function on one of the pin connected to the companion chip
*
* param op: a code representing the operation to be performed (WRITE_DIGITAL or WRITE_ANALOG)
* param pin: pin on which operation has to take effect
* param mod: value to be applied to pin
*/
extern "C" void sendGPIOWrite(uint8_t op, uint8_t pin, uint32_t val = 0, uint32_t freq = 0){
	communication.handleCommEvents();

	if(op == WRITE_TOGGLE){
		communication.sendCmdPkt(GPIO_REQUEST, PARAM_NUMS_2);
		communication.sendParam(&op, 1, NO_LAST_PARAM);
		communication.sendParam(&pin, 1, LAST_PARAM);
		return;
	}

	if(op == WRITE_ANALOG_F)
		communication.sendCmdPkt(GPIO_REQUEST, PARAM_NUMS_4);
	else
		communication.sendCmdPkt(GPIO_REQUEST, PARAM_NUMS_3);
	communication.sendParam(&op, 1, NO_LAST_PARAM);
	communication.sendParam(&pin, 1, NO_LAST_PARAM);
	if(op == WRITE_ANALOG_F){
		communication.sendParam((uint8_t*)&val, 4, NO_LAST_PARAM);
		communication.sendParam((uint8_t*)&freq, 4, LAST_PARAM);
	}
	else
		communication.sendParam((uint8_t*)&val, 4, LAST_PARAM);
}

/*
* Activate digitalRead or analogRead function on one of the pin connected to the companion chip
*
* param op: a code representing the operation to be performed (READ_DIGITAL or READ_ANALOG)
* param pin: pin on which operation has to take effect
*
* return: the read value
*/
extern "C" int32_t sendGPIORead(uint8_t op, uint8_t pin){
	communication.handleCommEvents();
	
	communication.clearResponseType();
	communication.sendCmdPkt(GPIO_REQUEST, PARAM_NUMS_2);
	communication.sendParam(&op, 1, NO_LAST_PARAM);
	communication.sendParam(&pin, 1, LAST_PARAM);

    // Poll response for the next 2 seconds
    uint32_t start = millis();
    while (((millis() - start) < 2000)) {
        communication.handleCommEvents();
        if (communication.getResponseType() == GPIO_RESPONSE) {
			uint8_t* dataBuf = communication.getRxBuffer();
			dataBuf += 5; // skip packet header
            uint32_t result = dataBuf[0] + (dataBuf[1] << 8) + (dataBuf[2] << 16) + (dataBuf[3] << 24);
			return result;
        }
    }
    return -1;
}

/*
* Activate attachInterrupt function on one of the pin connected to the companion chip
*
* param op: a code representing the operation to be performed
* param pin: pin on which operation has to take effect
* param userFunc: callback called when interrupt will be triggered from companion chip
* param arg: optional argument passed to callback function
* param mode: mode applied to interrupt (e.g. RISING, FALLING and so on)
*/
extern "C" void sendAttachInt(uint8_t op, uint8_t pin, voidFuncPtrArg userFunc, void * arg, uint8_t mode){
	communication.handleCommEvents();

	// change mode following companion mode specification
	uint8_t intr_type;
	switch(mode){
		case RISING:
			intr_type = 1;
		break;
		case FALLING:
			intr_type = 2;
		break;
		case CHANGE:
			intr_type = 3;
		break;
		case LOW:
			intr_type = 4;
		break;
		case HIGH:
		default:
			intr_type = 5;
		break;
	}
	communication.sendCmdPkt(GPIO_REQUEST, PARAM_NUMS_3);
	communication.sendParam(&op, 1, NO_LAST_PARAM);
	communication.sendParam(&pin, 1, NO_LAST_PARAM);
	communication.sendParam(&intr_type, 1, LAST_PARAM);

	registerInterrupt(pin, userFunc, arg);
}

/*
* Activate detachInterrupt function on one of the pin connected to the companion chip
*
* param op: a code representing the operation to be performed
* param pin: pin on which operation has to take effect
*/
extern "C" void sendDetachInt(uint8_t op, uint8_t pin){
	communication.handleCommEvents();

	communication.sendCmdPkt(GPIO_REQUEST, PARAM_NUMS_2);
	communication.sendParam(&op, 1, NO_LAST_PARAM);
	communication.sendParam(&pin, 1, LAST_PARAM);

	deleteInterrupt(pin);
}

/*
* Perform System level command to retrieve a value from the companion chip
*
* param op: a code representing the operation to be performed (SYSTEMCMD_GET_BAUD, etc)
*
* return: the read value
*/
int32_t sendSystemGet(uint8_t op){
	communication.handleCommEvents();

	communication.clearResponseType();
	communication.sendCmdPkt(SYSTEMCMD_REQUEST, PARAM_NUMS_1);
	communication.sendParam(&op, 1, LAST_PARAM);
	// Poll response for the next 2 seconds
	uint32_t start = millis();
	while (((millis() - start) < 2000)){
		communication.handleCommEvents();
		if (communication.getResponseType() == SYSTEMCMD_RESPONSE){
		  uint8_t *dataBuf = communication.getRxBuffer();
		  dataBuf += 5; // skip packet header
		  uint32_t result = dataBuf[0] + (dataBuf[1] << 8) + (dataBuf[2] << 16) +
							(dataBuf[3] << 24);
		  return result;
		}
	}
	return -1;
}

/*
* Get the baud rate value uart (sercom5) from registers 
*	 
*    
*  @return baud rate 
*/
uint32_t getBaudRateReg(void)
{
	uint16_t sampleRateValue;
	
	//   uint32_t baudTimes8 = (SystemCoreClock * 8) / (16 * baudrate);
    //   sercom->USART.BAUD.FRAC.FP   = (baudTimes8 % 8);           sercom->USART.BAUD.FRAC.BAUD = (baudTimes8 / 8);
    //   uint32_t baudTimes8 = (SystemCoreClock * 8) / (sampleRateValue * baudrate)
	//   BAUD = ( SystemCoreClock * 8 ) / sampleRateValue * baudTimes8
	uint32_t baudRateReg =( SystemCoreClock  * 8 ) / (16 * ((SERCOM5->USART.BAUD.FRAC.BAUD * 8) + SERCOM5->USART.BAUD.FRAC.FP));
    return 	baudRateReg;
	
}

#endif // COMMUNICATION_LIB_ENABLED