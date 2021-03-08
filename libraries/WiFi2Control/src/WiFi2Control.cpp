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

#include "WiFi2Control.h"
#include "CommCmd.h"
#include "communication.h"
#include "events.h"

void onEvent(WiFiEvent_t event){
  /*
   *  event management system. Look at tools\sdk\include\esp32\esp_event.h
   *  for a complete list of events
   */
   #ifdef DEBUG_SERIALE
  Serial.println("Received a new event: " + String(event));
  #endif
  switch (event){
	case SYSTEM_EVENT_STA_DISCONNECTED:
      #ifdef DEBUG_SERIALE
      Serial.print("Station disconnected: ");
      #endif
      wifi2control.connectionStatus = WL_DISCONNECTED;
      wifi2control.notify = true;
      if(wifi2control.connectionRequested){ // try to connect to a stored network if previous connection requested, by specifying ssid and pwd, failed
        wifi2control.connectionRequested = false;
        wifi2control.startAutoconnect = true;
      }
	  if(WiFi_disconnected)
		  WiFi_disconnected();
	  
    break;

    case SYSTEM_EVENT_STA_CONNECTED:
      #ifdef DEBUG_SERIALE
        Serial.print("Station connected: ");
      #endif

      wifi2control.connectionStatus = WL_CONNECTED;
      wifi2control.notify = true;
      wifi2control.connectionRequested = false;
	  
	  if(WiFi_connected)
		  WiFi_connected();
    break;

	case SYSTEM_EVENT_WIFI_READY:
		if(WiFi_ready)
			WiFi_ready();
	break;
	
	case SYSTEM_EVENT_SCAN_DONE:
		if(WiFi_scanDone)
			WiFi_scanDone();
	break;

	case SYSTEM_EVENT_STA_START:
		if(WiFi_startedSTA)
			WiFi_startedSTA();
	break;
	
	case SYSTEM_EVENT_STA_STOP:
		if(WiFi_stoppedSTA)
			WiFi_stoppedSTA();
	break;
	
	case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
		if(WiFi_authModeChanged)
			WiFi_authModeChanged();
	break;
	
	case SYSTEM_EVENT_STA_GOT_IP:
		if(WiFi_gotIP)
			WiFi_gotIP();
	break;
	
	case SYSTEM_EVENT_STA_LOST_IP:
		if(WiFi_lostIP)
			WiFi_lostIP();
	break;
	
	case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
		if(WiFi_wpsEnrolleeSuccess)
			WiFi_wpsEnrolleeSuccess();
	break;
	
	case SYSTEM_EVENT_STA_WPS_ER_FAILED:
		if(WiFi_wpsEnrolleeFailed)
			WiFi_wpsEnrolleeFailed();
	break;
	
	case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
		if(WiFi_wpsEnrolleeTimeout)
			WiFi_wpsEnrolleeTimeout();
	break;
	
	case SYSTEM_EVENT_STA_WPS_ER_PIN:
		if(WiFi_wpsEnrolleePinCode)
			WiFi_wpsEnrolleePinCode();
	break;
	
	case SYSTEM_EVENT_AP_START:
		if(WiFi_startedAP)
			WiFi_startedAP();
	break;
	
	case SYSTEM_EVENT_AP_STOP:
		if(WiFi_stoppedAP)
			WiFi_stoppedAP();
	break;
	
	case SYSTEM_EVENT_AP_STACONNECTED:
		if(WiFi_STAconnectedToAP)
			WiFi_STAconnectedToAP();
	break;

	case SYSTEM_EVENT_AP_STADISCONNECTED:
		if(WiFi_STAdisconnectedFromAP)
			WiFi_STAdisconnectedFromAP();
	break;
	
	case SYSTEM_EVENT_AP_STAIPASSIGNED:
		if(WiFi_APassignedIP2STA)
			WiFi_APassignedIP2STA();
	break;
	
	case SYSTEM_EVENT_AP_PROBEREQRECVED:
		if(WiFi_APreceivedProbe)
			WiFi_APreceivedProbe();
	break;
	
	case SYSTEM_EVENT_GOT_IP6:
		if(WiFi_gotIPv6)
			WiFi_gotIPv6();
	break;
   }
}


bool process(void){
  bool rep = true;
  
  memcpy((uint8_t*)&wifi2control._tempPkt, communication.getRxBuffer(), sizeof(tsNewCmd));
  
 if(wifi2control._tempPkt.cmd < 0x30)
	rep = wifi2control._processWiFiCmd(wifi2control._tempPkt.cmd);
  else if(wifi2control._tempPkt.cmd < 0x32)
	rep = wifi2control._processServerCmd(wifi2control._tempPkt.cmd);
  else if(wifi2control._tempPkt.cmd < 0x40)
	rep = wifi2control._processClientCmd(wifi2control._tempPkt.cmd);
  else if(wifi2control._tempPkt.cmd < 0x50)
	rep = wifi2control._processDataCmd(wifi2control._tempPkt.cmd);
  
  return rep;
}

// ----------------------------------------------- WiFi2ControlClass -----------------------------------------------

// ----------------------------------------------------- PUBLIC

/*
*
*/
WiFi2ControlClass::WiFi2ControlClass()
{
}


/*
*
*/
void WiFi2ControlClass::begin()
{
  EEPROM.begin(1024);
	
  // register process function in order to be able to receive commands from control chip
  communication.registerHandle(process);
  //_clearWiFiSSIDList();

  // fetch WiFi network list from flash memory
  _fetchWiFiSSIDList();

/*
  #ifdef DEBUG_SERIALE
    for(uint8_t i=0; i<MAX_NETWORK_LIST; i++){
      Serial.print("\nSSID: ");
      Serial.write(networkArray.netEntry[i].strSSID, 32);
      Serial.print("\nPWD: ");
      Serial.write(networkArray.netEntry[i].strPWD, 64);
      Serial.print("\naging: ");
      Serial.println(networkArray.netEntry[i].aging);
    }
  #endif
*/
  _nets.nScannedNets = 0;
  _nets.currNetIdx = 0;
  //WiFiEvent_t event
  WiFi.onEvent(onEvent);
  WiFi.begin();
  setWiFiConfig(WIFI_AP_STA);
  
  // wait initialization phase
  while(!communication.companionReady());
}

/*
*
*/
void WiFi2ControlClass::poll()
{
	_pollCommands();
	_pollFromWiFi();
}

/*
*
*/
void WiFi2ControlClass::setWiFiConfig(WiFiMode_t mode)
{
  setWiFiConfig(mode, NULL, NULL);
}

void WiFi2ControlClass::setWiFiConfig(WiFiMode_t mode, const char *apName)
{
  setWiFiConfig(mode, apName, NULL);
}

