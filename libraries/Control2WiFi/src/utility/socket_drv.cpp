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

#include "socket_drv.h"
#include "wifi_drv.h"
#include "CommCmd.h"

#include "communication.h"

#include "Arduino.h"

extern "C" {
#include "utility/wl_types.h"
}


// Start server TCP on port specified
bool SocketDrv::startServer(uint16_t port, uint8_t sock, uint8_t protMode)
{
    communication.sendCmdPkt(START_SERVER_TCP, PARAM_NUMS_3);
    communication.sendParam(port);
    communication.sendParam(&sock, 1);
    return communication.sendParam(&protMode, 1, LAST_PARAM);
}

// Start server TCP on port specified
bool SocketDrv::startClient(uint32_t ipAddress, uint16_t port, uint8_t sock, uint8_t protMode)
{
    communication.sendCmdPkt(START_CLIENT_TCP, PARAM_NUMS_4);
    communication.sendParam((uint8_t*)&ipAddress, sizeof(ipAddress));
    communication.sendParam(port);
    communication.sendParam(&sock, 1);
    return communication.sendParam(&protMode, 1, LAST_PARAM);
}

// Start server TCP on port specified
bool SocketDrv::stopClient(uint8_t sock)
{
    communication.sendCmdPkt(STOP_CLIENT_TCP, PARAM_NUMS_1);
    return communication.sendParam(&sock, 1, LAST_PARAM);
}

bool SocketDrv::getClientState(uint8_t sock)
{
    communication.sendCmdPkt(GET_CLIENT_STATE_TCP, PARAM_NUMS_1);
    return communication.sendParam(&sock, sizeof(sock), LAST_PARAM);
}


bool SocketDrv::getData(uint8_t sock, uint8_t peek)
{
    communication.sendCmdPkt(GET_DATA_TCP, PARAM_NUMS_2);
    communication.sendParam(&sock, sizeof(sock));
    return communication.sendParam(peek, LAST_PARAM);
}

bool SocketDrv::getDataBuf(uint8_t sock, uint16_t _dataLen)
{
	bool ret;
    communication.sendCmdPkt(GET_DATABUF_TCP, PARAM_NUMS_2);
	communication.sendParam(&sock, sizeof(sock));
	ret = communication.sendParam(_dataLen, LAST_PARAM);
		
	return ret;
}

bool SocketDrv::sendUdpData(uint8_t sock)
{
    communication.sendCmdPkt(SEND_DATA_UDP, PARAM_NUMS_1);
    return communication.sendParam(&sock, sizeof(sock), LAST_PARAM);
}

bool SocketDrv::sendData(uint8_t sock, const uint8_t *data, uint16_t len)
{	
	communication.sendDataPkt(SEND_DATA_TCP);
	communication.appendByte(sock);
	return communication.appendBuffer((uint8_t*)data, len);
}