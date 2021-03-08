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

#include "Control2WiFi.h"
#include "WServer.h"
#include "utility/CommCmd.h"

WiFiServer::WiFiServer(uint16_t port)
{
    _port = port;
}

void WiFiServer::begin()
{
	Control2WiFi::handleEvents();
	
	Control2WiFi::gotResponse = false;
	Control2WiFi::responseType = 0x00;
 
    uint8_t _sock = Control2WiFi::getSocket();

    if (_sock != NO_SOCKET_AVAIL) {
		if(!SocketDrv::startServer(_port, _sock)){ // packet has not been sent. Maybe an interrupt occurred in the meantime
			// launch interrupt management function, then try to send request again
			uint32_t start = millis();
			while(millis() - start < 10)
				Control2WiFi::handleEvents();
			if(!SocketDrv::startServer(_port, _sock)){ // exit if another error occurs
				Serial.println("Error while starting server");
				return;
			}
		}

		// Poll response for the next 10 seconds. If nothing happens print an error
		uint32_t start = millis();
		while(((millis() - start) < GENERAL_TIMEOUT)){
			Control2WiFi::handleEvents();
			if(Control2WiFi::gotResponse && Control2WiFi::responseType == START_SERVER_TCP){
				if(Control2WiFi::data[0] != 0){
					Control2WiFi::server_port[_sock] = _port;
					return;
				}
				break;
			}
		}
		Serial.println("Error while starting server");
    }
}

WiFiClient WiFiServer::available()
{
	Control2WiFi::handleEvents();
	// search for a socket with a client request
	for(uint8_t i = 0; i < MAX_SOCK_NUM; i++){
		if(Control2WiFi::state[i] == ESTABLISHED){ // there is a client request
			if(Control2WiFi::server_port[i] == _port){ // request refers to our socket
				WiFiClient client(i);
				// restore flag in order to be able to receive other clients on that socket
				return client;
			}
		}
	}
	return WiFiClient(255);
}

size_t WiFiServer::write(uint8_t b)
{
    return write(&b, 1);
}

size_t WiFiServer::write(const uint8_t *buffer, size_t size)
{
	size_t n = 0;

	Control2WiFi::handleEvents();

    for (int sock = 0; sock < MAX_SOCK_NUM; sock++) {
        if (Control2WiFi::server_port[sock] != 0) {
        	WiFiClient client(sock);

            if (Control2WiFi::server_port[sock] == _port && client.status() == ESTABLISHED) {
                n+=client.write(buffer, size);
            }
        }
    }
    return n;
}