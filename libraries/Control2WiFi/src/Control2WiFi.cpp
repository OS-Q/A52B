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

#include "Control2WiFi.h"
#include "utility/wifi_drv.h"
#include "communication.h"
#include "utility/CommCmd.h"

extern "C" {
#include "utility/wl_definitions.h"
#include "utility/wl_types.h"
}

// XXX: don't make assumptions about the value of MAX_SOCK_NUM.
uint8_t Control2WiFi::_hostname[MAX_HOSTNAME_LEN]{ 0 };
uint8_t Control2WiFi::state[MAX_SOCK_NUM] = { 0, 0, 0, 0 };
uint16_t Control2WiFi::server_port[MAX_SOCK_NUM] = { 0, 0, 0, 0 };
uint16_t Control2WiFi::client_data[MAX_SOCK_NUM] = { 0, 0, 0, 0 };

bool Control2WiFi::gotResponse = false;
uint8_t Control2WiFi::responseType = 0x00;
uint32_t Control2WiFi::dataLen = 0x0000;
uint32_t Control2WiFi::totalLen = 0x0000;
uint8_t* Control2WiFi::data = NULL;
wl_status_t Control2WiFi::connectionStatus = WL_NO_WIFI_MODULE_COMM;
bool Control2WiFi::notify = false;

static tsDataPacket dataPkt;
static tsNewCmd cmdPkt;
char fwVersion[WL_FW_VER_LENGTH] = { 0 };

