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

#ifndef WIFI_LINK_H
#define WIFI_LINK_H

#include "Arduino.h"
#include "config.h"
#include <WiFi.h>
#include <inttypes.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include "spi/spi_def.h"

#define CONNECTION_TIMEOUT  10000

#define WIFI_SPI_ACK        1
#define WIFI_SPI_ERR        0xFF

//#define TIMEOUT_CHAR      1000

#define MAX_SOCK_NUMBER     4 /**< Maxmium number of socket  */
#define NO_SOCKET_AVAIL     255

#define MAX_NETWORK_LIST    10

#define MTU_SIZE            1401  // MTU size = 1400 + sizeof(sock)

#define PROT_TCP 0
#define PROT_UDP 1


typedef struct __attribute__((__packed__))
{
  char strSSID[33];
  char strPWD[65];
  uint8_t aging;
} tsNetParam;

typedef struct __attribute__((__packed__))
{
  tsNetParam netEntry[MAX_NETWORK_LIST];
  uint8_t lastConnected;
} tsNetArray;

#define MAX_PARAMS MAX_PARAM_NUMS-1


typedef struct __attribute__((__packed__))
{
  uint8_t len;
  uint8_t* paramPtr;
} tsParameter;

typedef struct {
    int8_t nScannedNets;
    uint8_t currNetIdx;
} tsNetworkParams;

typedef struct{
  bool inUse;
  bool conn;
  uint8_t protocol;
} tsSockStatus;

typedef void (*ext_callback)(uint32_t);

/*
*
*/
class WiFi2ControlClass {
  private:
  tsNetworkParams _nets;
  IPAddress* _handyIp;
  IPAddress* _handyGateway;
  IPAddress* _handySubnet;
  tsNewCmd _tempPkt;
  uint16_t _sockNumber;

  WiFiServer* _mapWiFiServers[MAX_SOCK_NUMBER];
  WiFiClient _mapWiFiClients[MAX_SOCK_NUMBER];
  WiFiUDP _mapWiFiUDP[MAX_SOCK_NUMBER];
  uint16_t _port[MAX_SOCK_NUMBER];
  uint16_t _oldDataClient[MAX_SOCK_NUMBER] = {0};
  tsSockStatus _sockInUse[MAX_SOCK_NUMBER] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
  uint8_t _dataBuf[MTU_SIZE];
  uint16_t _dataBufSz = 0;
  
  // WiFi Base
 
 /*
  * process SET_IP_CONFIG command.
  */
  void _config();
  
 /*
  * process CONNECT_OPEN_AP, CONNECT_SECURED_AP and AUTOCONNECT_TO_STA commands.
  *
  * param option: uint8_t used to distinguish what is the command to be processed
  * return: false, to signal that no reply has been prepared
  */
  bool _connect(uint8_t option);
  
 /*
  * check if a given ssid is in the memorized network list.
  *
  * param targetSSID: the SSID to be check
  * return: the index of the SSID in the list if it was found, -1 otherwise
  */
  int8_t _checkSSIDInList(char* targetSSID);
  
 /*
  * clear the memorized network list.
  */
  void _clearWiFiSSIDList();
    
 /*
  * process DISCONNECT command.
  *
  * return: false, to signal that no reply has been prepared
  */
  bool _disconnect();

 /*
  * retrieve the memorized network list from EEPROM memory.
  */
  void _fetchWiFiSSIDList();
  
 /*
  * process GET_CURR_BSSID command.
  */
  void _getBSSID();
  
 /*
  * process GET_CURR_SSID command.
  */
  void _getCurrentSSID();
  
 /*
  * process GET_CURR_ENCT and GET_IDX_ENCT commands.
  *
  * param current: if 0 process GET_IDX_ENCT command, if 1 process GET_CURR_ENCT command 
  */  
  void _getEncryption(uint8_t current);
  
 /*
  * process GET_FW_VERSION command.
  */
  void _getFwVersion();
  
 /*
  * process GET_HOST_BY_NAME command.
  */
  void _getHostByName();
  
 /*
  * process GET_HOSTNAME command.
  */
  void _getHostname();
  
 /*
  * process GET_MACADDR command.
  */
  void _getMacAddress();
  
 /*
  * process GET_IPADDR command.
  */ 
  void _getNetworkData();
  
 /*
  * process GET_CURR_RSSI and GET_IDX_RSSI commands.
  *
  * param current: if 0 process GET_IDX_RSSI command, if 1 process GET_CURR_RSSI
  */
  void _getRSSI(uint8_t current);
  
 /*
  * process GET_CONN_STATUS command.
  */
  void _getStatus();
  
 /*
  * update aging in the memorized network list in order to be able to automatically
  * connect to the last connected network.
  *
  * param connectedIndex: index of the network in the list from which update aging,
  *                       0xFF if the network is a new entry.
  * return: the index of the oldest network in the list
  */ 
  uint8_t _networkListEvalAging(uint8_t connectedIndex);

 /*
  * Notify the control chip about a change in the connection status.
  *
  * param stat: one of the value defined in wl_status_t
  */
  void _notifySTAConnStatus(uint8_t stat);
      
 /*
  * process WiFi base commands by calling the right processing function for each command.
  *
  * param cmd: the command to be processed out
  * return: true if a reply packet has been prepared, false otherwise
  */
  bool _processWiFiCmd(uint8_t cmd);
  