void WiFi2ControlClass::setWiFiConfig(WiFiMode_t mode, const char *apName, const char *apPwd)
{
  char softApssid[33];
  
  //set mode
  WiFi.mode(mode);
  WiFi.disconnect();
  delay(100);

  //set default AP
  if(mode == WIFI_AP || mode == WIFI_AP_STA){
    WiFi.softAPConfig(default_IP, default_IP, IPAddress(255, 255, 255, 0));   //set default ip for AP mode
    
    if(apName == NULL){
      String mac = WiFi.macAddress();
      String apSSID = String(SSIDNAME) + "-" + String(mac[9])+String(mac[10])+String(mac[12])+String(mac[13])+String(mac[15])+String(mac[16]);
      apSSID.toCharArray(softApssid, apSSID.length()+1);

      WiFi.softAP(softApssid);
    }
    else{
      if(apPwd != NULL){
        WiFi.softAP(apName, apPwd);
      }
      else
        WiFi.softAP(apName);
    }
  }
}

void WiFi2ControlClass::_notifyReady()
{
  uint8_t response = 1;
  communication.getLock();
  communication.sendCmdPkt(COMPANION_READY, PARAM_NUMS_1);
  communication.sendParam(&response, PARAM_SIZE_1, LAST_PARAM);
  communication.releaseLock();
}

bool WiFi2ControlClass::autoconnect()
{
  uint8_t result;
  char ssidNet[33] = {0};
  int32_t rssi = -100;
  int32_t tempRssi = 0;
  uint32_t start = 0;
  bool found = false;

  start = millis() + CONNECTION_TIMEOUT;
  _nets.nScannedNets = -1;

  //scanNetworks command
  do {
    _nets.nScannedNets = WiFi.scanNetworks(false, false);
    delayMicroseconds(100);
  } while (_nets.nScannedNets < 0 && start > millis());

  if (_nets.nScannedNets <= 0){
    _nets.nScannedNets = 0;
    result = WL_NO_SSID_AVAIL;
    #ifdef DEBUG_SERIALE
      Serial.println("no network found");
    #endif
  }
  else {
    for(uint8_t i=0; i<_nets.nScannedNets; i++) {
      strcpy(ssidNet, WiFi.SSID(i).c_str());
      for(uint8_t j=0; j<MAX_NETWORK_LIST; j++){
        if(strcmp((char*)ssidNet, networkArray.netEntry[j].strSSID) == 0) {
          found = true;
          if(networkArray.lastConnected == j){  //last connected -> connect directly
            connectedIndex_temp = j;

            #ifdef DEBUG_SERIALE
              Serial.println("Connect to the last connected");
            #endif
            break;
          }
          tempRssi = WiFi.RSSI(i);
          if(rssi < tempRssi) {
            rssi = tempRssi;
            connectedIndex_temp = j;
          }
        }
      }
      if(found)
        break;
      memset(ssidNet, 0, 33);
    }
  }
  // a network match found
  if(connectedIndex_temp >= 0) {
    #ifdef DEBUG_SERIALE
      Serial.print("\nSSID: ");
      Serial.println((char *)networkArray.netEntry[connectedIndex_temp].strSSID);
      Serial.print("PWD: ");
      Serial.println((char *)networkArray.netEntry[connectedIndex_temp].strPWD);
    #endif
    
    if(WiFi.status() == WL_CONNECTED){
      //if(networkArray.netEntry[connectedIndex_temp].strSSID != Config.getParam("ssid").c_str() != ssidNet)
        WiFi.disconnect();
    }

    connectionTimeout = millis() + CONNECTION_TIMEOUT;
    connectionStatus = WL_IDLE_STATUS;

    result = WiFi.begin(networkArray.netEntry[connectedIndex_temp].strSSID, networkArray.netEntry[connectedIndex_temp].strPWD);
  }
  else{
    #ifdef DEBUG_SERIALE
      Serial.println("No network memorized or match found");
    #endif
  }

  return found;
}

// ----------------------------------------------------- PRIVATE

/*
*
*/
void WiFi2ControlClass::_pollCommands(){
  communication.handleCommEvents();
}

/*
*
*/
void WiFi2ControlClass::_pollFromWiFi(){
  // poll for events from high-level interface
  for (uint8_t i = 0; i < MAX_SOCK_NUMBER; i++){
    // check for server events
    if (!_sockInUse[i].inUse) { // wait for other requests on the same socket to be completed before accepting new ones
      if (_mapWiFiServers[i] != NULL) {
        _mapWiFiClients[i] = _mapWiFiServers[i]->available();
        uint8_t result = _mapWiFiClients[i].connected();
        if (result > 0) {
          #ifdef DEBUG_SERIALE
            //Serial.println("client connected to my server");
          #endif
          _sendEventPacket(GET_CLIENT_STATE_TCP, i, &result);
          _sockInUse[i].inUse = true;
          _sockInUse[i].conn = true; 
          _sockInUse[i].protocol = PROT_TCP;
          _oldDataClient[i] = 0;
        }
      }
    }

    // check for client events
    if(_sockInUse[i].conn == true && _sockInUse[i].protocol == PROT_TCP) {
      bool disconnected = false;
      if(uint16_t result = _mapWiFiClients[i].available()){
        // check if there are available data only if oldDataClint for that socket is less than the data available on the clinet, otherwise consider that the other micro is aware about the number of data available
        if (_oldDataClient[i] < result) {
          #ifdef DEBUG_SERIALE
            //Serial.println("new data");
          #endif
		  uint8_t pkt[] = {((uint8_t *)&result)[0], ((uint8_t *)&result)[1]};
          _sendEventPacket(AVAIL_DATA_TCP, i, pkt, 2);
          _oldDataClient[i] = result;
        }
      }
      else if(!_mapWiFiClients[i].connected())
          disconnected = true;
      if(disconnected){  // remote client was disconnected. close socket and send related event
        #ifdef DEBUG_SERIALE
          //Serial.println("socket closed");
        #endif
        uint8_t status = 0;
		
        _sendEventPacket(GET_CLIENT_STATE_TCP, i, &status);
        _sockInUse[i].conn = false;
        _sockInUse[i].inUse = false;
        _mapWiFiClients[i].stop();
      }
    }
    

    if(_sockInUse[i].protocol == PROT_UDP){
        uint16_t result = _mapWiFiUDP[i].parsePacket();
        if (result > 0){ 
          // notify presence of data
          uint8_t pkt[] = {((uint8_t *)&result)[0], ((uint8_t *)&result)[1]};
          _sendEventPacket(AVAIL_DATA_TCP, i, pkt, 2);
          #ifdef DEBUG_SERIALE
            Serial.println("sending udp event" + String(result));
          #endif
        }
      }
    }
  

  if (connectionTimeout != 0 && connectionTimeout < millis() && connectionStatus != WL_CONNECTED){
    connectionTimeout = 0;
    WiFi.disconnect();
    #ifdef DEBUG_SERIALE
      Serial.println("Connection timeout");
    #endif
  }

  if(startAutoconnect){
    startAutoconnect = false;
    autoconnect();
  }

  if (notify) {
    notify = false;
	// send the asynchronous event
    _notifySTAConnStatus(connectionStatus);
  }
}

