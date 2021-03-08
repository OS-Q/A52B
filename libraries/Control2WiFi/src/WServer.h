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

#ifndef wifiserver_h
#define wifiserver_h

extern "C" {
  #include "utility/wl_definitions.h"
}

#include "Server.h"

class WiFiClient;

class WiFiServer : public Server {
private:
  uint16_t _port;
public:

  /**
  * Class construct
  *
  * @param port: A port number where server will listen.
  */
  WiFiServer(uint16_t);
  
  /**
  * Check if there are clients connected to our server.
  *
  * @return a WiFiClient object representing the remote client connected to us.
  */
  WiFiClient available();

  /**
  * Start the server on the port given by constructor.
  */
  void begin();
  
  /**
  * Write a single byte on a client connected to our server.
  *
  * @param b: The byte to be sent.
  *
  * @return number of byte written.
  */
  virtual size_t write(uint8_t);

 /**
  * Write data on a client connected to our server.
  *
  * @param buf: Pointer to the data.
  * @param size: Length of the data to be sent.
  *
  * @return number of byte written.
  */
  virtual size_t write(const uint8_t *buf, size_t size);

  using Print::write;
};

#endif
