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

#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include "communication_def.h"
#include "interface.h"
#include "WInterrupts.h"
#include "mbc_config.h"


#define COMPANION_OFF	0
#define COMPANION_ON	1

#ifdef COMMUNICATION_LIB_ENABLED


#define CUSTOM_CMD_CODE 	0x50
#define CUSTOM_CMD_NR		5

#define EXT_HANDLES 4

#define COMPANION_INTERRUPTS	40

#define SYSTEMCMD_GET_BAUD        0x01

typedef void (*ext_callback)(uint32_t);

typedef struct{
	uint8_t pin;
	voidFuncPtrArg func;
	void* args;
}externalInt;

#endif // COMMUNICATION_LIB_ENABLED

class Communication
{
#ifdef COMMUNICATION_LIB_ENABLED
private:
	bool _endReceived = true;
	uint32_t _totPktLen = 0;
	uint8_t* _custom_ext_buff[CUSTOM_CMD_NR] = { NULL, NULL, NULL, NULL, NULL };
	uint32_t _custom_buff_index = 0;
	uint8_t* _user_ext_buff = NULL;
	uint32_t _user_ext_buff_sz = 0;
	uint8_t _cmdIndex = 0;
	uint8_t _customCmdCode[CUSTOM_CMD_NR];
	ext_callback _userCallbacks[CUSTOM_CMD_NR] = { NULL, NULL, NULL, NULL, NULL };
	void(*_handleExt[EXT_HANDLES])(uint8_t*) = {NULL, NULL, NULL, NULL};
	uint8_t _currentCmd;
	uint8_t _customCurrentCmd;
	uint8_t _rxBuf[BUF_SIZE];
	uint8_t _responseType;
	externalInt _extInt[COMPANION_INTERRUPTS];
	uint8_t _extIntNumber = 0;
	bool autobaud = false;
	uint32_t currentBaud = 0;

	Interface _interface;

	/*
	* Process SYSTEMCMD_REQUEST command
	*/
	void _systemCommand();

	/*
	* Actuates received command GPIO related
	*/
	void _moveGpio();
		
	/*
	* Callback function called to check and parse received communication packet
	*/
	friend void communicationEvtCb(uint8_t* pkt);

	friend void registerInterrupt(uint8_t pin, voidFuncPtrArg userFunc, void * arg);
	friend void deleteInterrupt(uint8_t pin);

public:
	/**
	* Initialize communication between control and communication chip and register the main callback
	* that will be called everytime a new command is received.
	*/
	void communicationBegin();

	/**
	* register a custom command in order to be able to send and receive custom messages from communication microcontroller.
	*
	* @param cmdCode: The code associated to the custom command.
	* @param function: A callback that will be called every time the custom command so created is received.
	* @param buffer: A container of the custom message sent from the control microcontroller.
	*
	* @return true if operation succedeed, false otherwise.
	*/  
	bool registerCommand(uint8_t cmdCode, ext_callback function, void* buffer);

	/**
	* send a pre-registered custom command to communication microcontroller.
	*
	* @param cmdCode: The code associated to the custom command previously registered.
	* @param data: The data to be sent.
	* @param len: Length of data to be sent.
	*/
	void sendCommand(uint8_t cmdCode, void* data, uint32_t len);

	/**
	* Initializes an external buffer for generic communications.
	*
	* @param extBuf: A pointer to the buffer that will store data.
	*/
	void initExternalBuffer(void *extBuf);

	/**
	* Send a custom data packet to communication microcontroller.
	*
	* @param data: Pointer to the data to be sent.
	* @param len: Length of the data to be sent.
	*/
	void sendGenericData(void *data, uint32_t len);

	/**
	* Check if some data were received from communication microcontroller.
	*
	* @return number of available data if present, -1 otherwise.
	*/
	int32_t availGenericData(void);

	/**
	* Retrieve custom data received. If an external buffer was previously specified with initExternalBuffer
	* this function will just clear available data flag since data are already available in buffer.<br>
	* Otherwise data will be stored in a dynamically created buffer and with this function a copy of
	* the available data will be pasted in the pointer passed as first parameter.
	*
	* @param data: Pointer where data read will be placed.
	* @param len: Length of the data to read.
	* @return number of bytes read.
	*/
	uint32_t getGenericData(void *data = NULL, uint32_t len = 0);