bool WiFi2ControlClass::_processWiFiCmd(uint8_t cmd){
  bool rep = true;
    
  switch (cmd){
	case GET_CONN_STATUS:
	  _getStatus();
	  break;
	case GET_FW_VERSION:
	  _getFwVersion();
	  break;
	case CONNECT_OPEN_AP:
	  rep = _connect(0);
	  break;
	case CONNECT_SECURED_AP:
	  rep = _connect(1);
	  break;
	case AUTOCONNECT_TO_STA:
	  rep = _connect(2);
	  break;
	case DISCONNECT:
	  rep = _disconnect();
	  break;
	case CANCEL_NETWORK_LIST:
	  _clearWiFiSSIDList();
	  break;
	case SET_IP_CONFIG:
	  _config();
	  break;
	case SET_DNS_CONFIG:
	  _setDNS();
	  break;
	case GET_HOSTNAME:
	  _getHostname();
	  break;
	case GET_IPADDR:
	  _getNetworkData();
	  break;
	case GET_MACADDR:
	  _getMacAddress();
	  break;
	case GET_CURR_SSID:
	  _getCurrentSSID();
	  break;
	case GET_CURR_BSSID:
	  _getBSSID();
	  break;
	case GET_CURR_RSSI:
	  _getRSSI(1);
	  break;
	case GET_IDX_RSSI:
	  _getRSSI(0);
	  break;
	case GET_CURR_ENCT:
	  _getEncryption(1);
	  break;
	case GET_IDX_ENCT:
	  _getEncryption(0);
	  break;
	case START_SCAN_NETWORKS:
	  _startScanNetwork();
	  break;
	case SCAN_NETWORK_RESULT:
	  _scanNetwork(_tempPkt.dataPtr[1]);
	  break;
	case GET_HOST_BY_NAME:
	  _getHostByName();
	  break;
  }
  
  return rep;
}
	
bool WiFi2ControlClass::_processServerCmd(uint8_t cmd){
  uint8_t rep = true;
  
  switch (cmd){
	case START_SERVER_TCP:
	  _startServer();
	  break;
  }
  
  return rep;
}

bool WiFi2ControlClass::_processClientCmd(uint8_t cmd){
  uint8_t rep = true;
	  
  switch (cmd){
	case GET_DATA_TCP:
	  _getData();
	  break;
	case STOP_CLIENT_TCP:
	  _stopClient();
	  break;
	case GET_CLIENT_STATE_TCP:
	  _clientStatus();
	  break;
	case SEND_DATA_TCP:
	  rep = _sendData();
	  break;
	case START_CLIENT_TCP:
	  _startClient();
	  break;
	case SEND_DATA_UDP:
	  _sendUdpData();
	  break;
	case GET_REMOTE_DATA:
	  _remoteData();
	  break;
  }
  
  return rep;
}

bool WiFi2ControlClass::_processDataCmd(uint8_t cmd){
  bool rep = true;
  
  switch (cmd){
	case GET_DATABUF_TCP: 
	  _getDataBuf();
      break;
  }
  
  return rep;
}

void WiFi2ControlClass::_notifySTAConnStatus(uint8_t stat)
{ 
  communication.getLock();
  if(stat == WL_CONNECTED){
    if(connectedIndex_temp < 0){
      int8_t ind = -1;
      
      // the network was already in the list
      if((ind = _checkSSIDInList(tempNetworkParam.strSSID)) < 0){
        // SSID not present make a new entry
        networkArray.lastConnected = _networkListEvalAging(0xff);
        memset(networkArray.netEntry[networkArray.lastConnected].strSSID, 0, 33);
        memset(networkArray.netEntry[networkArray.lastConnected].strPWD, 0, 65);
        strcpy(networkArray.netEntry[networkArray.lastConnected].strSSID, tempNetworkParam.strSSID);

		communication.sendCmdPkt(CONNECT_OPEN_AP, PARAM_NUMS_1);

        if (tempNetworkParam.strPWD[0] != 0xff){
		  communication.sendCmdPkt(CONNECT_SECURED_AP, PARAM_NUMS_1);	
          strcpy(networkArray.netEntry[networkArray.lastConnected].strPWD, tempNetworkParam.strPWD);
        }
        // update the aging
        _networkListEvalAging(networkArray.lastConnected);
        delay(0);
        _storeWiFiSSIDList();
        
      }
      else{
        networkArray.lastConnected = ind;
        if(networkArray.netEntry[networkArray.lastConnected].strPWD[0] != 0xff)
		  communication.sendCmdPkt(CONNECT_SECURED_AP, PARAM_NUMS_1);
        else 
		  communication.sendCmdPkt(CONNECT_OPEN_AP, PARAM_NUMS_1);

        _networkListEvalAging(networkArray.lastConnected);
      }
    }
    else{
      networkArray.lastConnected = connectedIndex_temp;
      if(networkArray.netEntry[networkArray.lastConnected].strPWD[0] != 0xff)
		communication.sendCmdPkt(CONNECT_SECURED_AP, PARAM_NUMS_1);
      else 
		communication.sendCmdPkt(CONNECT_OPEN_AP, PARAM_NUMS_1);

      _networkListEvalAging(connectedIndex_temp);
      
    }
/*
    #ifdef DEBUG_SERIALE
      for(uint8_t i=0; i<MAX_NETWORK_LIST; i++){
        Serial.print("\nSSID: ");
        Serial.write(networkArray.netEntry[i].strSSID, 32);
        Serial.print("\nPWD: ");
        Serial.write(networkArray.netEntry[i].strPWD, 64);
        Serial.print("\naging: ");
        Serial.println(networkArray.netEntry[i].aging);
        
        delay(0);
      }
    #endif
*/
  }
  else if(stat == WL_DISCONNECTED){
	communication.sendCmdPkt(DISCONNECT, PARAM_NUMS_1);
    #ifdef DEBUG_SERIALE
      Serial.println("disconnecting...");
    #endif
  }

  connectedIndex_temp = -1;

  communication.sendParam((uint8_t*)&stat, PARAM_SIZE_1, LAST_PARAM);
  // manually enable reception after each asynchronous event
  communication.enableRx();
  communication.releaseLock();
}