/* -----------------------------------------------------------------
* static callback that handles the data coming from the SPI driver
* calling the packet decoder to fire the right callback group
*/
static void wifiDrvCB(uint8_t* pkt)
{
    Control2WiFi::gotResponse = false;
    Control2WiFi::responseType = NONE;

	uint8_t cmd = pkt[1] - 0x80;

	// check asynch events at first
	if(pkt[0] == START_CMD){
		if(cmd == CONNECT_SECURED_AP || cmd == CONNECT_OPEN_AP || cmd == DISCONNECT){
			Control2WiFi::gotResponse = true;
			Control2WiFi::responseType = cmd;
			Control2WiFi::connectionStatus = (wl_status_t)pkt[5];
			Control2WiFi::notify = true;
			return;
		}
		if(cmd == GET_CLIENT_STATE_TCP){
			Control2WiFi::state[pkt[5]] = pkt[7];
			return;
		}
		if(cmd == AVAIL_DATA_TCP){
			Control2WiFi::client_data[pkt[5]] = pkt[7] | (uint16_t)(pkt[8] << 8);
			return;
		}
	}
	
	// check multipacket continuation
	if(dataPkt.totalLen > 0) {
		uint8_t rxIndex = 0, payloadSize = 0;
		for(uint8_t i=0; i<BUF_SIZE; i++){
			if(pkt[i] == END_CMD){
				rxIndex = i;
				if(dataPkt.receivedLen + rxIndex == dataPkt.totalLen - 1){
					dataPkt.endReceived = true;
					payloadSize = rxIndex;
					dataPkt.receivedLen = rxIndex = dataPkt.totalLen = 0;
					break;
				}
			}
		}
		if(dataPkt.endReceived == false) {
			payloadSize = BUF_SIZE;
			dataPkt.receivedLen += BUF_SIZE;
			if(dataPkt.receivedLen >= dataPkt.totalLen){
				dataPkt.endReceived = true;
				dataPkt.receivedLen = rxIndex = dataPkt.totalLen = 0;
			}
		}
		
		Control2WiFi::gotResponse = true;
		Control2WiFi::responseType = dataPkt.cmd;
		Control2WiFi::data = (uint8_t*)&pkt[0];
		Control2WiFi::dataLen = payloadSize;
		
		return;
	}
	
	// check multipacket start
	else if(pkt[0] == DATA_PKT) {
		// cast the buffer to the packet structure
		memset((tsDataPacket*)&dataPkt, 0, sizeof(tsDataPacket));
		memcpy((tsDataPacket*)&dataPkt, pkt, sizeof(tsDataPacket));
		dataPkt.cmd -= 0x80;

		// 32 byte - (cmdType (1 byte) + cmd (1 byte) + totalLen (4 byte)) = 32 - 6 = 26
		dataPkt.receivedLen = BUF_SIZE;
		dataPkt.endReceived = false;
		
		cmdPkt.dataPtr = (uint8_t*)(pkt + 6);
		int32_t pktLen = (int32_t)(pkt[2] + (pkt[3] << 8) + (pkt[4] << 16) + (pkt[5] << 24));
		dataPkt.totalLen = pktLen;
		
		Control2WiFi::totalLen = pktLen - 7;
		Control2WiFi::dataLen = BUF_SIZE - 6; // do not consider END_CMD
				
		if(dataPkt.totalLen <= BUF_SIZE){ // data are contained in a single packet control the packet footer.
			bool ret = 0;
			if(pkt[dataPkt.totalLen - 1] != END_CMD){
				ret = 1;
			}
			Control2WiFi::totalLen = dataPkt.totalLen - 7;
			Control2WiFi::dataLen = dataPkt.totalLen - 7;
			dataPkt.totalLen = 0;
			dataPkt.receivedLen = 0;
			dataPkt.endReceived = true;
			
			if(ret){
				return;
			}
		}
		
		switch(dataPkt.cmd){
			case SCAN_NETWORKS_RESULT:
			case GET_DATABUF_TCP:
				Control2WiFi::gotResponse = true;
				Control2WiFi::responseType = dataPkt.cmd;
				Control2WiFi::data = (uint8_t*)(cmdPkt.dataPtr);
				break;
		}
		return;
	}

	// check single packet
	else if (pkt[0] == START_CMD) {
		// cast the buffer to the packet structure
		memset((tsNewCmd*)&cmdPkt, 0, sizeof(tsNewCmd));
		memcpy((tsNewCmd*)&cmdPkt, pkt, sizeof(tsNewCmd));
		// remove the REPLY_FLAG
		cmdPkt.cmd -= 0x80;
		cmdPkt.dataPtr = (uint8_t*)(pkt + 4);
		
        switch (cmdPkt.cmd) {
            case COMPANION_READY:
                break;

            case GET_CONN_STATUS:
				// check the number of parameters of the command
				if(cmdPkt.nParam == PARAM_NUMS_1){
					// check the parameter len and the packet footer.
					if (cmdPkt.dataPtr[0] == PARAM_LEN_SIZE && pkt[cmdPkt.totalLen - 1] == END_CMD) {
						// set gotResponse flag to true to signal that esp replied us
						Control2WiFi::gotResponse = true;
						Control2WiFi::responseType = GET_CONN_STATUS;
						// extract the data from the data pointer
						Control2WiFi::data = (uint8_t*)(cmdPkt.dataPtr + 1);
						Control2WiFi::connectionStatus = (wl_status_t)(Control2WiFi::data[0]);
					}
				}
                break;

				            
            case GET_HOSTNAME:
			case GET_MACADDR:
			case SET_DNS_CONFIG:
			case GET_FW_VERSION:
				if(cmdPkt.nParam == PARAM_NUMS_1 && pkt[cmdPkt.totalLen - 1] == END_CMD){
					Control2WiFi::gotResponse = true;
					Control2WiFi::responseType = cmdPkt.cmd;
					// extract the data from the data pointer
					Control2WiFi::data = (uint8_t*)(cmdPkt.dataPtr);
				}
                break;

            case START_SCAN_NETWORKS:{
                // check the parameter len
				uint8_t paramLen = (cmdPkt.dataPtr[0] > WL_NETWORKS_LIST_MAXNUM) ? WL_NETWORKS_LIST_MAXNUM : cmdPkt.dataPtr[0];
                if (paramLen == PARAM_LEN_SIZE && pkt[cmdPkt.totalLen - 1] == END_CMD) {
					// set gotResponse flag to true to signal that esp replied us
                    Control2WiFi::gotResponse = true;
                    Control2WiFi::responseType = cmdPkt.cmd;
                    // extract the data from the data pointer
                    Control2WiFi::data = (uint8_t*)(cmdPkt.dataPtr + 1);
					Control2WiFi::dataLen = paramLen;
				}
			}break;
			

			case SET_KEY:
				if(cmdPkt.nParam == PARAM_NUMS_1 && pkt[cmdPkt.totalLen - 1] == END_CMD){
					// set gotResponse flag to true to signal that esp replied us
					Control2WiFi::gotResponse = true;
					Control2WiFi::responseType = cmdPkt.cmd;
					// extract the data from the data pointer
					Control2WiFi::data = (uint8_t*)(cmdPkt.dataPtr + 1);
					Control2WiFi::connectionStatus = (wl_status_t)(cmdPkt.dataPtr[1]);
					Control2WiFi::notify = true;
				}
                break;

            case GET_CURR_SSID:
				if(cmdPkt.nParam == PARAM_NUMS_1/* && pkt[cmdPkt.totalLen - 1] == END_CMD*/){
					// set gotResponse flag to true to signal that esp replied us
					Control2WiFi::gotResponse = true;
					Control2WiFi::responseType = GET_CURR_SSID;
					if(cmdPkt.totalLen > BUF_SIZE){
						dataPkt.totalLen = cmdPkt.totalLen;
						dataPkt.receivedLen = BUF_SIZE;
						dataPkt.endReceived = false;
						dataPkt.cmd = cmdPkt.cmd;
						Control2WiFi::dataLen = dataPkt.totalLen - 6;
					}
					// extract the data from the data pointer
					Control2WiFi::data = (uint8_t*)(cmdPkt.dataPtr);
				}
				break;
					
            case GET_CURR_BSSID:
			case GET_CURR_RSSI:
			case GET_IDX_RSSI:
			case GET_CURR_ENCT:
			case GET_IDX_ENCT:
			case GET_HOST_BY_NAME:
			case SET_IP_CONFIG:
			case GET_DATA_TCP:
			case STOP_CLIENT_TCP:
			case START_CLIENT_TCP:
			case SEND_DATA_TCP:
			case SEND_DATA_UDP:
			case START_SERVER_TCP:
				if(cmdPkt.nParam == PARAM_NUMS_1 && pkt[cmdPkt.totalLen - 1] == END_CMD){
					// set gotResponse flag to true to signal that companion chip replied us
					Control2WiFi::gotResponse = true;
					Control2WiFi::responseType = cmdPkt.cmd;
					// extract the data from the data pointer
					Control2WiFi::data = (uint8_t*)(cmdPkt.dataPtr + 1);
					Control2WiFi::dataLen = cmdPkt.dataPtr[0];
				}
                break;

            case GET_IPADDR:
				if(cmdPkt.nParam == PARAM_NUMS_3 && pkt[cmdPkt.totalLen - 1] == END_CMD){
					// set gotResponse flag to true to signal that companion replied us
					Control2WiFi::gotResponse = true;
					Control2WiFi::responseType = cmdPkt.cmd;
					// extract the data from the data pointer
					Control2WiFi::data = (uint8_t*)(cmdPkt.dataPtr);
				}
                break;
				
			    case GET_REMOTE_DATA:
				// check the number of parameters of the command. PARAM_NUMS is 1 if an error occured, 2 otherwise
				if((cmdPkt.nParam == PARAM_NUMS_1 || cmdPkt.nParam == PARAM_NUMS_2) && pkt[cmdPkt.totalLen - 1] == END_CMD){
					Control2WiFi::gotResponse = true;
					Control2WiFi::responseType = cmdPkt.cmd;
					Control2WiFi::dataLen = (uint16_t)cmdPkt.dataPtr[0];
					Control2WiFi::data = (uint8_t*)(cmdPkt.dataPtr + 1);
				}
                break;				
		}
	}
}

