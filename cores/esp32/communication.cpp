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
#include "driver/uart.h"

#ifdef COMMUNICATION_LIB_ENABLED

#define INT_BUFF_SIZE 40

uint8_t gpioData[4];
bool serialStatus = false;
uint8_t intPin[40] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, \
                      21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39 };
volatile uint8_t intBuffer[INT_BUFF_SIZE];
volatile uint8_t buffHead = 0;
volatile uint8_t buffTail = 0;

void IRAM_ATTR virtualInt(void* arg){
	// store interrupt in HEAD
	intBuffer[buffHead] = *(uint8_t*)arg;
	// move HEAD ahead
	buffHead = ++buffHead % INT_BUFF_SIZE;
	// if after insertion HEAD and TAIL are the same, move TAIL ahead
	if(buffTail == buffHead)
		buffTail = ++buffTail % INT_BUFF_SIZE;
}

void handle(){
  // check if command belongs to the user-specified ones
  if(communication._tempPkt.cmd == CUSTOM_CMD_CODE){
	if (communication._tempPkt.cmdType == DATA_PKT && communication._interface.recPacket.endReceived == false) {
		if (communication._interface.socketPktPtr == 0) { // first packet. received size is 0
			communication._customCurrentCmd = communication._tempPkt.dataPtr[0];
			for(uint8_t i = 0; i < communication._cmdIndex; i++){
				if(communication._customCurrentCmd == communication._customCmdCode[i]){
					if(communication._custom_ext_buff[i] != NULL)
						memcpy(communication._custom_ext_buff[i], communication._tempPkt.dataPtr + 1, communication._tempPkt.nParam - 1);
					communication._interface.socketPktPtr = communication._tempPkt.nParam;
					break;
				}
			}
		} else {
			for(uint8_t i = 0; i < communication._cmdIndex; i++){ // intermediate chunk
				if(communication._customCurrentCmd == communication._customCmdCode[i]){
					if(communication._custom_ext_buff[i] != NULL)
						memcpy(communication._custom_ext_buff[i] + communication._interface.socketPktPtr - 1, communication._tempPkt.dataPtr, communication._tempPkt.nParam);
					communication._interface.socketPktPtr += communication._tempPkt.nParam;
					break;
				}		
			}
		}
	}
	else {
		if (communication._tempPkt.nParam == 255) { // 255 indicates a special case where data packet payload fits 1 packet
			uint8_t customCmd = communication._tempPkt.dataPtr[0];
			for(uint8_t i = 0; i < communication._cmdIndex; i++){
				if(customCmd == communication._customCmdCode[i]){
					if(communication._custom_ext_buff[i] != NULL)
						memcpy(communication._custom_ext_buff[i], communication._tempPkt.dataPtr + 1, communication._tempPkt.totalLen);
					communication._interface.socketPktPtr = communication._tempPkt.totalLen;
					// call user-defined function
					if(communication._userCallbacks[i] != NULL)
						communication._userCallbacks[i](communication._interface.socketPktPtr);
					break;
				}
			}
			communication._interface.socketPktPtr = 0;
		} else { // last packet
			for(uint8_t i = 0; i < communication._cmdIndex; i++){
				if(communication._customCurrentCmd == communication._customCmdCode[i]){
					if(communication._custom_ext_buff[i] != NULL)
						memcpy(communication._custom_ext_buff[i] + communication._interface.socketPktPtr - 1, communication._tempPkt.dataPtr, communication._tempPkt.nParam);
					communication._interface.socketPktPtr += communication._tempPkt.nParam;
					// call user-defined function
					if(communication._userCallbacks[i] != NULL)
						communication._userCallbacks[i](communication._interface.socketPktPtr - 1);
					break;
				}
			}
			communication._interface.socketPktPtr = 0;
		}
	}
	return;
  }
  
  // check all other commands
  switch(communication._tempPkt.cmd){
	case NONE: // used for synchronization aim
		communication._interface.enableRx();
		communication._interface.ack();
	break;

	case SYSTEMCMD_RESPONSE:
		memcpy(gpioData, &communication._tempPkt.dataPtr[1], 4); // consider 32-bit responses maximum
		communication._responseType = SYSTEMCMD_RESPONSE;
	break;

	case SYSTEMCMD_REQUEST:
		communication._systemCommand();
	break;

#ifndef COMM_CH_UART
	case AUTOBAUD_UPD:{
		// get baudrate value
		uint32_t baud = communication._tempPkt.dataPtr[1] + (communication._tempPkt.dataPtr[2] << 8) + (communication._tempPkt.dataPtr[3] << 16) +
						(communication._tempPkt.dataPtr[4] << 24);
		// enable Serial at expected baudrate
		Serial.end();
		uart_driver_delete(UART_NUM_0);
		periph_module_disable( PERIPH_UART0_MODULE );
		delay( 20 );
		periph_module_enable( PERIPH_UART0_MODULE );
		Serial.begin(baud);
	}
	break;
#endif

	case GPIO_REQUEST:
		communication._moveGpio();
	break;

	case GPIO_RESPONSE:
		memcpy(gpioData, &communication._tempPkt.dataPtr[1], 4); // consider 32-byte responses maximum
		communication._responseType = GPIO_RESPONSE;
	break;

	case GPIO_INTERRUPT:
		for(uint8_t i = 0; i < communication._extIntNumber; i++){
			if(communication._extInt[i].pin == communication._tempPkt.dataPtr[1]){
				if(communication._extInt[i].args != NULL)
					communication._extInt[i].func(communication._extInt[i].args);
				else
					((voidFuncPtr)communication._extInt[i].func)();
				break;
			}
		}
		communication._interface.notifyAck = true;
	break;

	case SERIAL_STATUS:
		serialStatus = communication._tempPkt.dataPtr[1];
		communication._responseType = SERIAL_STATUS;
		communication._interface.notifyAck = true;
	break;

	// process PUT_CUSTOM_DATA command sent from the other micro
	case PUT_CUSTOM_DATA:
		if (communication._tempPkt.cmdType == DATA_PKT && communication._interface.recPacket.endReceived == false) {
			if(communication._multipacket == false){ // first packet
				if(communication._user_ext_buff == NULL){ // create a safe place where storing data
					communication._user_ext_buff = (uint8_t*)malloc(communication._tempPkt.totalLen);
				}
				if(communication._user_ext_buff != NULL)
					memcpy(communication._user_ext_buff, communication._tempPkt.dataPtr, communication._tempPkt.nParam);
				communication._user_ext_buff_sz = communication._tempPkt.nParam;
				communication._multipacket = true;
			} else { // for all the other intermediate packet nParam will be BUFFER_SIZE
				if(communication._user_ext_buff != NULL)
					memcpy(communication._user_ext_buff + communication._user_ext_buff_sz, communication._tempPkt.dataPtr, communication._tempPkt.nParam);
				communication._user_ext_buff_sz += communication._tempPkt.nParam;
			}
		} else {
			if (communication._tempPkt.nParam == 255) { // 255 indicates a special case where data packet payload fits 1 packet
				if(communication._user_ext_buff == NULL){ // create a safe place where storing data
					communication._user_ext_buff = (uint8_t*)malloc(communication._tempPkt.totalLen + 1);
				}
				if(communication._user_ext_buff != NULL)
					memcpy(communication._user_ext_buff, communication._tempPkt.dataPtr, communication._tempPkt.totalLen + 1);
				communication._user_ext_buff_sz = communication._tempPkt.totalLen + 1;
			} else {
				if(communication._user_ext_buff != NULL)
					memcpy(communication._user_ext_buff + communication._user_ext_buff_sz, communication._tempPkt.dataPtr, communication._tempPkt.nParam);
				communication._user_ext_buff_sz += communication._tempPkt.nParam;
			}
			communication._multipacket = false;
		}
	break;
	
	default:
		// call user defined handlers
		communication._responseType = NONE;
		for(uint8_t i = 0; i < EXT_HANDLES; i++)
			if(communication._handleExt[i] != NULL){
				communication._handleExt[i]();
			}
  }

  return;
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

void Communication::handleCommEvents(){
  // acquire lock
  if(getLock()){

	  while(buffTail != buffHead){ // empty the interrupt buffer
		  _interface.sendCmd(GPIO_INTERRUPT, PARAM_NUMS_1);
		  _interface.sendParam((uint8_t*)&intBuffer[buffTail], PARAM_SIZE_1, LAST_PARAM);
		  buffTail = ++buffTail % INT_BUFF_SIZE;
	  }

#ifndef COMM_CH_UART
	  // check for baudate changes if companion required autobaud
	  if(autobaud){
		uint32_t baud = 0;
		if(uart_get_baudrate(UART_NUM_0, &baud) == ESP_OK){
		  if(currentBaud != baud){
			currentBaud = baud;
			sendCmdPkt(AUTOBAUD_UPD, PARAM_NUMS_1);
			sendParam((uint8_t*)&currentBaud, PARAM_SIZE_4, LAST_PARAM);
		  }
		}
	  }
#endif

	  if (_interface.canProcess()) {
		while(_interface.canProcess()){
		  _interface.endProcessing();
		  if (_interface.read(&_tempPkt) == 0){
		   handle();
		  }
		}
	  }

	  _interface.enableRx(); // enable next packet reception

	  if(_interface.notifyAck){
		_interface.notifyAck = false;
		_interface.ack();
	  }

	  // release lock
	  releaseLock();
  }
}

void Communication::communicationBegin(){
  _interface.begin();
  _interface.enableRx();
  _mutex = xSemaphoreCreateRecursiveMutex();
  communication._ready = true;

}

bool Communication::registerHandle(bool(*func)(void)){
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

bool Communication::registerCommand(uint8_t cmdCode, ext_callback function, void* buffer){
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
  // acquire lock
  if(getLock()){
	handleCommEvents();
	
	bool existing = false;
	
	// check if cmdCode is an already registered one
	for(uint8_t i = 0; i < _cmdIndex; i++){
		if(_customCmdCode[i] == cmdCode){
			existing = true;
			break;
		}
	}
	if(!existing){ // do not send a command that has not been previously registered
		// release lock
		releaseLock();
		return;
	}

	communication.sendDataPkt(CUSTOM_CMD_CODE);
	communication.appendByte(cmdCode);
	communication.appendBuffer((uint8_t*)data, len);
	communication.enableRx();
	// release lock
	releaseLock();
  }
}

void Communication::initExternalBuffer(void* extBuf){
	handleCommEvents();
    _user_ext_buff = (uint8_t*)extBuf;
}

int32_t Communication::availGenericData(void){
  // acquire lock
  if(getLock()){
    handleCommEvents();

    // check if there are new data for our socket
    if (_interface.recPacket.endReceived && _user_ext_buff_sz > 0) {
		// release lock
		releaseLock();
        return _user_ext_buff_sz;
    }
	// release lock
	releaseLock();
    return 0;
  }
  return 0;
}

uint32_t Communication::getGenericData(void *data, uint32_t len){
  // acquire lock
  if(getLock()){
	uint32_t size = _user_ext_buff_sz;
	if(data != NULL){
		// get minimum between len and _user_ext_buff_sz
		if(len > 0)
			size = (len > _user_ext_buff_sz) ? _user_ext_buff_sz : len;
		if(_user_ext_buff == NULL) // if user doens't register a buffer and malloc fails
			size = 0;
		memcpy(data, _user_ext_buff, size);
	}
	_user_ext_buff_sz = 0;
	// release lock
	releaseLock();
	return size;
  }
  return 0;
}

void Communication::sendGenericData(void* data, uint32_t len){
  // acquire lock
  if(getLock()){
    handleCommEvents();
    
	communication.sendDataPkt(PUT_CUSTOM_DATA);
	communication.appendBuffer((uint8_t*)data, len);
	communication.enableRx();
	
	// release lock
	releaseLock();
  }
}

uint8_t Communication::getResponseType(){	  
	return _responseType;
}

void Communication::clearResponseType(){
	_responseType = NONE;
}

void Communication::sendCmdPkt(uint8_t cmd, uint8_t numParam){
  // acquire lock
  if(getLock()){
	_interface.sendCmd(cmd, numParam);
	// release lock
	releaseLock();
  }
}

void Communication::sendDataPkt(uint8_t cmd){
  // acquire lock
  if(getLock()){
	_interface.sendDataPkt(cmd);
	// release lock
	releaseLock();
  }
}

void Communication::sendParam(uint8_t *param, uint8_t param_len, uint8_t lastParam){
  // acquire lock
  if(getLock()){
	_interface.sendParam(param, param_len, lastParam);
	// release lock
	releaseLock();
  }
}

void Communication::sendParam(uint16_t param, uint8_t lastParam){
  // acquire lock
  if(getLock()){
	_interface.sendParam(param, lastParam);
	// release lock
	releaseLock();
  }
}

void Communication::appendByte(uint8_t data, uint8_t lastParam){
  // acquire lock
  if(getLock()){
	_interface.appendByte(data, lastParam);
	// release lock
	releaseLock();
  }
}

void Communication::appendBuffer(const uint8_t *data, uint32_t len){
  // acquire lock
  if(getLock()){
	_interface.appendBuffer(data, len);
	// release lock
	releaseLock();
  }
}

uint8_t* Communication::getRxBuffer(){
	return (uint8_t*)&_tempPkt;
}

bool Communication::endReceived(){
	return _interface.recPacket.endReceived;
}

void Communication::enableRx(){
	if(getLock()){
		_interface.enableRx();
		releaseLock();
	}
}

bool Communication::companionReady(){
	return _ready;
}

bool Communication::getLock(){
  // acquire lock
  if(_mutex != NULL && (xSemaphoreTakeRecursive(_mutex, ( TickType_t )(LOCK_TIMEOUT / portTICK_PERIOD_MS)) == pdTRUE))
		return true;
	return false;
}

void Communication::releaseLock(){
	// release lock
	xSemaphoreGiveRecursive(_mutex);
}


/*
* Process SYSTEMCMD_REQUEST commands
*/
void Communication::_systemCommand(){
	bool rep = false;
	uint8_t op = _tempPkt.dataPtr[1];
	uint32_t val = 0;
	uint32_t result = 0;

	switch (op){
		case SYSTEMCMD_GET_BAUD:{
		  // get baud rate of our companion-connected serial port
		  uint32_t baud = 0;
		  if(uart_get_baudrate(UART_NUM_0, &baud) == ESP_OK)
			result = baud;
		  currentBaud = result;
		  autobaud = true;
		  rep = true;
		}
		break;

		default:
		  rep = false;
		break;
	}

	if (rep == true){
		// send reply
		sendCmdPkt(SYSTEMCMD_RESPONSE, PARAM_NUMS_1);
		sendParam((uint8_t*)&result, PARAM_SIZE_4, LAST_PARAM);
	}
	return;
}

/*
* Process GPIO_REQUEST command by actuating the selected operation on the specified pin
*/
void Communication::_moveGpio(){
	bool rep = false; // no reply needed
	uint8_t op = _tempPkt.dataPtr[1];
	uint8_t pin = _tempPkt.dataPtr[3];
	uint8_t mode;
	uint32_t val = 0;
	uint32_t freq = 0;
	uint32_t result = 0;

	switch(op){
		case PIN_MODE: // pinMode command
			mode = _tempPkt.dataPtr[5]; 
			pinMode(pin, mode);
			_interface.notifyAck = true;
		break;
		case WRITE_DIGITAL: // digitalWrite command
			val = _tempPkt.dataPtr[5] + (_tempPkt.dataPtr[6] << 8) + (_tempPkt.dataPtr[7] << 16) + (_tempPkt.dataPtr[8] << 24);
			digitalWrite(pin, val);
			_interface.notifyAck = true;
		break;
		case WRITE_TOGGLE: // digitalToggle command
			digitalToggle(pin);
			_interface.notifyAck = true;
		break;
		case WRITE_ANALOG: // analogWrite command
			val = _tempPkt.dataPtr[5] + (_tempPkt.dataPtr[6] << 8) + (_tempPkt.dataPtr[7] << 16) + (_tempPkt.dataPtr[8] << 24);
			analogWrite(pin, val);

			_interface.notifyAck = true;
		break;
		case WRITE_ANALOG_F: // analogWriteFreq command
			val = _tempPkt.dataPtr[5] + (_tempPkt.dataPtr[6] << 8) + (_tempPkt.dataPtr[7] << 16) + (_tempPkt.dataPtr[8] << 24);
			freq = _tempPkt.dataPtr[10] + (_tempPkt.dataPtr[11] << 8) + (_tempPkt.dataPtr[12] << 16) + (_tempPkt.dataPtr[13] << 24);
			analogWriteFreq(pin, val, freq);

			_interface.notifyAck = true;
		break;
		case READ_DIGITAL: // digitalRead command	
			result = digitalRead(pin);
			rep = true;
		break;
		case READ_ANALOG: // analogRead commands
			result = analogRead(pin);
			rep = true;
		break;
		case SET_INTERRUPT: // attachInterrupt commands
			_interface.notifyAck = true;
			mode = _tempPkt.dataPtr[5];
			pinMode(pin, INPUT_PULLUP);
			attachInterruptArg(pin, virtualInt, &intPin[pin], mode);
		break;
		case UNSET_INTERRUPT: // detachInterrupt commands
			_interface.notifyAck = true;
			detachInterrupt(pin);
		break;
		default:
		break;
	}
	
	if(rep == true){
	  //set the response struct
	  sendCmdPkt(GPIO_RESPONSE, PARAM_NUMS_1);
	  sendParam((uint8_t*)&result, PARAM_SIZE_4, LAST_PARAM);
	}
	
	return;
}

#endif //COMMUNICATION_LIB_ENABLED
// switchCompanion function is always defined

void Communication::switchCompanion(uint8_t status){
	pinMode(COMPANION_RESET, OUTPUT);
	digitalWrite(COMPANION_RESET, status);
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

  if(communication.getLock()){
	  communication.sendCmdPkt(GPIO_REQUEST, PARAM_NUMS_3);
	  communication.sendParam(&op, PARAM_SIZE_1, NO_LAST_PARAM);
	  communication.sendParam(&pin, PARAM_SIZE_1, NO_LAST_PARAM);
	  communication.sendParam(&mod, PARAM_SIZE_1, LAST_PARAM);
	  communication.enableRx();
	  communication.releaseLock();
  }
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

  if(communication.getLock()){
	  if(op == WRITE_TOGGLE){
		communication.sendCmdPkt(GPIO_REQUEST, PARAM_NUMS_2);
		communication.sendParam(&op, PARAM_SIZE_1, NO_LAST_PARAM);
		communication.sendParam(&pin, PARAM_SIZE_1, LAST_PARAM);
		communication.enableRx();
		communication.releaseLock();
		return;
	  }
	  if(op == WRITE_ANALOG_F)
		communication.sendCmdPkt(GPIO_REQUEST, PARAM_NUMS_4);
	  else
		communication.sendCmdPkt(GPIO_REQUEST, PARAM_NUMS_3);
	  communication.sendParam(&op, PARAM_SIZE_1, NO_LAST_PARAM);
	  communication.sendParam(&pin, PARAM_SIZE_1, NO_LAST_PARAM);
	  if(op == WRITE_ANALOG_F){
		communication.sendParam((uint8_t*)&val, PARAM_SIZE_4, NO_LAST_PARAM);
		communication.sendParam((uint8_t*)&freq, PARAM_SIZE_4, LAST_PARAM);
	  }
	  else
		communication.sendParam((uint8_t*)&val, PARAM_SIZE_4, LAST_PARAM);

	  communication.enableRx();
	  communication.releaseLock();
  }
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

  if(communication.getLock()){
	  communication.sendCmdPkt(GPIO_REQUEST, PARAM_NUMS_2);
	  communication.sendParam(&op, PARAM_SIZE_1, NO_LAST_PARAM);
	  communication.sendParam(&pin, PARAM_SIZE_1, LAST_PARAM);
	  communication.enableRx();
	  communication.releaseLock();
  }
    // Poll response for the next 2 seconds
    uint32_t start = millis();
    while (((millis() - start) < 2000)) {
        communication.handleCommEvents();
        if (communication.getResponseType() == GPIO_RESPONSE) {
			communication.clearResponseType();
			uint32_t result = gpioData[0] + (gpioData[1] << 8) + (gpioData[2] << 16) + (gpioData[3] << 24);
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
* param intr_type: mode applied to interrupt (e.g. RISING, FALLING and so on)
*/
extern "C" void sendAttachInt(uint8_t op, uint8_t pin, voidFuncPtrArg userFunc, void * arg, uint8_t intr_type){
	communication.handleCommEvents();
	// change intr_type following companion mode specification
	uint8_t mode;
	switch(intr_type){
		case RISING:
			mode = 4;
		break;
		case FALLING:
			mode = 3;
		break;
		case CHANGE:
			mode = 2;
		break;
		case ONLOW:
		case ONLOW_WE:
		case LOW:
			mode = 0;
		break;
		case ONHIGH:
		case ONHIGH_WE:
		//case HIGH:
		default:
			mode = 1;
		break;
	}
	if(communication.getLock()){
	  communication.sendCmdPkt(GPIO_REQUEST, PARAM_NUMS_3);
	  communication.sendParam(&op, PARAM_SIZE_1, NO_LAST_PARAM);
	  communication.sendParam(&pin, PARAM_SIZE_1, NO_LAST_PARAM);
	  communication.sendParam(&mode, PARAM_SIZE_1, LAST_PARAM);
	  communication.enableRx();
	  communication.releaseLock();
	}
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
	if(communication.getLock()){
	  communication.sendCmdPkt(GPIO_REQUEST, PARAM_NUMS_2);
	  communication.sendParam(&op, PARAM_SIZE_1, NO_LAST_PARAM);
	  communication.sendParam(&pin, PARAM_SIZE_1, LAST_PARAM);
	  communication.enableRx();
	  communication.releaseLock();
	}
	deleteInterrupt(pin);
}

/*
* Check if serial monitor is open.
* Only companion is connected to USB. Here we ask him if serial monitor has been opened.
*/
bool isSerialOpen(){
	communication.handleCommEvents();

	if(communication.getLock()){
	  communication.sendCmdPkt(SERIAL_STATUS, PARAM_NUMS_0);
	  communication.enableRx();
	  communication.releaseLock();
	}

	// Poll response for the next 2 seconds
	uint32_t start = millis();
	while (((millis() - start) < 2000)) {
		communication.handleCommEvents();
		if (communication.getResponseType() == SERIAL_STATUS) {
			communication.clearResponseType();
			return serialStatus;
		}
	}
	return false;
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

	if (communication.getLock()){
		communication.sendCmdPkt(SYSTEMCMD_REQUEST, PARAM_NUMS_1);
		communication.sendParam(&op, 1, LAST_PARAM);
		communication.enableRx();
		communication.releaseLock();
	}

	// Poll response for the next 2 seconds
	uint32_t start = millis();
	while (((millis() - start) < 2000)){
		communication.handleCommEvents();
		if (communication.getResponseType() == SYSTEMCMD_RESPONSE){
			communication.clearResponseType();
			uint32_t result = gpioData[0] + (gpioData[1] << 8) + (gpioData[2] << 16) + (gpioData[3] << 24);
			return result;
		}
	}
	return -1;
}

#endif // COMMUNICATION_LIB_ENABLED