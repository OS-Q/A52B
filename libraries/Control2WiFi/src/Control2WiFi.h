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

#ifndef WiFi_h
#define WiFi_h

#if defined BRIKI_MBC_WB_ESP
#error "Sorry, Control2WiFi library doesn't work with esp32 processor"
#endif

#define WIFI_FIRMWARE_REQUIRED "1.0.0"

#include <inttypes.h>

extern "C" {
	#include "utility/wl_definitions.h"
	#include "utility/wl_types.h"
}

#include "IPAddress.h"
#include "WClient.h"
#include "WServer.h"
#include "WUdp.h"

#define GENERAL_TIMEOUT		10000
#define MAX_HOSTNAME_LEN	32

/*  -----------------------------------------------------------------
* Structure to memorize network names and relative string len.
*/
typedef struct{
	int32_t rssi;
	uint8_t enc;
	uint8_t SSID_len;
	char SSID_name[33];
}networkParams_s;

typedef enum
{
	UNCONNECTED = -1,
	AP_MODE = 0,
	STA_MODE,
	AP_STA_MODE,
} teConnectionMode;

class Control2WiFi
{
	private:
	static uint8_t _hostname[MAX_HOSTNAME_LEN];
	
	public:
	
	static uint8_t state[MAX_SOCK_NUM];
	static uint16_t server_port[MAX_SOCK_NUM];
	static uint16_t client_data[MAX_SOCK_NUM];

	static bool gotResponse;
	static uint8_t responseType;
	static uint32_t dataLen;
	static uint32_t totalLen;
	static uint8_t* data;
	static wl_status_t connectionStatus;
	static bool notify;
		
	/**
	* class constructor.
	*/
	Control2WiFi();
		
	/**
	* handles the WiFi driver events to see if communication layer detected some event for us.
	*/	
	static void handleEvents(void);
	
	/**
	* Initializes the wifi class registering the relative callback in the communication layer.
	*/	
	static void init();
	
	/**
	* Get length of an available data packet if data are present
	*
	* @return length of data if present. 0 otherwise.
	*/
	static uint16_t getAvailableData();
	
	/**
	* Call low level driver function to delete the memorized networks list.
	*/	
	static void cancelNetworkListMem();
	
	/**
	* Get the first socket available.
	*
	* @return the number of the first available socket.
	*/
	static uint8_t getSocket();

	/**
	* Get firmware version.
	*
	* @return a char array representing firmware version.
	*/
	static char* firmwareVersion();

	/**
	* Check the firmware version required. Calls firmwareVersion and compare the received
	* response with the firmware version passed as parameter.
	*
	* @param fw_required: The required firmware version
	* @return true if the current firmware version is greater than or equal to the required one,
	*         false otherwise.
	*/
	bool checkFirmwareVersion(String fw_required);

	/** 
	* Start Wifi connection by trying to connect to a network in the memorized networks list.
	*
	* @return the connection status.
	*/
	wl_status_t begin();
	
	/**
	* Start Wifi connection for OPEN networks
	*
	* @param ssid: Pointer to the SSID string.
	*
	* @return the connection status.
	*/
	wl_status_t begin(char* ssid);

	/**
	* Start Wifi connection with WEP encryption.
	* Configure a key into the device. The key type (WEP-40, WEP-104)
	* is determined by the size of the key (5 bytes for WEP-40, 13 bytes for WEP-104).
	*
	* @param ssid: Pointer to the SSID string.
	* @param key_idx: The key index to set. Valid values are 0-3.
	* @param key: Key input buffer.
	*
	* @return the connection status.
	*/
	wl_status_t begin(char* ssid, uint8_t key_idx, const char* key);

	/**
	* Start Wifi connection with passphrase.
	* The most secure supported mode will be automatically selected.
	*
	* @param ssid: Pointer to the SSID string.
	* @param passphrase: Passphrase. Valid characters in a passphrase
	*        must be between ASCII 32-126 (decimal).
	*
	* @return the connection status.
	*/
	wl_status_t begin(char* ssid, const char *passphrase);

	/**
	* Change Ip configuration settings disabling the dhcp client.
	*
	* @param local_ip: Static ip configuration.
	*
	* @return true if the request succeeded, false otherwise.
	*/
	bool config(IPAddress local_ip);

	/**
	* Change Ip configuration settings disabling the dhcp client.
	*
	* @param local_ip: Static ip configuration.
	* @param dns_server: IP configuration for DNS server 1.
	*
	* @return true if the request succeeded, false otherwise.
	*/
	bool config(IPAddress local_ip, IPAddress dns_server);

	/**
	* Change Ip configuration settings disabling the dhcp client.
	*
	* @param local_ip: Static ip configuration.
	* @param dns_server: IP configuration for DNS server 1.
	* @param gateway : Static gateway configuration.
	*
	* @return true if the request succeeded, false otherwise.
	*/
	bool config(IPAddress local_ip, IPAddress dns_server, IPAddress gateway);