/* -----------------------------------------------------------------
*
*/
Control2WiFi::Control2WiFi()
{
    connectionStatus = WL_NO_WIFI_MODULE_COMM;
}

/* -----------------------------------------------------------------
* handles the WiFi driver event to see if an ISR has been detected
*/
void Control2WiFi::handleEvents(void)
{
    communication.handleCommEvents();
}

/* -----------------------------------------------------------------
* Initializes the wifi class receiving the relative callback and 
* registering it
*/
void Control2WiFi::init()
{
	// register command handler with communication object. In this way we will be able to receive commands
	communication.registerHandle(wifiDrvCB);
}

/*
*
*/
uint16_t Control2WiFi::getAvailableData()
{
    gotResponse = false;
    responseType = NONE;

    if (dataPkt.totalLen == 0) {
		handleEvents();
    } else if (dataPkt.totalLen > dataPkt.receivedLen) {
		handleEvents();
    } else {
        dataPkt.cmdType = 0;
        dataPkt.cmd = NONE;
        dataPkt.totalLen = 0;
        dataPkt.receivedLen = 0;
        dataPkt.endReceived = false;
    }
    if(gotResponse && responseType == GET_DATABUF_TCP)
        return dataLen;

    return 0;
}

// -----------------------------------------------------------------
char* Control2WiFi::getHostname()
{
    handleEvents();

    gotResponse = false;
    responseType = NONE;

    if (!WiFiDrv::getHostname()) { // packet has not been sent. Maybe an interrupt occurred in the meantime
        // launch interrupt management function, then try to send request again
        uint32_t start = millis();
		while(millis() - start < 10)
			handleEvents();
        if (!WiFiDrv::getHostname()) // exit if another error occurs
            return NULL;
    }

    // Poll response for the next 10 seconds
    uint32_t start = millis();
    while (((millis() - start) < GENERAL_TIMEOUT)) {
        handleEvents();
        if (gotResponse && responseType == GET_HOSTNAME) {
            uint8_t len = data[0];
            memset(_hostname, 0, MAX_HOSTNAME_LEN);
            memcpy(_hostname, (void*)&data[1], len);
            return (char*)_hostname;
        }
    }
    return NULL;
}

// -----------------------------------------------------------------
bool Control2WiFi::setHostname(char* name)
{
    handleEvents();

    gotResponse = false;
    responseType = NONE;

    if (!WiFiDrv::setHostname(name)) { // packet has not been sent. Maybe an interrupt occurred in the meantime
        // launch interrupt management function, then try to send request again
        uint32_t start = millis();
		while(millis() - start < 10)
			handleEvents();
        if (!WiFiDrv::setHostname(name)) // exit if another error occurs
            return false;
    }

    // Poll response for the next 10 seconds
    uint32_t start = millis();
    while (((millis() - start) < GENERAL_TIMEOUT)) {
        handleEvents();
        if (gotResponse && responseType == SET_HOSTNAME) {
            return (data[0]);
        }
    }
    return false;
}

// -----------------------------------------------------------------
uint8_t Control2WiFi::getSocket()
{
    for (uint8_t i = 0; i < MAX_SOCK_NUM; ++i) {
        if (Control2WiFi::server_port[i] == 0) {
            return i;
        }
    }
    return NO_SOCKET_AVAIL;
}

// -----------------------------------------------------------------
char* Control2WiFi::firmwareVersion()
{
    handleEvents();

    gotResponse = false;
    responseType = NONE;

    if (!WiFiDrv::getFwVersion()) { // packet has not been sent. Maybe an interrupt occurred in the meantime
        // launch interrupt management function, then try to send request again
        uint32_t start = millis();
		while(millis() - start < 10)
			handleEvents();
        if (!WiFiDrv::getFwVersion()) // exit if another error occurs
            return NULL;
    }

    // Poll response for the next 10 seconds
    uint32_t start = millis();
    while (((millis() - start) < GENERAL_TIMEOUT)) {
        handleEvents();
        if (gotResponse && responseType == GET_FW_VERSION) {
            uint8_t len = data[0];
            memcpy(fwVersion, (void*)&data[1], len);
            return (char*)fwVersion;
        }
    }
    return NULL;
}

// -----------------------------------------------------------------
bool Control2WiFi::checkFirmwareVersion(String fw_required)
{
    String fw_version = firmwareVersion();
    int idx = fw_version.indexOf('.');
    int last_idx = fw_version.lastIndexOf('.');
    //fw_version as int value
    int fw_version_tmp = (fw_version.substring(0, idx) + fw_version.substring(idx + 1, last_idx) + fw_version.substring(last_idx + 1)).toInt();
    idx = fw_required.indexOf('.');
    last_idx = fw_required.lastIndexOf('.');
    //fw_required as int value
    int fw_required_tmp = (fw_required.substring(0, idx) + fw_required.substring(idx + 1, last_idx) + fw_required.substring(last_idx + 1)).toInt();
    if (fw_version_tmp >= fw_required_tmp)
        return true;
    else
        return false;
}