/*
*
*/
void WiFi2ControlClass::_getHostname()
{
  String host = String(WiFi.getHostname());
  uint8_t sz = host.length();
   
  // set the response struct
  communication.getLock();
  communication.sendCmdPkt(_tempPkt.cmd, PARAM_NUMS_1);
  communication.sendParam((uint8_t*)host.c_str(), sz, LAST_PARAM);
  communication.releaseLock();
}

/*
*
*/
void WiFi2ControlClass::_getStatus()
{
  // Disconnet from the network
  uint8_t result = WiFi.status();

  // set the response struct
  communication.getLock();
  communication.sendCmdPkt(_tempPkt.cmd, PARAM_NUMS_1);
  communication.sendParam((uint8_t*)&result, PARAM_SIZE_1, LAST_PARAM);
  communication.releaseLock();
}

/*
*
*/
void WiFi2ControlClass::_getFwVersion()
{
  // set the response struct
  communication.getLock();
  communication.sendCmdPkt(_tempPkt.cmd, PARAM_NUMS_1);
  communication.sendParam((uint8_t*)FW_VERSION, PARAM_SIZE_5, LAST_PARAM);
  communication.releaseLock();
}

/*
*
*/
void WiFi2ControlClass::_getMacAddress()
{
  uint8_t mac[PARAM_SIZE_6];

  //Retrive mac address
  WiFi.macAddress(mac);

  // set the response struct
  communication.getLock();
  communication.sendCmdPkt(_tempPkt.cmd, PARAM_NUMS_1);
  communication.sendParam((uint8_t*)mac, PARAM_SIZE_6, LAST_PARAM);
  communication.releaseLock();
}

/*
*
*/
void WiFi2ControlClass::_getCurrentSSID()
{
  //retrieve SSID of the current network
  String result = WiFi.SSID();

  // set the response struct
  communication.getLock();
  communication.sendCmdPkt(_tempPkt.cmd, PARAM_NUMS_1);
  communication.sendParam((uint8_t*)result.c_str(), result.length(), LAST_PARAM);
  communication.releaseLock();
}

/*
*
*/
void WiFi2ControlClass::_getRSSI(uint8_t current)
{
  int32_t result;
  //retrieve RSSI
  if (current == 1)
  {
    result = WiFi.RSSI();
  }
  else
  {
    int8_t net_idx = _tempPkt.dataPtr[1];

    // NOTE: only for test this function
    // user must call scan network before
    // int num = WiFi.scanNetworks();
    result = WiFi.RSSI(net_idx);
  }

  // set the response struct
  communication.getLock();
  communication.sendCmdPkt(_tempPkt.cmd, PARAM_NUMS_1);
  uint8_t res[4] = {(int8_t)result, 0xFF, 0xFF, 0xFF};
  communication.sendParam(res, PARAM_SIZE_4, LAST_PARAM);
  communication.releaseLock();
}

/*
*
*/
void WiFi2ControlClass::_getEncryption(uint8_t current)
{

  if (current == 1)
  {
    String currSSID = WiFi.SSID(); //get current SSID
    for (int i = 0; i < _nets.nScannedNets; i++)
    {
      if (currSSID == WiFi.SSID(i))
      {
        _nets.currNetIdx = i; //get the index of the current network
        break;
      }
    }
  }
  else
  {
    _nets.currNetIdx = _tempPkt.dataPtr[1];
  }

  uint8_t result = WiFi.encryptionType(_nets.currNetIdx);

  // set the response struct
  communication.getLock();
  communication.sendCmdPkt(_tempPkt.cmd, PARAM_NUMS_1);
  communication.sendParam(&result, PARAM_SIZE_1, LAST_PARAM);
  communication.releaseLock();
}


uint8_t WiFi2ControlClass::_networkListEvalAging(uint8_t connectedIndex)
{
  uint8_t agingVal = 0;
  uint8_t agingValStore = 0;
  uint8_t oldest = 0;

  // request an index to store a new network
  if(connectedIndex == 0xff){
    for(uint8_t i=0; i<MAX_NETWORK_LIST; i++){

      if(networkArray.netEntry[i].aging == 0){
        networkArray.netEntry[i].aging = 1;
        return i;
      }

      // find out the oldest netowrk
      if(agingVal < networkArray.netEntry[i].aging){
        agingVal = networkArray.netEntry[i].aging;
        oldest = i;
      }
    }
  }
  else{
    agingValStore = networkArray.netEntry[connectedIndex].aging;
    networkArray.netEntry[connectedIndex].aging = 1;
    
    for(uint8_t i=0; i<MAX_NETWORK_LIST; i++){
      if(connectedIndex == i)
        continue;
      if(networkArray.netEntry[i].aging != 0 && networkArray.netEntry[i].aging < agingValStore){
        networkArray.netEntry[i].aging++;
      }
    }
  }
  
  return oldest;
}

int8_t WiFi2ControlClass::_checkSSIDInList(char* targetSSID)
{
  int8_t index = -1;

  for(uint8_t j=0; j<MAX_NETWORK_LIST; j++){
    if(strcmp(targetSSID, networkArray.netEntry[j].strSSID) == 0) {
      index = j;
    }
  }
  return index;
}

