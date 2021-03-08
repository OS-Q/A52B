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

#ifndef WiFi_Drv_h
#define WiFi_Drv_h

#include <inttypes.h>
#include "wl_definitions.h"
#include "IPAddress.h"

// Key index length
#define KEY_IDX_LEN     1
// firmware version string length
#define WL_FW_VER_LENGTH 6

typedef struct __attribute__((__packed__))
{
	uint8_t cmdType;
	uint8_t cmd;
	uint32_t totalLen;
	uint32_t receivedLen;
	bool endReceived;
} tsDataPacket;

typedef struct __attribute__((__packed__))
{
	uint8_t cmdType;
	uint8_t cmd;
	uint32_t totalLen;
	uint8_t sockNum;
	uint16_t* dataPtr;
} tsNewData;

typedef struct __attribute__((__packed__))
{
	uint8_t cmdType;
	uint8_t cmd;
	uint8_t totalLen;
	uint8_t nParam;
	uint8_t* dataPtr;
} tsNewCmd;

class WiFiDrv
{
private:
	// settings of requested network
	static char 	_networkSsid[WL_NETWORKS_LIST_MAXNUM][WL_SSID_MAX_LENGTH];

public:
	// settings of current selected network
	static char 	ssid[WL_SSID_MAX_LENGTH];

	/**
	 * Using communication layer, pack and send a command to get network data information.
	 *
	 * @return true if command packet has been sent, false otherwise.
	 */
    static bool getNetworkData();
	
	/**
	 * Using communication layer, pack and send a command to retrieve IP address from an hostname.
	 *
	 * @param aHostname: The hostname that will be resolved.
	 *
	 * @return true if command packet has been sent, false otherwise.
	 */
    static bool getHostByName(const char* aHostname);


	/**
	 * Using communication layer, pack and send a command to get data from a remote socket.
	 *
	 * @param sock: The socket from which retrieve data.
	 *
	 * @return true if command packet has been sent, false otherwise.
	 */
    static bool getRemoteData(uint8_t sock);
	
	/**
	 * Using communication layer, pack and send a command to get our current hostname.
	 *
	 * @return true if command packet has been sent, false otherwise.
	 */
	static bool getHostname();

	/**
	 * Using communication layer, pack and send a command to set a new hostname.
	 *
	 * @param name: A char array representing the new hostname.
	 *
	 * @return true if command packet has been sent, false otherwise.
	 */
	static bool setHostname(const char* name);

	/**
	 * Using communication layer, pack and send a command to start connection to a known network.
	 *
	 * @return true if command packet has been sent, false otherwise.
	 */	
	static bool wifiAutoConnect();

	/**
	 * Using communication layer, pack and send a command to delete the list of known networks.
	 *
	 * @return true if command packet has been sent, false otherwise.
	 */	
	static void wifiCancelNetList();
	
	
	/**
	 * Using communication layer, pack and send a command to start connection to an open network.
	 *
	 * @param ssid: The ssid of the desired network.
	 * @param ssid_len: Lenght of ssid string.
	 *
	 * @return true if command packet has been sent, false otherwise.
	 */	
    static bool wifiSetNetwork(char* ssid, uint8_t ssid_len);

	/**
	 * Using communication layer, pack and send a command to start connection to an encrypted network.
	 *
	 * @param ssid: The ssid of the desired network.
	 * @param ssid_len: Lenght of ssid string.
	 * @param passphrase: Passphrase. Valid characters in a passphrase
     *        must be between ASCII 32-126 (decimal).
	 * @param len: Lenght of passphrase string.
	 *
	 * @return true if command packet has been sent, false otherwise.
	 */	
    static bool wifiSetPassphrase(char* ssid, uint8_t ssid_len, const char *passphrase, const uint8_t len);

	/**
	 * Using communication layer, pack and send a command to start connection with WEP encryption.
	 *
	 * @param ssid: The ssid of the desired network.
	 * @param ssid_len: Lenght of ssid string.
	 * @param key_idx: The key index to set. Valid values are 0-3.
     * @param key: Key input buffer.
	 * @param len: Lenght of passphrase string.
	 *
	 * @return true if command packet has been sent, false otherwise.
	 */	
    static bool wifiSetKey(char* ssid, uint8_t ssid_len, uint8_t key_idx, const void *key, const uint8_t len);

