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

#include <string.h>
#include "utility/socket_drv.h"
#include "utility/wifi_drv.h"
#include "utility/CommCmd.h"

#include "Control2WiFi.h"
#include "WUdp.h"

uint16_t _sockDataSzUdp = 0;
uint8_t _internalBufUdp[25];
uint8_t _internalBufPtrUdp = 0;
uint16_t _internalBufSzUdp = 0;


/* Constructor */
WiFiUDP::WiFiUDP() : _sock(NO_SOCKET_AVAIL) {}

/* Start WiFiUDP socket, listening at local port PORT */
uint8_t WiFiUDP::begin(uint16_t port) {

	uint8_t sock = Control2WiFi::getSocket();
	if (sock != NO_SOCKET_AVAIL)
	{
		if(!SocketDrv::startServer(port, sock, UDP_MODE)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
			// launch interrupt management function, then try to send request again
			uint32_t start = millis();
			while(millis() - start < 10)
				Control2WiFi::handleEvents();
			if(!SocketDrv::startServer(port, sock, UDP_MODE)) // exit if another error occurs
			return 0;
		}

		// Poll response for the next 10 seconds
		uint32_t start = millis();
		while(((millis() - start) < GENERAL_TIMEOUT)){
			Control2WiFi::handleEvents();
			if(Control2WiFi::gotResponse && Control2WiFi::responseType == START_SERVER_TCP){
				if(Control2WiFi::data[0] != 0){
					Control2WiFi::server_port[sock] = port;
					_sock = sock;
					_port = port;
					return 1;
				}
			}
		}
	}
	return 0;

}

/* return number of bytes available in the current packet*/
int WiFiUDP::available() {
	Control2WiFi::handleEvents();

	if(_sock != NO_SOCKET_AVAIL){
		if((int16_t)_internalBufSzUdp > 0)
		return _internalBufSzUdp;
		
		// check if there are new data for our socket
		if(Control2WiFi::client_data[_sock] != 0){
			_sockDataSzUdp = Control2WiFi::client_data[_sock];
			Control2WiFi::client_data[_sock] = 0;
		}
		if((int16_t)_sockDataSzUdp > 0)
		return _sockDataSzUdp;
	}
	return -1;
}

/* Release any resources being used by this WiFiUDP instance */
void WiFiUDP::stop(){
	if (_sock == NO_SOCKET_AVAIL)
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
	_sockDataSzUdp = _internalBufPtrUdp = _internalBufSzUdp = 0;
	_sock = NO_SOCKET_AVAIL;
}

int WiFiUDP::beginPacket(const char *host, uint16_t port)
{
	// Look up the host first
	IPAddress remote_addr;
	if (WiFi.hostByName(host, remote_addr)) {
		return beginPacket(remote_addr, port);
	}
	return 0;
}

int WiFiUDP::beginPacket(IPAddress ip, uint16_t port)
{
  if (_sock == NO_SOCKET_AVAIL)
	  _sock = Control2WiFi::getSocket();
  if (_sock != NO_SOCKET_AVAIL)
  {
	Control2WiFi::handleEvents();
	  		
	Control2WiFi::gotResponse = false;
	Control2WiFi::responseType = NONE;

	if(!SocketDrv::startClient(uint32_t(ip), port, _sock, UDP_MODE)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		uint32_t start = millis();
		while(millis() - start < 10)
			Control2WiFi::handleEvents();
		if(!SocketDrv::startClient(uint32_t(ip), port, _sock, UDP_MODE)) // exit if another error occurs
		return 0;
	}

	// Poll flags until we got a response or timeout occurs
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		Control2WiFi::handleEvents();
		if(Control2WiFi::gotResponse && Control2WiFi::responseType == START_CLIENT_TCP){
			if(!Control2WiFi::data[0]) // error while starting client
				return 0;
			break;
		}
	}

	Control2WiFi::state[_sock] = ESTABLISHED;
	return 1;
  }
  return 0;
}

