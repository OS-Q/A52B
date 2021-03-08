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

#ifndef Server_Drv_h
#define Server_Drv_h

#include <inttypes.h>
//#include "wifi_spi.h"

typedef enum eProtMode {TCP_MODE, UDP_MODE}tProtMode;

class SocketDrv
{
public:

	/**
	 * Using communication layer, pack and send a command to start a server on a given socket.
	 *
	 * @param port: The port on which server will be listening.
	 * @param sock: The socket number on which start the server.
	 * @param protMode: If TCP_MODE a tcp server will be started,
	 *                  if UDP_MODE and udp server will listen on the given socket.
	 *
	 * @return true if command packet has been sent, false otherwise.
	 */
    static bool startServer(uint16_t port, uint8_t sock, uint8_t protMode=TCP_MODE);

	/**
	 * Using communication layer, pack and send a command to start a connection to a given server.
	 *
	 * @param ipAddress: The IP address of the server we want to connect to.
	 * @param port: The port on which the remote server is listening.
	 * @param sock: The socket number on which client will be listening.
	 * @param protMode: If TCP_MODE a tcp client will be started.
	 *                  If UDP_MODE and udp client will be started.
	 *
	 * @return true if command packet has been sent, false otherwise.
	 */
    static bool startClient(uint32_t ipAddress, uint16_t port, uint8_t sock, uint8_t protMode=TCP_MODE);

	/**
	 * Using communication layer, pack and send a command to disconnect and stop a client.
	 *
	 * @param sock: The socket number on which client is running.
	 *
	 * @return true if command packet has been sent, false otherwise.
	 */
    static bool stopClient(uint8_t sock);

	/**
	 * Using communication layer, pack and send a command to ask for the status of a client.
	 *
	 * @param sock: The socket number on which client is running.
	 *
	 * @return true if command packet has been sent, false otherwise.
	 */
    static bool getClientState(uint8_t sock);

	/**
	 * Using communication layer, pack and send a command to read a byte from a remote connection.
	 *
	 * @param sock: The socket number from which read the data.
	 * @param peek: If 0, the data will be read and discarded from the buffer.
	 *              If 1, the data will be read but will remain in the buffer.
	 *
	 * @return true if command packet has been sent, false otherwise.
	 */
    static bool getData(uint8_t sock, uint8_t peek = 0);

	/**
	 * Using communication layer, pack and send a command to read a buffer of bytes from a remote connection.
	 *
	 * @param sock: The socket number from which read the data.
	 * @param len: The length of the data to be read.
	 *
	 * @return true if command packet has been sent, false otherwise.
	 */
    static bool getDataBuf(uint8_t sock, uint16_t len = 25);

	/**
	 * Using communication layer, pack and send a command to write a buffer of bytes to a remote connection.
	 *
	 * @param sock: The socket number to which write the data.
	 * @param data: The data to write.
	 * @param len: The length of the data to write.
	 *
	 * @return true if command packet has been sent, false otherwise.
	 */
    static bool sendData(uint8_t sock, const uint8_t *data, uint16_t len);

    /**
	 * Using communication layer, pack and send a command to end an UDP packet and send the previously initialized data.
	 *
	 * @param sock: The socket number to which write the data.
	 *
	 * @return true if command packet has been sent, false otherwise.
	 */
	static bool sendUdpData(uint8_t sock);
};

#endif