bool WiFi2ControlClass::_connect(uint8_t option)
{
  bool ret = false;
  uint8_t result;
  uint32_t start = 0;

  connectedIndex_temp = -1;
 
  // autoconnect mode
  if(option == 2) {
	#ifdef DEBUG_SERIALE
      Serial.println("Autoconnect mode");
    #endif

    char ssidNet[33] = {0};
    int32_t rssi = -100;
    int32_t tempRssi = 0;
    bool found = false;

    start = millis() + CONNECTION_TIMEOUT;
    _nets.nScannedNets = -1;

    //scanNetworks command
    do {
      _nets.nScannedNets = WiFi.scanNetworks(false, false);
      delayMicroseconds(100);
    } while (_nets.nScannedNets < 0 && start > millis());

    if (_nets.nScannedNets <= 0){
      _nets.nScannedNets = 0;
      result = WL_NO_SSID_AVAIL;
      #ifdef DEBUG_SERIALE
        Serial.println("no network found");
      #endif
    }
    else {
      for(uint8_t i=0; i<_nets.nScannedNets; i++) {
        strcpy(ssidNet, WiFi.SSID(i).c_str());
        for(uint8_t j=0; j<MAX_NETWORK_LIST; j++){
          if(strcmp((char*)ssidNet, networkArray.netEntry[j].strSSID) == 0) {
            found = true;
            if(networkArray.lastConnected == j){  //last connected -> connect directly
              connectedIndex_temp = j;

              #ifdef DEBUG_SERIALE
                Serial.println("Connect to the last connected");
              #endif
              break;
            }
            tempRssi = WiFi.RSSI(i);
            if(rssi < tempRssi) {
              rssi = tempRssi;
              connectedIndex_temp = j;
            }
          }
        }
        if(found)
          break;
        memset(ssidNet, 0, 33);
      }
    }
    // a network match found
    if(connectedIndex_temp >= 0) {
      #ifdef DEBUG_SERIALE
        Serial.print("\nSSID: ");
        Serial.println((char *)networkArray.netEntry[connectedIndex_temp].strSSID);
        Serial.print("PWD: ");
        Serial.println((char *)networkArray.netEntry[connectedIndex_temp].strPWD);
      #endif
      
      if(WiFi.status() == WL_CONNECTED){
        //if(networkArray.netEntry[connectedIndex_temp].strSSID != Config.getParam("ssid").c_str() != ssidNet)
          WiFi.disconnect();
      }

      connectionTimeout = millis() + CONNECTION_TIMEOUT;
      connectionStatus = WL_IDLE_STATUS;

      result = WiFi.begin(networkArray.netEntry[connectedIndex_temp].strSSID, networkArray.netEntry[connectedIndex_temp].strPWD);
    }
    else{
      #ifdef DEBUG_SERIALE
        Serial.println("No network memorized or match found");
      #endif
    }
  }
  else {
    //get parameter
    tsParameter ssid = {
      .len = _tempPkt.dataPtr[0],
      .paramPtr = (uint8_t *)(_tempPkt.dataPtr + 1),
    };

    char pwdBuf[65];
    // string size + terminator
    char ssidBuf[ssid.len + 1];
    memset(ssidBuf, 0, ssid.len + 1);
    memcpy(ssidBuf, ssid.paramPtr, ssid.len);
    ssidBuf[ssid.len] = '\0';
	
    // Open network, connection without password
    if (option == 0) {
      if(WiFi.status() == WL_CONNECTED){
        //if(networkArray.netEntry[connectedIndex_temp].strSSID != Config.getParam("ssid").c_str() != ssidNet)
          WiFi.disconnect();
      }

      connectionTimeout = millis() + CONNECTION_TIMEOUT;
      connectionStatus = WL_IDLE_STATUS;
      
      //set network and retrieve result
      result = WiFi.begin(ssidBuf);
      memset(tempNetworkParam.strSSID, 0, 33);
      memset(tempNetworkParam.strPWD, 0, 65);
      tempNetworkParam.aging = 0;

      strcpy(tempNetworkParam.strSSID, ssidBuf);
      connectionRequested = true;
    }
    // secured network, connection with password
    else if (option == 1) {
      //get parameter
      tsParameter pwd = {
        .len = _tempPkt.dataPtr[ssid.len + 1],
        .paramPtr = (uint8_t *)(_tempPkt.dataPtr + ssid.len + 2),
      };
      // string size + terminator
      //char pwdBuf[pwd.len + 1];
      memset(pwdBuf, 0, pwd.len + 1);
      memcpy(pwdBuf, pwd.paramPtr, pwd.len);
      pwdBuf[pwd.len] = '\0';

  #ifdef DEBUG_SERIALE
      Serial.print("\nSSID: ");
      Serial.println((char *)ssidBuf);
      Serial.print("PWD: ");
      Serial.println((char *)pwdBuf);
  #endif
      if(WiFi.status() == WL_CONNECTED){
        //if(networkArray.netEntry[connectedIndex_temp].strSSID != Config.getParam("ssid").c_str() != ssidNet)
          WiFi.disconnect();
      }

      connectionTimeout = millis() + CONNECTION_TIMEOUT;
      connectionStatus = WL_IDLE_STATUS;

      //set network and retrieve result
      result = WiFi.begin(ssidBuf, pwdBuf);
      memset(tempNetworkParam.strSSID, 0, 33);
      memset(tempNetworkParam.strPWD, 0, 65);
      tempNetworkParam.aging = 0;

      strcpy(tempNetworkParam.strSSID, ssidBuf);
      strcpy(tempNetworkParam.strPWD, pwdBuf);

      connectionRequested = true;
    }
    else {
      //_createErrorResponse();
      return ret;
    }
  }
  return ret;
}


/*
*
*/
bool WiFi2ControlClass::_disconnect()
{
  bool ret = false;
  //Disconnet from the network
  uint8_t result = WiFi.disconnect();
  return ret;
}

//ok
/*
* START_CMD, CMD | REPLY_FLAG, PARAM_NUMS_1, PARAM_SIZE_1, numNets, END_CMD
*/
void WiFi2ControlClass::_startScanNetwork()
{
  //scanNetworks command
  _nets.nScannedNets = WiFi.scanNetworks();
  
  if (_nets.nScannedNets < 0)
    _nets.nScannedNets = 0;

  //set the response struct
  communication.getLock();
  communication.sendCmdPkt(_tempPkt.cmd, PARAM_NUMS_1);
  communication.sendParam((uint8_t*)&_nets.nScannedNets, PARAM_SIZE_1, LAST_PARAM);
  communication.releaseLock();
}

/*
* START_CMD, CMD | REPLY_FLAG, NET_NUMBER, SSID_LEN, SSID_NAME, END_CMD
*/
void WiFi2ControlClass::_scanNetwork(uint8_t which)
{
  String ssidNet = WiFi.SSID(which);
  uint8_t enc = WiFi.encryptionType(which);
  int32_t rssi = WiFi.RSSI(which);
  delay(10);

  uint32_t result = ssidNet.length() + sizeof(enc) + sizeof(rssi) + 2;
  uint8_t buff[result];
  buff[0] = which;
  buff[1] = (uint8_t)rssi;
  buff[2] = 0xff;
  buff[3] = 0xff;
  buff[4] = 0xff;
  buff[5] = enc;
  buff[6] = ssidNet.length();
  memcpy((buff + 7), ssidNet.c_str(), ssidNet.length());

  communication.getLock();  
  communication.sendDataPkt(_tempPkt.cmd);
  communication.appendBuffer(buff, result);
  communication.releaseLock();
}

/*
*
*/
void WiFi2ControlClass::_getBSSID()
{
  int paramLen = 6;
  uint8_t idx = 0; //network index
  uint8_t *result;

  //Retrive the BSSID
  result = WiFi.BSSID();

  //set the response struct
  communication.getLock();
  communication.sendCmdPkt(_tempPkt.cmd, PARAM_NUMS_1);
  communication.sendParam(result, paramLen, LAST_PARAM);
  communication.releaseLock();
}

