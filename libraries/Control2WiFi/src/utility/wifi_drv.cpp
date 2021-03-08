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

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "Arduino.h"
#include "communication.h"

#include "wifi_drv.h"
#include "CommCmd.h"

#include "Control2WiFi.h"

#define _DEBUG_

extern "C" {
	#include "utility/wl_types.h"
}

// Array of data to cache the information related to the networks discovered
char WiFiDrv::_networkSsid[][WL_SSID_MAX_LENGTH] = {{"1"},{"2"},{"3"},{"4"},{"5"}};

// Cached values of retrieved data
char WiFiDrv::ssid[WL_SSID_MAX_LENGTH] = {0};

// Private Methods
// ----------------------------------------------------------------- 
bool WiFiDrv::getNetworkData()
{
	uint8_t _dummy = DUMMY_DATA;

	communication.sendCmdPkt(GET_IPADDR, PARAM_NUMS_1);
	return communication.sendParam(&_dummy, sizeof(_dummy), LAST_PARAM);
}

bool WiFiDrv::getRemoteData(uint8_t sock)
{
	communication.sendCmdPkt(GET_REMOTE_DATA, PARAM_NUMS_1);
	return communication.sendParam(&sock, sizeof(sock), LAST_PARAM);
}

// Public Methods
// -----------------------------------------------------------------
bool WiFiDrv::wifiAutoConnect()
{
	return communication.sendCmdPkt(AUTOCONNECT_TO_STA, PARAM_NUMS_0);
}

void WiFiDrv::wifiCancelNetList()
{
	communication.sendCmdPkt(CANCEL_NETWORK_LIST, PARAM_NUMS_0);
}

// -----------------------------------------------------------------
bool WiFiDrv::wifiSetNetwork(char* ssid, uint8_t ssid_len)
{
	communication.sendCmdPkt(CONNECT_OPEN_AP, PARAM_NUMS_1);
	return communication.sendParam((uint8_t*)ssid, ssid_len, LAST_PARAM);
}

// ----------------------------------------------------------------- ok
bool WiFiDrv::wifiSetPassphrase(char* ssid, uint8_t ssid_len, const char *passphrase, const uint8_t len)
{
	communication.sendCmdPkt(CONNECT_SECURED_AP, PARAM_NUMS_2);
	communication.sendParam((uint8_t*)ssid, ssid_len, NO_LAST_PARAM);
	return communication.sendParam((uint8_t*)passphrase, len, LAST_PARAM);
}

// -----------------------------------------------------------------
bool WiFiDrv::wifiSetKey(char* ssid, uint8_t ssid_len, uint8_t key_idx, const void *key, const uint8_t len)
{
	communication.sendCmdPkt(SET_KEY, PARAM_NUMS_3);
	communication.sendParam((uint8_t*)ssid, ssid_len, NO_LAST_PARAM);
	communication.sendParam(&key_idx, KEY_IDX_LEN, NO_LAST_PARAM);
	return communication.sendParam((uint8_t*)key, len, LAST_PARAM);
}

// -----------------------------------------------------------------
bool WiFiDrv::config(uint8_t validParams, uint32_t local_ip, uint32_t gateway, uint32_t subnet)
{
	communication.sendCmdPkt(SET_IP_CONFIG, PARAM_NUMS_4);
	communication.sendParam((uint8_t*)&validParams, 1, NO_LAST_PARAM);
	communication.sendParam((uint8_t*)&local_ip, 4, NO_LAST_PARAM);
	communication.sendParam((uint8_t*)&gateway, 4, NO_LAST_PARAM);
	return communication.sendParam((uint8_t*)&subnet, 4, LAST_PARAM);
}

// -----------------------------------------------------------------
bool WiFiDrv::setDNS(uint8_t validParams, uint32_t dns_server1, uint32_t dns_server2)
{
	communication.sendCmdPkt(SET_DNS_CONFIG, PARAM_NUMS_3);
	communication.sendParam((uint8_t*)&validParams, 1, NO_LAST_PARAM);
	communication.sendParam((uint8_t*)&dns_server1, 4, NO_LAST_PARAM);
	return communication.sendParam((uint8_t*)&dns_server2, 4, LAST_PARAM);
}

// -----------------------------------------------------------------
bool WiFiDrv::disconnect()
{
	uint8_t _dummy = DUMMY_DATA;
	
	communication.sendCmdPkt(DISCONNECT, PARAM_NUMS_1);
	return communication.sendParam(&_dummy, 1, LAST_PARAM);
}