	/**
	* Change Ip configuration settings disabling the dhcp client.
	*
	* @param local_ip: Static ip configuration.
	* @param dns_server: IP configuration for DNS server 1.
	* @param gateway: Static gateway configuration.
	* @param subnet: Static Subnet mask.
	*
	* @return true if the request succeeded, false otherwise.
	*/
	bool config(IPAddress local_ip, IPAddress dns_server, IPAddress gateway, IPAddress subnet);

	/**
	* Change DNS Ip configuration.
	*
	* @param dns_server1: ip configuration for DNS server 1.
	*
	* @return true if the request succeeded, false otherwise.
	*/
	bool setDNS(IPAddress dns_server1);

	/**
	* Change DNS Ip configuration.
	*
	* @param dns_server1: Ip configuration for DNS server 1.
	* @param dns_server2: Ip configuration for DNS server 2.
	*
	* @return true if the request succeeded, false otherwise.
	*/
	bool setDNS(IPAddress dns_server1, IPAddress dns_server2);

	/**
	* Disconnect from the network.
	*
	* @return the connection status or -1 in event of error.
	*/
	int disconnect(void);

	/**
	* Get the interface MAC address.
	*
	* @param mac: pointer to uint8_t array with length WL_MAC_ADDR_LENGTH where mac address will be stored.
	*/
	void macAddress(uint8_t* mac);

	/**
	* Get the interface IP address.
	*
	* @return IP address value.
	*/
	IPAddress localIP();

	/**
	* Get the interface subnet mask address.
	*
	* @return subnet mask address value.
	*/
	IPAddress subnetMask();

	/**
	* Get the gateway ip address.
	*
	* @return gateway ip address value.
	*/
	IPAddress gatewayIP();

	/**
	* Get the current SSID associated with the network we are connected to.
	*
	* @return a char array representing the ssid string
	*/
	char* SSID();

	/**
	* Return the current BSSID associated with the network we are connected to.
	* It is the MAC address of the Access Point.
	*
	* @param bssid: Pointer to uint8_t array with length WL_MAC_ADDR_LENGTH where the bssid will be placed.
	*
	* @return true if operation succeeded, false otherwise.
	*/
	bool BSSID(uint8_t* bssid);

	/**
	* Return the current RSSI (Received Signal Strength in dBm)
	* associated with the network we are connect to.
	*
	* @return signed value representing the RSSI.
	*/
	int32_t RSSI();

	/**
	* Return the Encryption Type associated with the network we are connected to.
	*
	* @return one value of wl_enc_type enum.
	*/
	uint8_t	encryptionType();

	/**
	* Start scan WiFi networks available.
	*
	* @return number of discovered networks.
	*/
	int8_t scanNetworks();
	
	/**
	* Get the hostname associated to the board.
	*
	* @return a char pointer to the hostname.
	*/
	char* getHostname();
	
	/**
	* Set a new hostname for the board.
	*
	* @param name: The hostname to be set.
	*
	* @return true if the operation succeeded, false otherwise.
	*/
	bool setHostname(char* name);
	
	/**
	* Get information about one of the previously scanned networks.
	*
	* @param netNum: Index of the desired network.
	* @param ssid: Char pointer where the ssid of the desired network will be stored.
	* @param rssi: Int32 where the rssi will be stored.
	* @param enc: Unsigned int where a code representing an encryption type will be stored.
	*
	* @return 1 if operation succeeded, 0 otherwise.
	*/
	uint8_t getScannedNetwork(uint8_t netNum, char *ssid, int32_t& rssi, uint8_t& enc);

	/**
	* Get the SSID of a network discovered during the network scan.
	*
	* @param networkItem: Specify from which network item want to get the information.
	*
	* @return ssid string of the specified item on the networks scanned list.
	*/
	char*	SSID(uint8_t networkItem);

	/**
	* Get the encryption type of the networks discovered during the scanNetworks.
	*
	* @param networkItem: Specify from which network item want to get the information.
	*
	* @return encryption type (enum wl_enc_type) of the specified item on the networks scanned list.
	*/
	uint8_t	encryptionType(uint8_t networkItem);

	/**
	* Get the RSSI of the networks discovered during the scanNetworks.
	*
	* @param networkItem: Specify from which network item want to get the information.
	*
	* @return signed value of RSSI of the specified item on the networks scanned list.
	*/
	int32_t RSSI(uint8_t networkItem);

	/**
	* Get the connection status.
	*
	* @return one of the value defined in wl_status_t
	*/
	wl_status_t status();
	
	/**
	* Resolve the given hostname to an IP address.
	*
	* @param aHostname: Name to be resolved.
	* @param aResult: IPAddress structure to store the returned IP address.
	*
	* @result 1 if aIPAddrString was successfully converted to an IP address,
	*          else error code
	*/
	int hostByName(const char* aHostname, IPAddress& aResult);


	friend class WiFiClient;
	friend class WiFiServer;
	
};

extern Control2WiFi WiFi;

#endif