void WiFi2ControlClass::_config()
{
  bool result;
  uint8_t validParams = 0;

  uint8_t stip0, stip1, stip2, stip3,
    gwip0, gwip1, gwip2, gwip3,
    snip0, snip1, snip2, snip3;

  validParams = _tempPkt.dataPtr[1];

  //retrieve the static IP address
  stip0 = _tempPkt.dataPtr[3];
  stip1 = _tempPkt.dataPtr[4];
  stip2 = _tempPkt.dataPtr[5];
  stip3 = _tempPkt.dataPtr[6];
  _handyIp = new IPAddress(stip0, stip1, stip2, stip3);

  //retrieve the gateway IP address
  gwip0 = _tempPkt.dataPtr[8];
  gwip1 = _tempPkt.dataPtr[9];
  gwip2 = _tempPkt.dataPtr[10];
  gwip3 = _tempPkt.dataPtr[11];
  _handyGateway = new IPAddress(gwip0, gwip1, gwip2, gwip3);

  //retrieve the subnet mask
  snip0 = _tempPkt.dataPtr[13];
  snip1 = _tempPkt.dataPtr[14];
  snip2 = _tempPkt.dataPtr[15];
  snip3 = _tempPkt.dataPtr[16];
  _handySubnet = new IPAddress(snip0, snip1, snip2, snip3);

  result = WiFi.config(*_handyIp, *_handyGateway, *_handySubnet);

  communication.getLock();
  communication.sendCmdPkt(_tempPkt.cmd, PARAM_NUMS_1);
  communication.sendParam((uint8_t*)&result, PARAM_SIZE_1, LAST_PARAM);
  communication.releaseLock();
}

void WiFi2ControlClass::_setDNS()
{
  bool result;
  uint8_t validParams = 0;

  validParams = _tempPkt.dataPtr[1];

  uint8_t dns1ip0, dns1ip1, dns1ip2, dns1ip3,
    dns2ip0, dns2ip1, dns2ip2, dns2ip3;

  //retrieve the dns 1 address
  dns1ip0 = _tempPkt.dataPtr[3];
  dns1ip1 = _tempPkt.dataPtr[4];
  dns1ip2 = _tempPkt.dataPtr[5];
  dns1ip3 = _tempPkt.dataPtr[6];
  IPAddress dns1(dns1ip0, dns1ip1, dns1ip2, dns1ip3);

  //retrieve the dns 2 address
  dns2ip0 = _tempPkt.dataPtr[8];
  dns2ip1 = _tempPkt.dataPtr[9];
  dns2ip2 = _tempPkt.dataPtr[10];
  dns2ip3 = _tempPkt.dataPtr[11];
  IPAddress dns2(dns2ip0, dns2ip1, dns2ip2, dns2ip3);

  result = WiFi.config(*_handyIp, *_handyGateway, *_handySubnet, dns1, dns2);

  communication.getLock();
  communication.sendCmdPkt(_tempPkt.cmd, PARAM_NUMS_1);
  communication.sendParam((uint8_t*)&result, PARAM_SIZE_1, LAST_PARAM);
  communication.releaseLock();
}


void WiFi2ControlClass::_getHostByName()
{
  uint8_t result;
  IPAddress _reqHostIp;
  char host[_tempPkt.dataPtr[0]+1];
  //get the host name to look up
  //strncpy(host, _reqPckt.params[0].param, _reqPckt.params[0].paramLen);
  memcpy(host, &_tempPkt.dataPtr[1], _tempPkt.dataPtr[0]);
  host[_tempPkt.dataPtr[0]] = '\0';

  result = WiFi.hostByName(host, _reqHostIp); //retrieve the ip address of the host
  
  communication.getLock();
  communication.sendCmdPkt(_tempPkt.cmd, PARAM_NUMS_1);

  if(result == 0){ // call failed, return 0
    communication.sendParam(&result, PARAM_SIZE_1, LAST_PARAM);
  }
  else{ //call succeded, return host's ip
	uint8_t res[4] = {_reqHostIp.operator[](0), _reqHostIp.operator[](1), _reqHostIp.operator[](2), _reqHostIp.operator[](3)};
    communication.sendParam(res, PARAM_SIZE_4, LAST_PARAM);
  }
  communication.releaseLock();
}

void WiFi2ControlClass::_getNetworkData()
{
  IPAddress localIp, subnetMask, gatewayIp; //, dnsIp1, dnsIp2;

  subnetMask = WiFi.subnetMask();
  gatewayIp = WiFi.gatewayIP();
  localIp = WiFi.localIP();
  
  uint32_t tim = millis() + 5000;

  while(localIp.operator[](0) == 0 && millis() < tim){
    delay(1);
    localIp = WiFi.localIP();
  }

  //set the response struct
  communication.sendCmdPkt(_tempPkt.cmd, PARAM_NUMS_3);
  uint8_t res[4] = {localIp.operator[](0), localIp.operator[](1), localIp.operator[](2), localIp.operator[](3)};
  communication.sendParam(res, PARAM_SIZE_4, NO_LAST_PARAM);
  res[0] = subnetMask.operator[](0);
  res[1] = subnetMask.operator[](1);
  res[2] = subnetMask.operator[](2);
  res[3] = subnetMask.operator[](3);
  communication.sendParam(res, PARAM_SIZE_4, NO_LAST_PARAM);
  res[0] = gatewayIp.operator[](0);
  res[1] = gatewayIp.operator[](1);
  res[2] = gatewayIp.operator[](2);
  res[3] = gatewayIp.operator[](3);
  communication.sendParam(res, PARAM_SIZE_4, LAST_PARAM);
}

/* WiFI Server */
void WiFi2ControlClass::_startServer()
{
  uint8_t result = 0;

  uint8_t _p1 = _tempPkt.dataPtr[1];
  uint8_t _p2 = _tempPkt.dataPtr[2];

  //retrieve the port to start server
  uint16_t port = (_p1 << 8) + _p2;
  //retrieve sockets number
  uint8_t sock = _tempPkt.dataPtr[4];
  //retrieve protocol mode (TCP/UDP)
  uint8_t prot = _tempPkt.dataPtr[6];

  if (sock < MAX_SOCK_NUMBER)
  {
    if (prot == PROT_TCP)
    { //TCP MODE
      if (_mapWiFiServers[sock] != NULL)
      {
        //_mapWiFiServers[_sock]->stop();
        _mapWiFiServers[sock]->close();
        free(_mapWiFiServers[sock]);
      }
      if (port == 80)
      {
        //UIserver.stop();      //stop UI SERVER
        UI_alert = true;
      }
      _mapWiFiServers[sock] = new WiFiServer(port);
      _mapWiFiServers[sock]->begin();
      result = 1;
    }
    else
    { //UDP MODE
      //if (_mapWiFiUDP[_sock] != NULL)
      {
        _mapWiFiUDP[sock].stop();
      }
      _mapWiFiUDP[sock].begin(port);
      _port[sock] = port;
      _sockInUse[sock].inUse = true;
      _sockInUse[sock].protocol = PROT_UDP;
      result = 1;
    }
  }

  //set the response struct
  communication.getLock();
  communication.sendCmdPkt(_tempPkt.cmd, PARAM_NUMS_1);
  communication.sendParam(&result, PARAM_SIZE_1, LAST_PARAM);
  communication.releaseLock();
}

