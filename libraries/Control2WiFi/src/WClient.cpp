/*
Meteca SA.  All right reserved.
created by Dario Trimarchi and Chiara Ruggeri 2017-2018
email: support@meteca.org

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

extern "C" {
  #include "utility/wl_definitions.h"
  #include "utility/wl_types.h"
  #include "string.h"
}

#include "Control2WiFi.h"
#include "WClient.h"
#include "utility/socket_drv.h"
#include "utility/wifi_drv.h"
#include "utility/CommCmd.h"

#define ATTEMPTS 350

uint8_t client_status = 0;
int attempts_conn = 0;

uint8_t _internalBuf[25];
uint8_t _internalBufPtr = 0;
int16_t _internalBufSz = 0;
uint16_t _sockDataSz = 0;


WiFiClient::WiFiClient() : _sock(NO_SOCKET_AVAIL) {
}

WiFiClient::WiFiClient(uint8_t sock) : _sock(sock) {
}

int WiFiClient::connect(const char* host, uint16_t port) 
{
	IPAddress remote_addr;
	if (WiFi.hostByName(host, remote_addr)) {
		return connect(remote_addr, port);
	}
	return 0;
}

int WiFiClient::connect(IPAddress ip, uint16_t port) 
{
	_sock = getFirstSocket();
	if (_sock != NO_SOCKET_AVAIL) {
		Control2WiFi::handleEvents();
		
		Control2WiFi::gotResponse = false;
		Control2WiFi::responseType = NONE;
    	
		if(!SocketDrv::startClient(uint32_t(ip), port, _sock)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
			// launch interrupt management function, then try to send request again
			uint32_t start = millis();
			while(millis() - start < 10)
				Control2WiFi::handleEvents();
			if(!SocketDrv::startClient(uint32_t(ip), port, _sock)) // exit if another error occurs
			return 0;
		}

		// Poll flags until we got a response or timeout occurs
		uint32_t start = millis();
		while(((millis() - start) < GENERAL_TIMEOUT)){
			Control2WiFi::handleEvents();
			if(Control2WiFi::gotResponse && Control2WiFi::responseType == START_CLIENT_TCP){
				if(Control2WiFi::data[0]){ // client started correctly
					Control2WiFi::state[_sock] = ESTABLISHED;
					return 1;
				}
				break;
			}
		}
    }
	else{
    	Serial.println("No Socket available");
    	return 0;
    }

    return 0;
}

size_t WiFiClient::write(uint8_t b) 
{
	  return write(&b, 1);
}

size_t WiFiClient::write(const uint8_t *buf, size_t size)
{
	Control2WiFi::handleEvents();
	
	if(!SocketDrv::sendData(_sock, buf, size)) { // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		uint32_t start = millis();
		while(millis() - start < 10)
			Control2WiFi::handleEvents();
		if(!SocketDrv::sendData(_sock, buf, size)) // exit if another error occurs
			return 0;
	}
	return size;
}

int WiFiClient::available() 
{
	Control2WiFi::handleEvents();
	
	if(_sock != 255){
		if(_internalBufSz > 0)
			return _internalBufSz;
		
		// check if there are new data for our socket 
		if(Control2WiFi::client_data[_sock] != 0){
			_sockDataSz = Control2WiFi::client_data[_sock];
			Control2WiFi::client_data[_sock] = 0;
		}
		if(_sockDataSz > 0)
			return _sockDataSz;
	}
	return -1;
}

int WiFiClient::read()
{
	if(_internalBufSz > 0){
		uint8_t ret = _internalBuf[_internalBufPtr++];
		_internalBufSz--;
		
		return ret;
	}

	if (available() <= 0)
		return -1;
	
	Control2WiFi::handleEvents();
	
	Control2WiFi::gotResponse = false;
	Control2WiFi::responseType = NONE;

	uint16_t sz = min(_sockDataSz, 25);

	if(!SocketDrv::getDataBuf(_sock, sz)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		uint32_t start = millis();
		while(millis() - start < 10)
			Control2WiFi::handleEvents();
		if(!SocketDrv::getDataBuf(_sock, sz)) // exit if another error occurs
			return -1;
	}
	// Poll flags until we got a response or timeout occurs
	uint32_t start = millis();

	while(((millis() - start) < 4000)){
		Control2WiFi::handleEvents();
		if(Control2WiFi::gotResponse && Control2WiFi::responseType == GET_DATABUF_TCP){
			_internalBufPtr = 0;
			_internalBufSz = (int8_t)Control2WiFi::dataLen;
			memset(_internalBuf, 0, 25);
			if(_internalBufSz <= 0){
				_sockDataSz = 0;
				_internalBufSz = 0;
				
				return -1;
			}
			memcpy(_internalBuf, (uint8_t *)Control2WiFi::data, _internalBufSz);
			
			uint8_t ret = _internalBuf[_internalBufPtr++];
			if(_internalBufSz != sz)
				_sockDataSz = _internalBufSz;
			_sockDataSz -= _internalBufSz;
			_internalBufSz--;

			return ret;
		}
	}
	return -1;
}

int WiFiClient::read(uint8_t* buf, size_t size) {
	Control2WiFi::handleEvents();
	Control2WiFi::gotResponse = false;
	Control2WiFi::responseType = NONE;

	if(!SocketDrv::getDataBuf(_sock, size)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		uint32_t start = millis();
		while(millis() - start < 10)
			Control2WiFi::handleEvents();
		if(!SocketDrv::getDataBuf(_sock, size)) // exit if another error occurs
		return -1;
	}

	// Poll flags until we got a response or timeout occurs
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		Control2WiFi::handleEvents();
		if(Control2WiFi::gotResponse && Control2WiFi::responseType == GET_DATABUF_TCP){
			uint16_t receivedBytes;
			uint8_t chunkSize = 0;
			int32_t totalLen = Control2WiFi::totalLen;

			if(totalLen <= 0){ // No data was read. Maybe an error occurred. Restore available flag and exit
				Control2WiFi::client_data[_sock] = 0;
				_sockDataSz = 0;
				return -1;
			}

			receivedBytes = Control2WiFi::dataLen;
			
			memcpy(buf, (uint8_t *)Control2WiFi::data, receivedBytes);

			while(receivedBytes < totalLen && (millis() - start) < 15000){ // wait 15 seconds at maximum for receiving the whole message. Otherwise consider that an error occurred
				chunkSize = Control2WiFi::getAvailableData();
				if(chunkSize > 0){
					memcpy(&buf[receivedBytes], (void *)Control2WiFi::data, chunkSize);
					receivedBytes += chunkSize;
				}
			}
			Control2WiFi::client_data[_sock] -= receivedBytes;
			_sockDataSz -= receivedBytes;
			return receivedBytes;
		}
	}

	Control2WiFi::client_data[_sock] = 0;
	_sockDataSz = 0;
	return -1;
}

int WiFiClient::peek() {
	
	if(_internalBufSz > 0){
		uint8_t ret = _internalBuf[_internalBufPtr];
		return ret;
	}
	
	if (available() <= 0)
		return -1;

	Control2WiFi::handleEvents();

	Control2WiFi::gotResponse = false;
	Control2WiFi::responseType = NONE;

	uint8_t sz = min(_sockDataSz, 25);
	
	if(!SocketDrv::getDataBuf(_sock, sz)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		uint32_t start = millis();
		while(millis() - start < 10)
			Control2WiFi::handleEvents();
		if(!SocketDrv::getDataBuf(_sock, sz)) // exit if another error occurs
		return -1;
	}

	// Poll flags until we got a response or timeout occurs
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		Control2WiFi::handleEvents();
		if(Control2WiFi::gotResponse && Control2WiFi::responseType == GET_DATABUF_TCP){
			_internalBufPtr = 0;
			_internalBufSz = (int16_t)Control2WiFi::dataLen;
			memset(_internalBuf, 0, 25);
			if(_internalBufSz <= 0){
				_sockDataSz = 0;
				_internalBufSz = 0;
				return -1;
			}
			memcpy(_internalBuf, (uint8_t *)Control2WiFi::data, _internalBufSz);
			
			uint8_t ret = _internalBuf[_internalBufPtr];
			_sockDataSz -= _internalBufSz;
						
			return ret;
		}
	}
	
	return -1;
}

void WiFiClient::flush() {
  while (available() > 0)
    read();
}

void WiFiClient::stop() {

	if (_sock == 255)
		return;

	Control2WiFi::handleEvents();
	
	Control2WiFi::gotResponse = false;
	Control2WiFi::responseType = NONE;
	
	if(!SocketDrv::stopClient(_sock)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		uint32_t start = millis();
		while(millis() - start < 10)
			Control2WiFi::handleEvents();
		if(!SocketDrv::stopClient(_sock)) // exit if another error occurs
			return;
	}

    // Poll flags until we got a response or timeout occurs
    uint32_t start = millis();
    while(((millis() - start) < GENERAL_TIMEOUT)){
	    Control2WiFi::handleEvents();
	    if(Control2WiFi::gotResponse && Control2WiFi::responseType == STOP_CLIENT_TCP){
		    break;
	    }
    }


  Control2WiFi::state[_sock] = CLOSED;
  _sockDataSz = _internalBufPtr = _internalBufSz = 0;
  client_status = 0;
  _sock = 255;
}

uint8_t WiFiClient::connected() {

  if (_sock == 255) {
    return 0;
  } else {
      uint8_t s;
      attempts_conn++;
      if(client_status == 0 || attempts_conn > ATTEMPTS){    //EDIT by Andrea
        client_status = status();
        s = client_status;
        attempts_conn = 0;
      }
      else
        s = client_status;

        // client_status = status();
        // s = client_status;
	return !(s == CLOSED);
  }
}

uint8_t WiFiClient::status() {
    if (_sock == 255)
	    return CLOSED;

	Control2WiFi::handleEvents();
	return Control2WiFi::state[_sock];
}

WiFiClient::operator bool() {
  return _sock != 255;
}

// Private Methods
uint8_t WiFiClient::getFirstSocket()
{
    for (int i = 0; i < MAX_SOCK_NUM; i++) {
      if (Control2WiFi::state[i] == CLOSED)
      {
          return i;
      }
    }
    return NO_SOCKET_AVAIL;
}