 /*
  * process WiFi server related commands by calling the right processing function for each command.
  *
  * param cmd: the command to be processed out
  * return: true if a reply packet has been prepared, false otherwise
  */
  bool _processServerCmd(uint8_t cmd);
  
 /*
  * process WiFi client related commands by calling the right processing function for each command.
  *
  * param cmd: the command to be processed out
  * return: true if a reply packet has been prepared, false otherwise
  */
  bool _processClientCmd(uint8_t cmd);
  
 /*
  * process DATA_PKT commands by calling the right processing function for each command.
  *
  * param cmd: the command to be processed out
  * return: true if a reply packet has been prepared, false otherwise
  */
  bool _processDataCmd(uint8_t cmd);
    
 /*
  * process SCAN_NETWORK_RESULT command by supplying information about one of the scanned networks.
  *
  * param which: the index of the network from which retrieving information
  */
  void _scanNetwork(uint8_t which);

 /*
  * prepare and send an asynchronous command packet (not a reply one) related to a WiFi monitored event.
  *
  * param cmd: index of the command to be sent
  * param sock: index of the socket on which the event occurred
  * param result: the real data of the command
  * param len: length of result parameter
  */
  void _sendEventPacket(uint8_t cmd, uint8_t sock, uint8_t* result, uint8_t len = 1);

 /*
  * process SET_DNS_CONFIG command.
  */
  void _setDNS();

 /*
  * process START_SCAN_NETWORKS command and prepare a replay packet with the number of scanned networks.
  */
  void _startScanNetwork();
  
 /*
  * save the current memorized network list in eeprom memory.
  */ 
  void _storeWiFiSSIDList();

 /*
  * check for new received commands by calling _process function.
  */ 
  void _pollCommands();

 /*
  * check for events in active sockets and eventually call _sendEventPacket function to signal them.
  */
  void _pollFromWiFi();

  // TCP
  
 /*
  * process GET_DATA_TCP command by reading data on the requested socket.
  */ 
  void _getData();
  
 /*
  * process SEND_DATA_TCP command by sending data on the requested socket (TCP or UDP).
  */   
  bool _sendData();

  
  // WiFi Server
  
 /*
  * process START_SERVER_TCP command by starting the requested server (TCP or UDP) on the requested
  * _port and socket.
  */ 
  void _startServer();

  
  // WiFi Client
  
 /*
  * process START_CLIENT_TCP command by starting a connection (TCP or UDP) on a remote server.
  */  
  void _startClient();

 /*
  * process STOP_CLIENT_TCP command by closing a previously opened connection (TCP or UDP).
  */
  void _stopClient();

 /*
  * process GET_CLIENT_STATE_TCP command.
  */
  void _clientStatus();


  // WiFI UDP Client

 /*
  * process GET_REMOTE_DATA command by returning remote IP address and _port of an UDP packet.
  */
  void _remoteData();

 /*
  * process GET_DATABUF_TCP command by reading data on the requested socket.
  */
  void _getDataBuf();

 /*
  * process SEND_DATA_UDP command by calling endPacket on the requested UDP socket.
  */
  void _sendUdpData();
    
 /*
  * functions to initialize and manage the internal buffer
  */
  uint32_t _sockBuffInit(void);
  uint32_t _sockBuffAdd(uint8_t* data, uint32_t sz);
  
 /*
  * notify that we are ready to receive commands.
  */
  void _notifyReady();


   /*
  * Check the received command and forward the command to the right processing function.
  *
  * return: true if a reply packet has been prepared, false otherwise
  */
  friend bool process(void);


public:
  tsNetParam tempNetworkParam;
  tsNetArray networkArray;
  uint32_t connectionTimeout = 0;
  wl_status_t connectionStatus = WL_IDLE_STATUS;
  bool connectionRequested = false;
  int8_t connectedIndex_temp = -1;
  bool startAutoconnect = false;

  bool notify = false;
  bool UI_alert = false;
  
 /**
  * class constructor.
  */  
  WiFi2ControlClass();


  // Logic Functions
  
 /**
  * try to connect to the first available network in the networks memorized list.
  * 
  * @return true if one of the memorized networks is available, false otherwise
  */
  bool autoconnect();
  
 /**
  * perform the main initializations needed to the class.
  */ 
  void begin();

 /**
  * function needed to check for commands or events.
  */
  void poll();

 /**
  * initialize WiFi as access point or access point plus station mode.
  *
  * @param mode: The WiFi mode. Can be WIFI_STA, WIFI_AP or WIFI_AP_STA.
  */
  void setWiFiConfig(WiFiMode_t mode);

 /**
  * initialize WiFi as access point or access point plus station mode.
  *
  * @param mode: The WiFi mode. Can be WIFI_STA, WIFI_AP or WIFI_AP_STA.
  * @param apName: The name that the board will show as access point in WIFI_AP_STA and WIFI_AP mode.
  */
  void setWiFiConfig(WiFiMode_t mode, const char *apName);


 /**
  * initialize WiFi as access point or access point plus station mode.
  *
  * @param mode: The WiFi mode. Can be WIFI_STA, WIFI_AP or WIFI_AP_STA.
  * @param apName: The name that the board will show as access point in WIFI_AP_STA and WIFI_AP mode.
  * @param apPwd: The password that will be required to the ones that will try to connect to our access point.
  */
  void setWiFiConfig(WiFiMode_t mode, const char *apName, const char *apPwd);

};

extern WiFi2ControlClass wifi2control;

#endif //WIFI_LINK_H