void WiFi2ControlClass::_getData()
{
  uint8_t result = 0;
  //retrieve socket index
  uint8_t _sock = _tempPkt.dataPtr[1];
  //retrieve peek
  uint8_t _p1 = _tempPkt.dataPtr[3];
  uint8_t _p2 = _tempPkt.dataPtr[4];
  uint16_t _peek = (_p1 << 8) + _p2;

  if(_sockInUse[_sock].protocol == PROT_TCP){
    if(_mapWiFiClients[_sock].available() > 0){
      if (_peek > 0) {
        result = _mapWiFiClients[_sock].peek();
      }
      else {
        result = _mapWiFiClients[_sock].read();
        if (_oldDataClient[_sock])
          _oldDataClient[_sock]--;
      }
    }
  }
  else // PROT_UDP
  {
    if (_peek > 0) {
      result = _mapWiFiUDP[_sock].peek();
    }
    else {
      result = _mapWiFiUDP[_sock].read();
    }
  }

  //set the response struct
  communication.getLock();
  communication.sendCmdPkt(_tempPkt.cmd, PARAM_NUMS_1);
  communication.sendParam(&result, PARAM_SIZE_1, LAST_PARAM);
  communication.releaseLock();
}

/* WiFi Client */
void WiFi2ControlClass::_stopClient()
{
  uint8_t result = 0;
  uint8_t _sock = _tempPkt.dataPtr[1];
  if (_sock < MAX_SOCK_NUMBER)
  {
    if (_sockInUse[_sock].protocol == PROT_TCP)
    {
      _mapWiFiClients[_sock].stop();
      _oldDataClient[_sock] = 0;
      _sockInUse[_sock].inUse = false;
      result = 1;
    }
    else // PROT_UDP
    {
      _mapWiFiUDP[_sock].stop();
      _oldDataClient[_sock] = 0;
      _sockInUse[_sock].inUse = false;
      result = 1;
    }
  }

  //set the response struct
  communication.getLock();
  communication.sendCmdPkt(_tempPkt.cmd, PARAM_NUMS_1);
  communication.sendParam(&result, PARAM_SIZE_1, LAST_PARAM);
  communication.releaseLock();
}

void WiFi2ControlClass::_clientStatus()
{
  uint8_t result = 0;
  uint8_t _sock = (uint8_t)_tempPkt.dataPtr[1];

  if (_sock < MAX_SOCK_NUMBER) {
    if (!_mapWiFiClients[_sock]) { //_mapWiFiClients[_sock] == NULL
      if (_mapWiFiServers[_sock] != NULL) {
        _mapWiFiClients[_sock] = _mapWiFiServers[_sock]->available(); //Create the client from the server [Arduino as a Server]
        // [cr] TODO - status function on client is not implemented. check for alternatives
        result = 0; //_mapWiFiClients[_sock].status();
      }
    }
    else {
      // [cr] TODO - status function on client is not implemented. check for alternatives
      result = 0; //_mapWiFiClients[_sock].status();
    }
  }

  //set the response struct
  communication.getLock();
  communication.sendCmdPkt(_tempPkt.cmd, PARAM_NUMS_2);
  communication.sendParam(&_sock, PARAM_SIZE_1, LAST_PARAM);
  communication.sendParam(&result, PARAM_SIZE_1, LAST_PARAM);
  communication.releaseLock();
}

bool WiFi2ControlClass::_sendData()
{
  if(_tempPkt.cmdType == DATA_PKT && communication.endReceived() == false) {
    if(_tempPkt.nParam == 26) {
	  // socket number is the first byte in packet payload
	  _sockNumber = _tempPkt.dataPtr[0];
      _sockBuffInit();
      _sockBuffAdd(&_tempPkt.dataPtr[1], _tempPkt.nParam - 1);
    }
    else{
      _sockBuffAdd(_tempPkt.dataPtr, _tempPkt.nParam);
    }
  }
  else {
    if(_tempPkt.nParam == 255){ // 255 indicates a special case where data packet payload fits 1 packet
	  // socket number is the first byte in packet payload
	  _sockNumber = _tempPkt.dataPtr[0];
      _sockBuffInit();
      _sockBuffAdd(&_tempPkt.dataPtr[1], _tempPkt.totalLen);
    }
    else{
      _sockBuffAdd(_tempPkt.dataPtr, _tempPkt.nParam);
    }

  #ifdef DEBUG_SERIALE
      //Serial.print("\r\nsocket msg: ");
     //Serial.write(_dataBuf, _dataBufSz);
  #endif
  
    uint8_t _sock = _sockNumber;
    uint32_t result = 0;
    uint8_t tcpResult = 0;
    
    if (_sock < MAX_SOCK_NUMBER && _sockInUse[_sock].inUse) {
      if(_sockInUse[_sock].protocol == PROT_TCP)
      {
        if (_mapWiFiClients[_sock].connected()){
          // empty client's cache before writing the response
          while(_mapWiFiClients[_sock].available()){
            _mapWiFiClients[_sock].read();
          }
          result = _mapWiFiClients[_sock].write((uint8_t *)(_dataBuf), _dataBufSz);
        }
      }
      else //PROT_UDP
      { 
        result = _mapWiFiUDP[_sock].write((uint8_t *)(_dataBuf), _dataBufSz);
      }
      if (result == _dataBufSz)
        tcpResult = 1;
      else
        tcpResult = 0;
    }
  }
  
  return false;
}