// -----------------------------------------------------------------
wl_status_t Control2WiFi::begin()
{
    uint8_t status = WL_CONNECT_FAILED;
    handleEvents();

    gotResponse = false;
    responseType = NONE;

    if (!WiFiDrv::wifiAutoConnect()) { // packet has not been sent. Maybe an interrupt occurred in the meantime
        // launch interrupt management function, then try to send request again
        uint32_t start = millis();
		while(millis() - start < 10)
			handleEvents();
        if (!WiFiDrv::wifiAutoConnect()) // exit if another error occurs
            return (wl_status_t)status;
    }

    // Poll response for the next 10 seconds. If nothing happens return WL_CONNECT_FAILED
    uint32_t start = millis();
    while (((millis() - start) < GENERAL_TIMEOUT)) {
        handleEvents();
        if (gotResponse && (responseType == CONNECT_OPEN_AP || responseType == CONNECT_SECURED_AP)) {
            status = data[0];
            break;
        }
    }

    return (wl_status_t)status;
}

// -----------------------------------------------------------------
wl_status_t Control2WiFi::begin(char* ssid)
{
    uint8_t status = WL_CONNECT_FAILED;
    handleEvents();

    gotResponse = false;
    responseType = NONE;

    if (!WiFiDrv::wifiSetNetwork(ssid, strlen(ssid))) { // packet has not been sent. Maybe an interrupt occurred in the meantime
        // launch interrupt management function, then try to send request again
        uint32_t start = millis();
		while(millis() - start < 10)
			handleEvents();
        if (!WiFiDrv::wifiSetNetwork(ssid, strlen(ssid))) // exit if another error occurs
            return (wl_status_t)status;
    }

    // Poll response for the next 10 seconds. If nothing happens return WL_CONNECT_FAILED
    uint32_t start = millis();
    while (((millis() - start) < GENERAL_TIMEOUT)) {
        handleEvents();
        if (gotResponse && responseType == CONNECT_OPEN_AP) {
            status = data[0];
            break;
        }
    }

    return (wl_status_t)status;
}

// -----------------------------------------------------------------
wl_status_t Control2WiFi::begin(char* ssid, uint8_t key_idx, const char* key)
{
    uint8_t status = WL_CONNECT_FAILED;
    handleEvents();

    gotResponse = false;
    responseType = NONE;

    if (!WiFiDrv::wifiSetKey(ssid, strlen(ssid), key_idx, key, strlen(key))) { // packet has not been sent. Maybe an interrupt occurred in the meantime
        // launch interrupt management function, then try to send request again
        uint32_t start = millis();
		while(millis() - start < 10)
			handleEvents();
        if (!WiFiDrv::wifiSetKey(ssid, strlen(ssid), key_idx, key, strlen(key))) // exit if another error occurs
            return (wl_status_t)status;
    }

    // Poll response for the next 10 seconds. If nothing happens return WL_CONNECT_FAILED
    uint32_t start = millis();
    while (((millis() - start) < GENERAL_TIMEOUT)) {
        handleEvents();
        if (gotResponse && responseType == SET_KEY) {
            status = data[0];
            break;
        }
    }

    return (wl_status_t)status;
}

// ----------------------------------------------------------------- ok
wl_status_t Control2WiFi::begin(char* ssid, const char* passphrase)
{
    uint8_t status = WL_CONNECT_FAILED;
    handleEvents();

    gotResponse = false;
    responseType = NONE;

    if (!WiFiDrv::wifiSetPassphrase(ssid, strlen(ssid), passphrase, strlen(passphrase))) { // packet has not been sent. Maybe an interrupt occurred in the meantime
        // launch interrupt management function, then try to send request again
        uint32_t start = millis();
		while(millis() - start < 10)
			handleEvents();
        if (!WiFiDrv::wifiSetPassphrase(ssid, strlen(ssid), passphrase, strlen(passphrase))) // exit if another error occurs
            return (wl_status_t)status;
    }

    // Poll response for the next 10 seconds. If nothing happens return WL_CONNECT_FAILED
    uint32_t start = millis();
    while (((millis() - start) < GENERAL_TIMEOUT)) {
        handleEvents();
        if (gotResponse && responseType == CONNECT_SECURED_AP) {
            status = data[0];
            break;
        }
    }

    return (wl_status_t)status;
}

bool Control2WiFi::config(IPAddress local_ip)
{
    handleEvents();

    gotResponse = false;
    responseType = NONE;

    if (!WiFiDrv::config(1, (uint32_t)local_ip, 0, 0)) { // packet has not been sent. Maybe an interrupt occurred in the meantime
        // launch interrupt management function, then try to send request again
        uint32_t start = millis();
		while(millis() - start < 10)
			handleEvents();
        if (!WiFiDrv::config(1, (uint32_t)local_ip, 0, 0)) // exit if another error occurs
            return 0;
    }

    // Poll response for the next 10 seconds
    uint32_t start = millis();
    while (((millis() - start) < GENERAL_TIMEOUT)) {
        handleEvents();
        if (gotResponse && responseType == SET_IP_CONFIG) {
            return data[1];
        }
    }

    return 0;
}

