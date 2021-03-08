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

#ifndef wificlient_h
#define wificlient_h
#include "Arduino.h"	
#include "Print.h"
#include "Client.h"
#include "IPAddress.h"

class WiFiClient : public Client {

public:
  /**
  * Class constructor.
  */
  WiFiClient();
  
  /**
  * Class constructor.
  *
  * @param sock: A socket number associated to the client.
  */
  WiFiClient(uint8_t sock);

  /**
  * Get client status.
  *
  * @return a number representing client status.
  */
  uint8_t status();

  /**
  * Connect to a remote server.
  *
  * @param ip: Ip address of remote server.
  * @param port: Port of remote server.
  *
  * @return 1 if operation succeeded, 0 otherwise.
  */
  virtual int connect(IPAddress ip, uint16_t port);
  
  /**
  * Connect to a remote server.
  *
  * @param host: Name address of remote server.
  * @param port: Port of remote server.
  *
  * @return 1 if operation succeeded, 0 otherwise.
  */
  virtual int connect(const char *host, uint16_t port);
  
  /**
  * Write a single byte to the remote server.
  *
  * @param b: The byte to be written.
  *
  * @return the number of written bytes.
  */  
  virtual size_t write(uint8_t);

  /**
  * Write data to the remote server.
  *
  * @param buf: Pointer to the data to be written.
  * @param size: Number of bytes to be written.
  *
  * @return the number of written bytes.
  */
  virtual size_t write(const uint8_t *buf, size_t size);

  /**
  * Check if there are available incoming data from remote server we are connected to.
  *
  * @return the number of available bytes or -1 if there are no data.
  */
  virtual int available();

  /**
  * Read a single byte from remote server we are connected to.
  *
  * @return the byte read or -1 if an error occurs.
  */
  virtual int read();

  /**
  * Read data from remote server we are connected to.
  *
  * @param buf: A pointer to the array where data will be placed.
  * @param size: The number of data to be read.
  * 
  * @return the number of bytes read or -1 if an error occurs.
  */
  virtual int read(uint8_t *buf, size_t size);

  /**
  * Peek a single byte from remote server without removing it from the buffer.
  *
  * @return the peeked byte or -1 if an error occurs.
  */
  virtual int peek();

  /**
  * Flush all the available incoming data from the remote server we are connected to.
  */
  virtual void flush();

  /**
  * Close connection to the remote server we are connected to.
  */
  virtual void stop();
  
  /**
  * Check if client is connected to a remote server.
  *
  * @return 1 if client is connected, 0 otherwise.
  */ 
  virtual uint8_t connected();

  /**
  * Operator bool.
  *
  * @return true if client has a valid socket, false otherwise. 
  */
  virtual operator bool();

  friend class WiFiServer;

  using Print::write;

private:
  uint8_t _sock;
  
 /*
  * Get the first available socket
  *
  * return: the number of the first available socket or SOCK_NOT_AVAIL if no socket is available
  */  
  uint8_t getFirstSocket();
};

#endif