void WiFi2ControlClass::_startClient()
{
  uint8_t result = 0;
  int sock;
  uint16_t port;
  uint8_t prot;

  //retrieve the IP address to connect to
  uint8_t stip1 = _tempPkt.dataPtr[1];
  uint8_t stip2 = _tempPkt.dataPtr[2];
  uint8_t stip3 = _tempPkt.dataPtr[3];
  uint8_t stip4 = _tempPkt.dataPtr[4];
  IPAddress ip(stip1, stip2, stip3, stip4);

  //retrieve the _port to connect to
  uint8_t _p1 = _tempPkt.dataPtr[6];
  uint8_t _p2 = _tempPkt.dataPtr[7];
  port = (_p1 << 8) + _p2;

  //retrieve sockets number
  sock = (int)_tempPkt.dataPtr[9];

  //retrieve protocol mode (TCP/UDP)
  prot = _tempPkt.dataPtr[11];

  if (sock < MAX_SOCK_NUMBER) {
    if (prot == PROT_TCP) {
      //TCP MODE
      if (_mapWiFiClients[sock]) {
        _mapWiFiClients[sock].stop();
        _oldDataClient[sock] = 0;
        _sockInUse[sock].inUse = false;
      }
      result = _mapWiFiClients[sock].connect(ip, port);
      if(result){
        _sockInUse[sock].inUse = true;
        _sockInUse[sock].conn = true;
        _sockInUse[sock].protocol = PROT_TCP;
      }
    }
    else {
      //UDP MODE
      result = _mapWiFiUDP[sock].beginPacket(ip, port);
      _sockInUse[sock].protocol = PROT_UDP;
    }
  }
  //set the response struct
  communication.getLock();
  communication.sendCmdPkt(_tempPkt.cmd, PARAM_NUMS_1);
  communication.sendParam(&result, PARAM_SIZE_1, LAST_PARAM);
  communication.releaseLock();
}

/* WiFi UDP Client */
void WiFi2ControlClass::_remoteData()
{
  uint8_t _sock;

  //retrieve sockets number
  _sock = _tempPkt.dataPtr[1];

  if (_sock < MAX_SOCK_NUMBER /*&& _mapWiFiUDP[_sock] != NULL*/)
  {
    IPAddress remoteIp = _mapWiFiUDP[_sock].remoteIP();
    uint16_t remotePort = _mapWiFiUDP[_sock].remotePort();

      
    //set the response struct
	communication.getLock();
	communication.sendCmdPkt(_tempPkt.cmd, PARAM_NUMS_2);
	uint8_t res[4] = {remoteIp.operator[](0), remoteIp.operator[](1), remoteIp.operator[](2), remoteIp.operator[](3)};
	communication.sendParam(res, PARAM_SIZE_4, NO_LAST_PARAM);
	communication.sendParam(remotePort, LAST_PARAM);
	communication.releaseLock();
  }
  else
  {
    // send 0 to report an error
	communication.getLock();
	communication.sendCmdPkt(_tempPkt.cmd, PARAM_NUMS_1);
	uint8_t res = 0;
	communication.sendParam(&res, PARAM_SIZE_1, LAST_PARAM);
	communication.releaseLock();
  }
}

void WiFi2ControlClass::_getDataBuf()
{
  int32_t result = 0;
  uint8_t _sock = _tempPkt.dataPtr[1];
  uint16_t _len = _tempPkt.dataPtr[4] + (_tempPkt.dataPtr[3] << 8);
  
  if (_sock < MAX_SOCK_NUMBER) {
    if (_sockInUse[_sock].protocol == PROT_TCP) {
	  int32_t len = (_mapWiFiClients[_sock].available() < _len) ? _mapWiFiClients[_sock].available() : _len;
	  if(len > 0){
		result = _mapWiFiClients[_sock].read(_dataBuf, len);
		_oldDataClient[_sock] -= result;
	  }
    }
    else //PROT_UDP
    {
      // read available data
        result = _mapWiFiUDP[_sock].read(_dataBuf, _len);
    }
  }
  
  #ifdef DEBUG_SERIALE
      //Serial.print("\r\nget data buf ");
      //Serial.println(result);
  #endif
  
  communication.getLock();
  communication.sendDataPkt(_tempPkt.cmd);
  communication.appendBuffer(_dataBuf, result);
  communication.releaseLock();
}


/*
*
*/
void WiFi2ControlClass::_sendEventPacket(uint8_t cmd, uint8_t sock, uint8_t *result, uint8_t resLen)
{
  communication.getLock();
  communication.sendCmdPkt(cmd, PARAM_NUMS_2);
  communication.sendParam(&sock, PARAM_SIZE_1, NO_LAST_PARAM);
  communication.sendParam(result, resLen, LAST_PARAM);
  // manually enable reception after each asynchronous event
  communication.enableRx();
  communication.releaseLock();
}

/*
*
*/
void WiFi2ControlClass::_sendUdpData()
{
  uint8_t result = 0;
  uint8_t _sock = 0;

  //retrieve socket index
  _sock = _tempPkt.dataPtr[1];

  if (_sock < MAX_SOCK_NUMBER /*&& _mapWiFiUDP[_sock] != NULL*/)
  {
    //send data to client
    result = _mapWiFiUDP[_sock].endPacket();
    _mapWiFiUDP[_sock].begin(_port[_sock]);
  }
  
  //set the response struct
  communication.getLock();
  communication.sendCmdPkt(_tempPkt.cmd, PARAM_NUMS_1);
  communication.sendParam(&result, PARAM_SIZE_1, LAST_PARAM);
  communication.releaseLock();
}



void WiFi2ControlClass::_fetchWiFiSSIDList()
{
  uint8_t val = 0;
  uint8_t* addr = (uint8_t*)&networkArray;

  for(uint16_t i=0; i<sizeof(tsNetArray); i++){
    val = EEPROM.read(i);
    memcpy((uint8_t*)(addr + i), (uint8_t*)&val, 1);
  }
}

void WiFi2ControlClass::_storeWiFiSSIDList()
{
  uint8_t* addr = (uint8_t*)&networkArray;

  for (uint16_t i = 0; i < 1024; i++) {
    delay(1);
    if(i <= sizeof(tsNetArray)){
      EEPROM.write(i, *((uint8_t*)(addr + i)));
    }
    else
      EEPROM.write(i, 0xff);
  }
  EEPROM.commit();
}

void WiFi2ControlClass::_clearWiFiSSIDList()
{
  for (uint16_t i = 0; i < 1024; i++) {
    EEPROM.write(i, 0xff);
  }
  EEPROM.commit();
}

uint32_t WiFi2ControlClass::_sockBuffInit(void)
{
  memset(_dataBuf, 0, MTU_SIZE);
  _dataBufSz = 0;
}

uint32_t WiFi2ControlClass::_sockBuffAdd(uint8_t* data, uint32_t sz)
{
  if(_dataBufSz + sz <= MTU_SIZE){
    memcpy((uint8_t*)(_dataBuf + _dataBufSz), data, sz);
    _dataBufSz += sz;
  }
  return _dataBufSz;
}

WiFi2ControlClass wifi2control;