int WiFiUDP::endPacket()
{
	Control2WiFi::handleEvents();
		
	Control2WiFi::gotResponse = false;
	Control2WiFi::responseType = NONE;

	if(!SocketDrv::sendUdpData(_sock)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		uint32_t start = millis();
		while(millis() - start < 10)
			Control2WiFi::handleEvents();
		if(!SocketDrv::sendUdpData(_sock)) // exit if another error occurs
		return 0;
	}

	// Poll flags until we got a response or timeout occurs
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		Control2WiFi::handleEvents();
		if(Control2WiFi::gotResponse && Control2WiFi::responseType == SEND_DATA_UDP){
			if(Control2WiFi::data[0] == 1){
				//reset data available
				_internalBufSzUdp = 0;
				_sockDataSzUdp = 0;
				return 1;
			}
			break;
		}
	}
	return 0;
}

size_t WiFiUDP::write(uint8_t byte)
{
  return write(&byte, 1);
}

size_t WiFiUDP::write(const uint8_t *buffer, size_t size)
{
	Control2WiFi::handleEvents();
	
	if(!SocketDrv::sendData(_sock, buffer, size)) { // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		uint32_t start = millis();
		while(millis() - start < 10)
			Control2WiFi::handleEvents();
		if(!SocketDrv::sendData(_sock, buffer, size)) // exit if another error occurs
		return 0;
	}

	return size;
}

int WiFiUDP::parsePacket()
{
	return available();
}

int WiFiUDP::read()
{
	if(_internalBufSzUdp > 0){
		uint8_t ret = _internalBufUdp[_internalBufPtrUdp++];
		_internalBufSzUdp--;
		
		return ret;
	}
	if (available() <= 0)
		return -1;
	

	
	Control2WiFi::handleEvents();
	
	Control2WiFi::gotResponse = false;
	Control2WiFi::responseType = NONE;

	uint16_t sz = min(_sockDataSzUdp, 25);
	
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
			_internalBufPtrUdp = 0;
			_internalBufSzUdp = (int16_t)Control2WiFi::dataLen;
			memset(_internalBufUdp, 0, 25);
			if(_internalBufSzUdp <= 0){
				_sockDataSzUdp = 0;
				_internalBufSzUdp = 0;
		
				return -1;
			}
			memcpy(_internalBufUdp, (uint8_t *)Control2WiFi::data, _internalBufSzUdp);
				
			uint8_t ret = _internalBufUdp[_internalBufPtrUdp++];
			if(_internalBufSzUdp != sz)
				_sockDataSzUdp = _internalBufSzUdp;
			_sockDataSzUdp -= _internalBufSzUdp;
			_internalBufSzUdp--;
		
			return ret;

		}
	}
		
	return -1;
}

int WiFiUDP::read(unsigned char* buffer, size_t len)
{
	Control2WiFi::handleEvents();
	
	Control2WiFi::gotResponse = false;
	Control2WiFi::responseType = NONE;

	
	if(!SocketDrv::getDataBuf(_sock, len)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		uint32_t start = millis();
		while(millis() - start < 10)
			Control2WiFi::handleEvents();
		if(!SocketDrv::getDataBuf(_sock, len)) // exit if another error occurs
		return -1;
	}
	
	// Poll flags until we got a response or timeout occurs
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		Control2WiFi::handleEvents();
		if(Control2WiFi::gotResponse && Control2WiFi::responseType == GET_DATABUF_TCP){

			uint16_t receivedBytes;
			uint8_t chunkSize = 0;
			uint32_t totalLen = Control2WiFi::totalLen;

			if(totalLen <= 0){ // No data was read. Maybe an error occurred. Restore available flag and exit
				Control2WiFi::client_data[_sock] = 0;
				_sockDataSzUdp = 0;
				return -1;
			}

			receivedBytes = Control2WiFi::dataLen;

			memcpy(buffer, (uint8_t *)Control2WiFi::data, receivedBytes);

			while(receivedBytes < totalLen && (millis() - start) < 15000){ // wait 15 seconds at maximum for receiving the whole message. Otherwise consider that an error occurred
				chunkSize = Control2WiFi::getAvailableData();
				if(chunkSize > 0){
					memcpy(&buffer[receivedBytes], (void *)Control2WiFi::data, chunkSize);
					receivedBytes += chunkSize;
				}
			}
			Control2WiFi::client_data[_sock] -= receivedBytes;
			_sockDataSzUdp -= receivedBytes;
			return receivedBytes;
		}
	}
	Control2WiFi::client_data[_sock] = 0;
	_sockDataSzUdp = 0;
	return -1;
}

