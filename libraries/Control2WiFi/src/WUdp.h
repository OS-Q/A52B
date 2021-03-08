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

#ifndef wifiudp_h
#define wifiudp_h

#if defined BRIKI_MBC_WB_ESP
#error "Sorry, Control2WiFi library doesn't work with esp32 processor"
#endif

#include <Udp.h>

#define UDP_TX_PACKET_MAX_SIZE 24

class WiFiUDP : public UDP {
private:
  uint8_t _sock;  // socket ID
  uint16_t _port; // local port to listen on

public:
  /**
  * Class construct
  */
  WiFiUDP();
  
  /**
  * initialize, start listening on specified port.
  *
  * @param port: A port number where UDP packets will be listen.
  *
  * @return 1 if successful, 0 if there are no sockets available to use.
  */
  virtual uint8_t begin(uint16_t);
  
  /**
  * Close the UDP socket.
  */
  virtual void stop();

  // Sending UDP packets
  
  /**
  * Start building up a packet to send to the remote host.
  *
  * @param ip: The IP address of the remote host.
  * @param port: The port on which remote host is listening.
  *
  * @return 1 if successful, if there was a problem with the supplied IP address or port.
  */
  virtual int beginPacket(IPAddress ip, uint16_t port);
  
  /**
  * Start building up a packet to send to the remote host.
  *
  * @param host: A char array representing the hostname of the remote host.
  * @param port: The port on which remote host is listening.
  *
  * @return 1 if successful, if there was a problem resolving the hostname or port.
  */
  virtual int beginPacket(const char *host, uint16_t port);
  
  /**
  * Complete the building of a packet and send it to the remote host.
  *
  * @return 1 if the packet was sent successfully, 0 if there was an error.
  */
  virtual int endPacket();

  /**
  * Write a single byte into the packet.
  *
  * @param byte: The byte to be written.
  *
  * @return number of bytes written.
  */
  virtual size_t write(uint8_t);

  /**
  * Write a buffer of bytes into the packet.
  *
  * @param buffer: Data to be written.
  * @param size: Length of the buffer specified as first argument.
  *
  * @return number of bytes written.
  */
  virtual size_t write(const uint8_t *buffer, size_t size);
  
  using Print::write;

  /**
  * Start processing the next available incoming packet.
  *
  * @return the size of the packet in bytes, or 0 if no packets are available.
  */
  virtual int parsePacket();

  /**
  * Get the number of bytes remaining in the current packet.
  *
  * @return number of available bytes or -1 if no byte is available.
  */
  virtual int available();

  /**
  * Read a single byte from the current packet.
  *
  * @return the byte read or -1 if an error occurred.
  */
  virtual int read();

  /**
  * Read data from the current packet.
  *
  * @param buffer: a char array where data will be placed.
  * @param len: the number of bytes to be read.
  *
  * @return the number of bytes read or -1 if an error occurred..
  */
  virtual int read(unsigned char* buffer, size_t len);

  /**
  * Read data from the current packet.
  *
  * @param buffer: a char array where data will be placed.
  * @param len: the number of bytes to be read.
  *
  * @return the number of bytes read or -1 if an error occurred..
  */
  virtual int read(char* buffer, size_t len) { return read((unsigned char*)buffer, len); };
 
  /**
  * Get the next byte from the current packet without moving on to the next byte.
  *
  * @return the peeked byte or -1 if an error occurred.
  */
  virtual int peek();
  
  /**
  * Flush all the available incoming data in the current packet.
  */
  virtual void flush();

  /**
  * Get the IP address of the host who sent the current incoming packet.
  *
  * @return an object representing the IP address of the remote host.
  */
  virtual IPAddress remoteIP();

  /**
  * Get the port of the host who sent the current incoming packet.
  *
  * @return the port of the remote host.
  */
  virtual uint16_t remotePort();

  friend class WiFiDrv;
};

#endif