bool Control2WiFi::config(IPAddress local_ip, IPAddress dns_server)
{
    handleEvents();

    gotResponse = false;
    responseType = NONE;

    if (!WiFiDrv::config(1, (uint32_t)local_ip, 0, 0)) { // packet has not been sent. Maybe an interrupt occurred in the meantime
        // launch interrupt management function, then try to send request again
        uint32_t start = millis();
		while(millis() - start < 10)
			handleEvents();
        if (!WiFiDrv::config(1, (uint32_t)local_ip, 0, 0)) // exit if another error occurs
            return 0;
    }

    // Poll response for the next 10 seconds
    uint32_t start = millis();
    while (((millis() - start) < GENERAL_TIMEOUT)) {
        handleEvents();
        if (gotResponse && responseType == SET_IP_CONFIG) {
            if (data[1] == 1) {
                if (!WiFiDrv::setDNS(1, (uint32_t)dns_server, 0)) { // packet has not been sent. Maybe an interrupt occurred in the meantime
                    // launch interrupt management function, then try to send request again
                    handleEvents();
                    if (!WiFiDrv::setDNS(1, (uint32_t)dns_server, 0)) // exit if another error occurs
                        return 0;
                }
                // Poll response for the next 10 seconds
                uint32_t start = millis();
                while (((millis() - start) < GENERAL_TIMEOUT)) {
                    handleEvents();
                    if (gotResponse && responseType == SET_DNS_CONFIG) {
                        return data[1];
                    }
                }
            }
        }
    }

    return 0;
}

bool Control2WiFi::config(IPAddress local_ip, IPAddress dns_server, IPAddress gateway)
{
    handleEvents();

    gotResponse = false;
    responseType = NONE;

    if (!WiFiDrv::config(2, (uint32_t)local_ip, (uint32_t)gateway, 0)) { // packet has not been sent. Maybe an interrupt occurred in the meantime
        // launch interrupt management function, then try to send request again
        uint32_t start = millis();
		while(millis() - start < 10)
			handleEvents();
        if (!WiFiDrv::config(2, (uint32_t)local_ip, (uint32_t)gateway, 0)) // exit if another error occurs
            return 0;
    }

    // Poll response for the next 10 seconds
    uint32_t start = millis();
    while (((millis() - start) < GENERAL_TIMEOUT)) {
        handleEvents();
        if (gotResponse && responseType == SET_IP_CONFIG) {
            if (data[1] == 1) {
                if (!WiFiDrv::setDNS(1, (uint32_t)dns_server, 0)) { // packet has not been sent. Maybe an interrupt occurred in the meantime
                    // launch interrupt management function, then try to send request again
                    handleEvents();
                    if (!WiFiDrv::setDNS(1, (uint32_t)dns_server, 0)) // exit if another error occurs
                        return 0;
                }
                // Poll response for the next 10 seconds
                uint32_t start = millis();
                while (((millis() - start) < GENERAL_TIMEOUT)) {
                    handleEvents();
                    if (gotResponse && responseType == SET_DNS_CONFIG) {
                        return data[1];
                    }
                }
            }
        }
    }

    return 0;
}

bool Control2WiFi::config(IPAddress local_ip, IPAddress dns_server, IPAddress gateway, IPAddress subnet)
{
    handleEvents();

    gotResponse = false;
    responseType = NONE;

    if (!WiFiDrv::config(3, (uint32_t)local_ip, (uint32_t)gateway, (uint32_t)subnet)) { // packet has not been sent. Maybe an interrupt occurred in the meantime
        // launch interrupt management function, then try to send request again
        uint32_t start = millis();
		while(millis() - start < 10)
			handleEvents();
        if (!WiFiDrv::config(3, (uint32_t)local_ip, (uint32_t)gateway, (uint32_t)subnet)) // exit if another error occurs
            return 0;
    }

    // Poll response for the next 10 seconds
    uint32_t start = millis();
    while (((millis() - start) < GENERAL_TIMEOUT)) {
        handleEvents();
        if (gotResponse && responseType == SET_IP_CONFIG) {
            if (data[1] == 1) {
                if (!WiFiDrv::setDNS(1, (uint32_t)dns_server, 0)) { // packet has not been sent. Maybe an interrupt occurred in the meantime
                    // launch interrupt management function, then try to send request again
                    handleEvents();
                    if (!WiFiDrv::setDNS(1, (uint32_t)dns_server, 0)) // exit if another error occurs
                        return 0;
                }
                // Poll response for the next 10 seconds
                uint32_t start = millis();
                while (((millis() - start) < GENERAL_TIMEOUT)) {
                    handleEvents();
                    if (gotResponse && responseType == SET_DNS_CONFIG) {
                        return data[1];
                    }
                }
            }
        }
    }

    return 0;
}

bool Control2WiFi::setDNS(IPAddress dns_server1)
{
    handleEvents();

    gotResponse = false;
    responseType = NONE;

    if (!WiFiDrv::setDNS(1, (uint32_t)dns_server1, 0)) { // packet has not been sent. Maybe an interrupt occurred in the meantime
        // launch interrupt management function, then try to send request again
        uint32_t start = millis();
		while(millis() - start < 10)
			handleEvents();
        if (!WiFiDrv::setDNS(1, (uint32_t)dns_server1, 0)) // exit if another error occurs
            return 0;
    }
    // Poll response for the next 10 seconds
    uint32_t start = millis();
    while (((millis() - start) < GENERAL_TIMEOUT)) {
        handleEvents();
        if (gotResponse && responseType == SET_DNS_CONFIG) {
            return data[1];
        }
    }

    return 0;
}

bool Control2WiFi::setDNS(IPAddress dns_server1, IPAddress dns_server2)
{
    handleEvents();

    gotResponse = false;
    responseType = NONE;

    if (!WiFiDrv::setDNS(2, (uint32_t)dns_server1, (uint32_t)dns_server2)) { // packet has not been sent. Maybe an interrupt occurred in the meantime
        // launch interrupt management function, then try to send request again
        uint32_t start = millis();
		while(millis() - start < 10)
			handleEvents();
        if (!WiFiDrv::setDNS(2, (uint32_t)dns_server1, (uint32_t)dns_server2)) // exit if another error occurs
            return 0;
    }
    // Poll response for the next 10 seconds
    uint32_t start = millis();
    while (((millis() - start) < GENERAL_TIMEOUT)) {
        handleEvents();
        if (gotResponse && responseType == SET_DNS_CONFIG) {
            return data[1];
        }
    }

    return 0;
}