int WiFiUDP::peek()
{
	if(_internalBufSzUdp > 0){
		uint8_t ret = _internalBufUdp[_internalBufPtrUdp];
		return ret;
	}
	
	if (available() <= 0)
	return -1;

	Control2WiFi::handleEvents();

	Control2WiFi::gotResponse = false;
	Control2WiFi::responseType = NONE;

	uint8_t sz = min(_sockDataSzUdp, 25);
	
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
			_internalBufPtrUdp = 0;
			_internalBufSzUdp = (int16_t)Control2WiFi::dataLen;
			memset(_internalBufUdp, 0, 25);
			if(_internalBufSzUdp <= 0){
				_sockDataSzUdp = 0;
				_internalBufSzUdp = 0;
				return -1;
			}
			memcpy(_internalBufUdp, (uint8_t *)Control2WiFi::data, _internalBufSzUdp);
			
			uint8_t ret = _internalBufUdp[_internalBufPtrUdp];
			_sockDataSzUdp -= _internalBufSzUdp;
			return ret;

		}
	}
	return -1;
}

void WiFiUDP::flush()
{
  while (available())
    read();
}

IPAddress  WiFiUDP::remoteIP()
{
	uint8_t _remoteIp[4] = {0};

	Control2WiFi::handleEvents();

	Control2WiFi::gotResponse = false;
	Control2WiFi::responseType = NONE;

	if(!WiFiDrv::getRemoteData(_sock)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		uint32_t start = millis();
		while(millis() - start < 10)
			Control2WiFi::handleEvents();
		if(!WiFiDrv::getRemoteData(_sock)){ // exit if another error occurs
			IPAddress ip(_remoteIp);
			return ip;
		}
	}

	// Poll flags until we got a response or timeout occurs
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		Control2WiFi::handleEvents();
		if(Control2WiFi::gotResponse && Control2WiFi::responseType == GET_REMOTE_DATA){
			if(Control2WiFi::dataLen == 4){ // parse received ip address
				memcpy(_remoteIp, (void*)Control2WiFi::data, 4);
			}
			break;
		}
	}
	IPAddress ip(_remoteIp);
	return ip;
}

uint16_t  WiFiUDP::remotePort()
{
	uint16_t port = 0;

	Control2WiFi::handleEvents();

	Control2WiFi::gotResponse = false;
	Control2WiFi::responseType = NONE;

	if(!WiFiDrv::getRemoteData(_sock)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
		// launch interrupt management function, then try to send request again
		uint32_t start = millis();
		while(millis() - start < 10)
			Control2WiFi::handleEvents();
		if(!WiFiDrv::getRemoteData(_sock)) // exit if another error occurs
			return port;
	}

	// Poll flags until we got a response or timeout occurs
	uint32_t start = millis();
	while(((millis() - start) < GENERAL_TIMEOUT)){
		Control2WiFi::handleEvents();
		if(Control2WiFi::gotResponse && Control2WiFi::responseType == GET_REMOTE_DATA){
			if(Control2WiFi::dataLen == 4){ // response seems ok, we've got Ip as first parameter and port as second one
				port = (Control2WiFi::data[5]<<8) + Control2WiFi::data[6];
			}
			break;
		}
	}
	return port;
}