	/**
	 * Using communication layer, pack and send a command to set dhcp client config.
	 *
	 * @param validParams: set the number of parameters that we want to change
     * 					 i.e. validParams = 1 means that we'll change only ip address
     * 					 	  validParams = 3 means that we'll change ip address, gateway and netmask
	 * @param local_ip: Static ip configuration.
	 * @param gateway: Static gateway configuration.
	 * @param subnet: Static subnet mask configuration.
	 *
	 * @return true if command packet has been sent, false otherwise.
	 */	
    static bool config(uint8_t validParams, uint32_t local_ip, uint32_t gateway, uint32_t subnet);

	/**
	 * Using communication layer, pack and send a command to set DNS ip configuration.
	 *
	 * @param validParams: set the number of parameters that we want to change
     * 					 i.e. validParams = 1 means that we'll change only dns_server1
     * 					 	  validParams = 2 means that we'll change dns_server1 and dns_server2
	 * @param dns_server1: Static DNS server1 configuration.
	 * @param dns_server2: Static DNS server2 configuration.
	 *
	 * @return true if command packet has been sent, false otherwise.
	 */	
    static bool setDNS(uint8_t validParams, uint32_t dns_server1, uint32_t dns_server2);

	/**
	 * Using communication layer, pack and send a command to start disconnecting from a network
	 * we are currently connected to.
	 *
	 * @return true if command packet has been sent, false otherwise.
	 */	
    static bool disconnect();

	/**
	 * Using communication layer, pack and send a command to ask the current connection status.
	 *
	 * @return true if command packet has been sent, false otherwise.
	 */	
    static bool getConnectionStatus();

	/**
	 * Using communication layer, pack and send a command to ask for MAC address.
	 *
	 * @return true if command packet has been sent, false otherwise.
	 */	
    static bool getMacAddress();

	/**
	 * Using communication layer, pack and send a command to ask for the SSID of the network
	 * we are currently connected to.
	 *
	 * @return true if command packet has been sent, false otherwise.
	 */	
    static bool getCurrentSSID();

	/**
	 * Using communication layer, pack and send a command to ask for the BSSID of the network
	 * we are currently connected to.
	 *
	 * @return true if command packet has been sent, false otherwise.
	 */	
    static bool getCurrentBSSID();

	/**
	 * Using communication layer, pack and send a command to ask for the RSSI of the network
	 * we are currently connected to.
	 *
	 * @return true if command packet has been sent, false otherwise.
	 */	
    static bool getCurrentRSSI();

	/**
	 * Using communication layer, pack and send a command to ask for the enctyption type of the network
	 * we are currently connected to.
	 *
	 * @return true if command packet has been sent, false otherwise.
	 */	
    static bool getCurrentEncryptionType();

	/**
	 * Using communication layer, pack and send a command to start the WiFi scan of the nearby networks.
	 *
	 * @return true if command packet has been sent, false otherwise.
	 */	
    static bool startScanNetworks();

	/**
	 * Using communication layer, pack and send a command to ask for the number of scanned networks
	 *
	 * @return true if command packet has been sent, false otherwise.
	 */	
	static bool getScanNetworks(uint8_t which);

	/**
	 * Using communication layer, pack and send a command to ask for the SSID of a network
	 * discovered during scan.
	 *
	 * @param networkItem: specify the number of the network from which to get the information
	 *
	 * @return true if command packet has been sent, false otherwise.
	 */	
    static char* getSSIDNetoworks(uint8_t networkItem);

	/**
	 * Using communication layer, pack and send a command to ask for the RSSI of a network
	 * discovered during scan.
	 *
	 * @param networkItem: specify the number of the network from which to get the information
	 *
	 * @return true if command packet has been sent, false otherwise.
	 */	
    static bool getRSSINetoworks(uint8_t networkItem);

	/**
	 * Using communication layer, pack and send a command to ask for the encryption type of a network
	 * discovered during scan.
	 *
	 * @param networkItem: specify the number of the network from which to get the information
	 *
	 * @return true if command packet has been sent, false otherwise.
	 */	
    static bool getEncTypeNetowrks(uint8_t networkItem);

	/**
	 * Using communication layer, pack and send a command to ask for the firmware version running
	 * on the board.
	 *
	 * @return true if command packet has been sent, false otherwise.
	 */	
    static bool getFwVersion();
};

#endif