int Control2WiFi::disconnect()
{
    handleEvents();

    gotResponse = false;
    responseType = NONE;

    if (!WiFiDrv::disconnect()) { // packet has not been sent. Maybe an interrupt occurred in the meantime
        // launch interrupt management function, then try to send request again
        uint32_t start = millis();
		while(millis() - start < 10)
			handleEvents();
        if (!WiFiDrv::disconnect()) // exit if another error occurs
            return -1;
    }

    // Poll response for the next 10 seconds
    uint32_t start = millis();
    while (((millis() - start) < GENERAL_TIMEOUT)) {
        handleEvents();
        if (gotResponse && responseType == DISCONNECT) {
            return (int)data[0];
        }
    }

    return -1;
}

// ----------------------------------------------------------------- ok
void Control2WiFi::macAddress(uint8_t* mac)
{
    handleEvents();

    gotResponse = false;
    responseType = NONE;

    if (!WiFiDrv::getMacAddress()) { // packet has not been sent. Maybe an interrupt occurred in the meantime
        // launch interrupt management function, then try to send request again
        uint32_t start = millis();
		while(millis() - start < 10)
			handleEvents();
        if (!WiFiDrv::getMacAddress()) // exit if another error occurs
            return;
    }

    // Poll response for the next 10 seconds
    uint32_t start = millis();
    while (((millis() - start) < GENERAL_TIMEOUT)) {
        handleEvents();
        if (gotResponse && responseType == GET_MACADDR) {
            uint8_t len = data[0];
            memcpy(mac, (void*)&data[1], len);
            break;
        }
    }
}

// ----------------------------------------------------------------- ok
IPAddress Control2WiFi::localIP()
{
    handleEvents();
    uint8_t addr[4] = { 0 };

    gotResponse = false;
    responseType = NONE;

    if (!WiFiDrv::getNetworkData()) { // packet has not been sent. Maybe an interrupt occurred in the meantime
        // launch interrupt management function, then try to send request again
        uint32_t start = millis();
		while(millis() - start < 10)
			handleEvents();
        if (!WiFiDrv::getNetworkData()) // exit if another error occurs
            return IPAddress(addr);
    }

    // Poll response for the next 10 seconds
    uint32_t start = millis();
    while (((millis() - start) < GENERAL_TIMEOUT)) {
        handleEvents();
        if (gotResponse && responseType == GET_IPADDR) {
            // local ip is the first of the three parameters returned with GET_IPADDR command
            memcpy((uint8_t*)&addr, (uint8_t*)&data[1], data[0]);

            break;
        }
    }

    IPAddress ret(addr);

    return ret;
}

// -----------------------------------------------------------------
IPAddress Control2WiFi::subnetMask()
{
    uint8_t mask[4] = { 0 };
    handleEvents();

    gotResponse = false;
    responseType = NONE;

    if (!WiFiDrv::getNetworkData()) { // packet has not been sent. Maybe an interrupt occurred in the meantime
        // launch interrupt management function, then try to send request again
        uint32_t start = millis();
		while(millis() - start < 10)
			handleEvents();
        if (!WiFiDrv::getNetworkData()) // exit if another error occurs
            return IPAddress(mask);
    }

    // Poll response for the next 10 seconds
    uint32_t start = millis();
    while (((millis() - start) < GENERAL_TIMEOUT)) {
        handleEvents();
        if (gotResponse && responseType == GET_IPADDR) {
            // subnet mask is the second of the three parameters returned with GET_IPADDR command
            uint8_t firstDataLen = data[0];
            dataLen = data[firstDataLen + 1];
            memcpy((uint8_t*)&mask, (void*)&data[firstDataLen + 2], dataLen);
            break;
        }
    }

    IPAddress ret(mask);

    return ret;
}

// ----------------------------------------------------------------- ok
IPAddress Control2WiFi::gatewayIP()
{
    handleEvents();
    uint8_t gateway[4] = { 0 };

    gotResponse = false;
    responseType = NONE;

    if (!WiFiDrv::getNetworkData()) { // packet has not been sent. Maybe an interrupt occurred in the meantime
        // launch interrupt management function, then try to send request again
        uint32_t start = millis();
		while(millis() - start < 10)
			handleEvents();
        if (!WiFiDrv::getNetworkData()) // exit if another error occurs
            return IPAddress(gateway);
    }

    // Poll response for the next 10 seconds
    uint32_t start = millis();
    while (((millis() - start) < GENERAL_TIMEOUT)) {
        handleEvents();
        if (gotResponse && responseType == GET_IPADDR) {
            // gateway ip is the second of the three parameters returned with GET_IPADDR command
            // skip first two data in order to retrieve the third.
            uint8_t secondDataLen = data[0] + data[data[0] + 1];
            dataLen = data[secondDataLen + 1];
            memcpy((uint8_t*)&gateway, (void*)&data[secondDataLen + 2], dataLen);
            break;
        }
    }
    IPAddress ret(gateway);

    return ret;
}