// ----------------------------------------------------------------- ok
bool WiFiDrv::getConnectionStatus()
{
	return communication.sendCmdPkt(GET_CONN_STATUS, PARAM_NUMS_0);
}

// -----------------------------------------------------------------
bool WiFiDrv::getHostname()
{
	return communication.sendCmdPkt(GET_HOSTNAME, PARAM_NUMS_0);
}

// -----------------------------------------------------------------
bool WiFiDrv::setHostname(const char* name)
{
	communication.sendCmdPkt(SET_HOSTNAME, PARAM_NUMS_1);
	return communication.sendParam((uint8_t*)name, strlen(name), LAST_PARAM);
}

// ----------------------------------------------------------------- ok
bool WiFiDrv::getMacAddress()
{
	uint8_t _dummy = DUMMY_DATA;
	
	communication.sendCmdPkt(GET_MACADDR, PARAM_NUMS_1);
	return communication.sendParam(&_dummy, 1, LAST_PARAM);
}

// ----------------------------------------------------------------- ok
bool WiFiDrv::getCurrentSSID()
{
	uint8_t _dummy = DUMMY_DATA;
	
	communication.sendCmdPkt(GET_CURR_SSID, PARAM_NUMS_1);
	return communication.sendParam(&_dummy, 1, LAST_PARAM);
}

// ----------------------------------------------------------------- ok
bool WiFiDrv::getCurrentBSSID()
{
	uint8_t _dummy = DUMMY_DATA;

	communication.sendCmdPkt(GET_CURR_BSSID, PARAM_NUMS_1);
	return communication.sendParam(&_dummy, 1, LAST_PARAM);
}

// ----------------------------------------------------------------- ok
bool WiFiDrv::getCurrentRSSI()
{
	uint8_t _dummy = DUMMY_DATA;
	
	communication.sendCmdPkt(GET_CURR_RSSI, PARAM_NUMS_1);
	return communication.sendParam(&_dummy, 1, LAST_PARAM);
}

// ----------------------------------------------------------------- ok
bool WiFiDrv::getCurrentEncryptionType()
{
	uint8_t _dummy = DUMMY_DATA;
	
	communication.sendCmdPkt(GET_CURR_ENCT, PARAM_NUMS_1);
	return communication.sendParam(&_dummy, 1, LAST_PARAM);
}

// -----------------------------------------------------------------
bool WiFiDrv::startScanNetworks()
{
	return communication.sendCmdPkt(START_SCAN_NETWORKS, PARAM_NUMS_0);
}

// -----------------------------------------------------------------
bool WiFiDrv::getScanNetworks(uint8_t which)
{
	communication.sendCmdPkt(SCAN_NETWORKS_RESULT, PARAM_NUMS_1);
	return communication.sendParam(&which, 1, LAST_PARAM);
}

// -----------------------------------------------------------------
char* WiFiDrv::getSSIDNetoworks(uint8_t networkItem)
{
	if (networkItem >= WL_NETWORKS_LIST_MAXNUM)
	return NULL;

	return _networkSsid[networkItem];
}

// -----------------------------------------------------------------
bool WiFiDrv::getEncTypeNetowrks(uint8_t networkItem)
{
	if (networkItem >= WL_NETWORKS_LIST_MAXNUM)
	return false;
	
	communication.sendCmdPkt(GET_IDX_ENCT, PARAM_NUMS_1);
	return communication.sendParam(&networkItem, 1, LAST_PARAM);
}

// -----------------------------------------------------------------
bool WiFiDrv::getRSSINetoworks(uint8_t networkItem)
{
	if (networkItem >= WL_NETWORKS_LIST_MAXNUM)
	return false;
	
	communication.sendCmdPkt(GET_IDX_RSSI, PARAM_NUMS_1);
	return communication.sendParam(&networkItem, 1, LAST_PARAM);
}

// -----------------------------------------------------------------
bool WiFiDrv::getHostByName(const char* aHostname)
{
	communication.sendCmdPkt(GET_HOST_BY_NAME, PARAM_NUMS_1);
	return communication.sendParam((uint8_t*)aHostname, strlen(aHostname), LAST_PARAM);
}

// -----------------------------------------------------------------
bool WiFiDrv::getFwVersion()
{
	return communication.sendCmdPkt(GET_FW_VERSION, PARAM_NUMS_0);
}