	/**
	* Register an handle function to process commands in external libraries.
	* Registering may fails if the maximum number of allowed handlers is reached.
	* See EXT_HANDLES macro to know the maximum number of allowed handlers
	*
	* @param func: Pointer to the function that will handle commands. The command will be passed
	*             as uint8_t pointer to the function.
	*
	* @return true if handle was succesfully registered, false otherwise.
	*/
	bool registerHandle(void(*func)(uint8_t*));

	/**
	* handles communication events to see if an ISR has been detected
	*/
	void handleCommEvents(void);
	
	/**
	* Initialize response type to NONE
	*/
	void clearResponseType();
	
	/**
	* Get values of response type.
	*
	* @return a value representing the command to which response refers to.
	*/
	uint8_t getResponseType();
	
	/**
	* Start sending a command packet to the companion chip with the specified command.
	*
	* @param cmd: The command code.
	* @param numParam: Number of params the packet will contain. If 0 the packet will be closed and sent immediately.
	*                  If it's greater than 0, other parameters can be added to the packet with sendParam function.
	*
	* @return true if there are other parameters, or the result of the send command on the interface if the packet
	*         was sent with just the command.
	*/
	bool sendCmdPkt(uint8_t cmd, uint8_t numParam);

	/**
	* Start sending a data packet to the companion chip with the specified command.
	*
	* @param cmd: The command code
	*/
	void sendDataPkt(uint8_t cmd);
	
	/**
	* Add a parameter to the previously initialized packet.
	*
	* @param param: A pointer to the parameter.
	* @param param_len: The length of the parameter.
	* @param lastParam: If NO_LAST_PARAM is specified other parameter can be added by calling the same function again.
	*					If LAST_PARAM is specified the packet will be closed and sent immediately.
	*
	* @return true if there are other parameters, or the result of the send command on the interface if this was the
	*         last param in the packet.      
	*/
	bool sendParam(uint8_t *param, uint8_t param_len, uint8_t lastParam = NO_LAST_PARAM);
	
	/**
	* Add a parameter to the previously initialized packet.
	*
	* @param param: A 2-byte long parameter.
	* @param lastParam: If NO_LAST_PARAM is specified other parameter can be added by calling the same function again.
	*					If LAST_PARAM is specified the packet will be closed and sent immediately.
	*
	* @return true if there are other parameters, or the result of the send command on the interface if this was the
	*         last param in the packet.
	*/
	bool sendParam(uint16_t param, uint8_t lastParam = NO_LAST_PARAM);

	/**
	* Add a byte to the previously initialized packet.
	*
	* @param data: The byte to be added to the packet.
	* @param lastParam: If NO_LAST_PARAM is specified other parameter can be added by calling the same function again.
	*					If LAST_PARAM is specified the packet will be closed and sent immediately.
	*
	* @return true if there are other parameters, or the result of the send command on the interface if this was the
	*         last param in the packet.      
	*/	
	bool appendByte(uint8_t data, uint8_t lastParam = NO_LAST_PARAM);
	
	/**
	* Add a buffer to the previously initialized packet. After calling this function the packet will
	* be closed and send immediately.
	*
	* @param data: The buffer to be added to the packet.
	* @param len: The size of the buffer to be appended.
	*
	* @return the result of the sending packet on the interface.
	*/
	bool appendBuffer(const uint8_t *data, uint32_t len);
	
	/**
	* Get a reference to the buffer storing data received from interface.
	*
	* @return a pointer to the data.
	*/
	uint8_t* getRxBuffer();

#endif // COMMUNICATION_LIB_ENABLED
// switchCompanion is always declared

	/**
	* Turn companion on/off. Can be used to save power when companion is not needed.
	*
	* @param status: COMPANION_OFF to turn off companion - COMPANION_ON if companion has to be woken up
	*/
	void switchCompanion(uint8_t status = COMPANION_ON);
};


#ifdef COMMUNICATION_LIB_ENABLED
int32_t sendSystemGet(uint8_t op);

/**
* Get the baud rate value uart (sercom5) from registers 
*	 
*    
*  @return baud rate 
*/
uint32_t getBaudRateReg(void);

#endif // COMMUNICATION_LIB_ENABLED

extern Communication communication;
#endif // COMMUNICATION_H