// ----------------------------------------------------------------- ok
char* Control2WiFi::SSID()
{
    handleEvents();

    gotResponse = false;
    responseType = NONE;

    if (!WiFiDrv::getCurrentSSID()) { // packet has not been sent. Maybe an interrupt occurred in the meantime
        // launch interrupt management function, then try to send request again
        uint32_t start = millis();
		while(millis() - start < 10)
			handleEvents();
        if (!WiFiDrv::getCurrentSSID()) // exit if another error occurs
            return NULL;
    }

    // Poll response for the next 10 seconds
    uint32_t start = millis();
    while (((millis() - start) < GENERAL_TIMEOUT)) {
        handleEvents();
        if (gotResponse && responseType == GET_CURR_SSID) {
            uint8_t len = data[0];
            memset(WiFiDrv::ssid, 0, WL_SSID_MAX_LENGTH);
			if(len <= BUF_SIZE - 6){
	            memcpy(WiFiDrv::ssid, (void*)&data[1], len);
				return (char*)WiFiDrv::ssid;
			}
			else{ // SSID is greater than 1 packet. wait for another packet containing the rest of the SSID
				memcpy(WiFiDrv::ssid, (void*)&data[1], dataLen);
				uint8_t recvLen = dataLen;
				while(dataPkt.endReceived == false){
					gotResponse = false;
					responseType = NONE;
					start = millis();
					while(millis() - start < GENERAL_TIMEOUT){
						handleEvents();
						if(gotResponse && responseType == GET_CURR_SSID){
							memcpy(&WiFiDrv::ssid[recvLen-1], (void*)data, dataLen);
							recvLen += dataLen;
							break;
						}
					}
				}
				return (char*)WiFiDrv::ssid;
			}
        }
    }
    return NULL;
}

// ----------------------------------------------------------------- ok
bool Control2WiFi::BSSID(uint8_t* bssid)
{
    handleEvents();

    gotResponse = false;
    responseType = NONE;

    if (!WiFiDrv::getCurrentBSSID()) { // packet has not been sent. Maybe an interrupt occurred in the meantime
        // launch interrupt management function, then try to send request again
        uint32_t start = millis();
		while(millis() - start < 10)
			handleEvents();
        if (!WiFiDrv::getCurrentBSSID()) // exit if another error occurs
            return false;
    }

    // Poll response for the next 10 seconds
    uint32_t start = millis();
    while (((millis() - start) < GENERAL_TIMEOUT)) {
        handleEvents();
        if (gotResponse && responseType == GET_CURR_BSSID) {
            memcpy(bssid, (void*)data, WL_MAC_ADDR_LENGTH);
            return true;
        }
    }

    return false;
}

// ----------------------------------------------------------------- ok
int32_t Control2WiFi::RSSI()
{
    handleEvents();
    int32_t rssi = 0;

    gotResponse = false;
    responseType = NONE;

    if (!WiFiDrv::getCurrentRSSI()) { // packet has not been sent. Maybe an interrupt occurred in the meantime
        // launch interrupt management function, then try to send request again
        uint32_t start = millis();
		while(millis() - start < 10)
			handleEvents();
        if (!WiFiDrv::getCurrentRSSI()) // exit if another error occurs
            return rssi;
    }

    // Poll response for the next 10 seconds
    uint32_t start = millis();
    while (((millis() - start) < GENERAL_TIMEOUT)) {
        handleEvents();
        if (gotResponse && responseType == GET_CURR_RSSI) {
            // copy 4 consecutive byte in the response to get the resulting int32_t
            memcpy(&rssi, (void*)data, 4);
            break;
        }
    }

    return rssi;
}

// ----------------------------------------------------------------- ok
uint8_t Control2WiFi::encryptionType()
{
    handleEvents();
    uint8_t enc = 0;

    gotResponse = false;
    responseType = NONE;

    if (!WiFiDrv::getCurrentEncryptionType()) { // packet has not been sent. Maybe an interrupt occurred in the meantime
        // launch interrupt management function, then try to send request again
        uint32_t start = millis();
		while(millis() - start < 10)
			handleEvents();
        if (!WiFiDrv::getCurrentEncryptionType()) // exit if another error occurs
            return enc;
    }

    // Poll response for the next 10 seconds
    uint32_t start = millis();
    while (((millis() - start) < GENERAL_TIMEOUT)) {
        handleEvents();
        if (gotResponse && responseType == GET_CURR_ENCT) {
            enc = *data;
            break;
        }
    }

    return enc;
}

// ----------------------------------------------------------------- ok
int8_t Control2WiFi::scanNetworks()
{
    handleEvents();
    uint8_t networksNumber = 0;

    gotResponse = false;
    responseType = NONE;

    if (!WiFiDrv::startScanNetworks()) { // packet has not been sent. Maybe an interrupt occurred in the meantime
        // launch interrupt management function, then try to send request again
        uint32_t start = millis();
		while(millis() - start < 10)
			handleEvents();
        if (!WiFiDrv::startScanNetworks()) // exit if another error occurs
            return networksNumber;
    }

    // Poll response for the next 10 seconds. If nothing happens return
    uint32_t start = millis();
    while (((millis() - start) < GENERAL_TIMEOUT)) {
        handleEvents();
        if (gotResponse && responseType == START_SCAN_NETWORKS) {
            networksNumber = *data;
            break;
        }
    }

    return networksNumber;
}

// ----------------------------------------------------------------- ok
uint8_t Control2WiFi::getScannedNetwork(uint8_t netNum, char* ssid, int32_t& rssi, uint8_t& enc)
{
    handleEvents();

    gotResponse = false;
    responseType = NONE;

    if (!WiFiDrv::getScanNetworks(netNum)) { // packet has not been sent. Maybe an interrupt occurred in the meantime
        // launch interrupt management function, then try to send request again
        uint32_t start = millis();
		while(millis() - start < 10)
			handleEvents();
        if (!WiFiDrv::getScanNetworks(netNum)) // exit if another error occurs
            return 0;
    }

    // Poll response for the next 10 seconds. If nothing happens return
    uint32_t start = millis();
    while (((millis() - start) < GENERAL_TIMEOUT)) {
        handleEvents();
        if (gotResponse && responseType == SCAN_NETWORKS_RESULT) {
            uint16_t receivedBytes;
            uint8_t chunkSize = 0;
            int32_t totalLen = Control2WiFi::dataLen; //payload size
            uint8_t skipSize = 14;

            if (totalLen < 26) {
                receivedBytes = totalLen;
            } else {
                receivedBytes = 26;
                skipSize = 13;
            }

            uint8_t len = data[6];
            uint8_t cpyLen = BUF_SIZE - skipSize;
            uint8_t which = data[0];

            rssi = (int32_t)(data[1] + (data[2] << 8) + (data[3] << 16) + (data[4] << 24));
            enc = data[5];
            memset(ssid, 0, 33);
            if (totalLen < (BUF_SIZE - skipSize + 7))
                cpyLen = len;

            memcpy(ssid, (uint8_t*)(data + 7), cpyLen);

            if (len > BUF_SIZE - skipSize) {
                start = millis();
                gotResponse = false;
                responseType = NONE;
                while (((millis() - start) < GENERAL_TIMEOUT)) {
                    handleEvents();
                    if (gotResponse && responseType == SCAN_NETWORKS_RESULT) {
                        memcpy((uint8_t*)(ssid + cpyLen), (uint8_t*)(data), dataLen);
                        return 1;
                    }
                }
            } else
                return 1;
        }
    }
    return 0;
}

char* Control2WiFi::SSID(uint8_t networkItem)
{
    return WiFiDrv::getSSIDNetoworks(networkItem);
}

int32_t Control2WiFi::RSSI(uint8_t networkItem)
{
    handleEvents();
    int32_t rssi = 0;

    gotResponse = false;
    responseType = NONE;

    if (!WiFiDrv::getRSSINetoworks(networkItem)) { // packet has not been sent. Maybe an interrupt occurred in the meantime
        // launch interrupt management function, then try to send request again
        uint32_t start = millis();
		while(millis() - start < 10)
			handleEvents();
        if (!WiFiDrv::getRSSINetoworks(networkItem)) // exit if another error occurs
            return rssi;
    }

    // Poll response for the next 10 seconds
    uint32_t start = millis();
    while (((millis() - start) < GENERAL_TIMEOUT)) {
        handleEvents();
        if (gotResponse && responseType == GET_IDX_RSSI) {
            // copy 4 consecutive byte in the response to get the resulting int32_t
            memcpy(&rssi, (void*)data, 4);
            break;
        }
    }

    return rssi;
}

uint8_t Control2WiFi::encryptionType(uint8_t networkItem)
{
    handleEvents();
    uint8_t enc = ENC_TYPE_UNKNOWN;

    gotResponse = false;
    responseType = NONE;

    if (!WiFiDrv::getEncTypeNetowrks(networkItem)) { // packet has not been sent. Maybe an interrupt occurred in the meantime
        // launch interrupt management function, then try to send request again
        uint32_t start = millis();
		while(millis() - start < 10)
			handleEvents();
        if (!WiFiDrv::getEncTypeNetowrks(networkItem)) // exit if another error occurs
            return enc;
    }

    // Poll response for the next 10 seconds
    uint32_t start = millis();
    while (((millis() - start) < GENERAL_TIMEOUT)) {
        handleEvents();
        if (gotResponse && responseType == GET_IDX_ENCT) {
            enc = *data;
            break;
        }
    }
    return enc;
}

wl_status_t Control2WiFi::status()
{
    gotResponse = false;
    responseType = NONE;

    handleEvents();

    if (connectionStatus == WL_NO_WIFI_MODULE_COMM) {
        if (!WiFiDrv::getConnectionStatus()) { // packet has not been sent. Maybe an interrupt occurred in the meantime
            // launch interrupt management function, then try to send request again
            uint32_t start = millis();
			while(millis() - start < 10)
				handleEvents();
            if (!WiFiDrv::getConnectionStatus()) // exit if another error occurs
                return (wl_status_t)connectionStatus;
        }

        uint32_t start = millis();
        while (((millis() - start) < GENERAL_TIMEOUT)) {
            handleEvents();
            if (gotResponse && responseType == GET_CONN_STATUS) {
                connectionStatus = (wl_status_t)(*data);
                return connectionStatus;
            }
        }
    }

    return connectionStatus;
}

/*
*
*/
int Control2WiFi::hostByName(const char* aHostname, IPAddress& aResult)
{
    uint8_t _ipAddr[WL_IPV4_LENGTH];
    IPAddress dummy(0xFF, 0xFF, 0xFF, 0xFF);
    int result = 0;
    handleEvents();

    gotResponse = false;
    responseType = NONE;

    if (!WiFiDrv::getHostByName(aHostname)) { // packet has not been sent. Maybe an interrupt occurred in the meantime
        // launch interrupt management function, then try to send request again
        uint32_t start = millis();
		while(millis() - start < 10)
			handleEvents();
        if (!WiFiDrv::getHostByName(aHostname)) // exit if another error occurs
            return result;
    }

    // Poll response for the next 10 seconds. If nothing happens return 0
    uint32_t start = millis();
    while (((millis() - start) < GENERAL_TIMEOUT)) {
        handleEvents();
        if (gotResponse && responseType == GET_HOST_BY_NAME) {
            if (dataLen == 1) // we got an error, return
                return 0;

            memcpy((void*)_ipAddr, (void*)data, dataLen);
            aResult = _ipAddr;
            result = (aResult != dummy);
            break;
        }
    }

    return result;
}


void Control2WiFi::cancelNetworkListMem()
{
    WiFiDrv::wifiCancelNetList();
}

Control2WiFi WiFi;