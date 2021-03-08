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

enum {
  // WiFi base
  COMPANION_READY     = 0x10,
  CONNECT_OPEN_AP     = 0x11,
  CONNECT_SECURED_AP  = 0x12,
  SET_KEY             = 0x13,
  SET_IP_CONFIG       = 0x14,
  SET_DNS_CONFIG      = 0x15,
  GET_HOSTNAME        = 0x16,
  SET_HOSTNAME        = 0x17,
  AUTOCONNECT_TO_STA  = 0x18,
  CANCEL_NETWORK_LIST = 0x19,
  GET_CONN_STATUS     = 0x1A,
  GET_IPADDR          = 0x1B,
  GET_MACADDR         = 0x1C,
  GET_CURR_SSID       = 0x1D,
  GET_CURR_BSSID      = 0x1E,
  GET_CURR_RSSI       = 0x1F,
  GET_CURR_ENCT       = 0x20,
  SCAN_NETWORK_RESULT = 0x21,
  DISCONNECT          = 0x22,
  GET_IDX_SSID        = 0x23,
  GET_IDX_RSSI        = 0x24,
  GET_IDX_ENCT        = 0x25,
  GET_HOST_BY_NAME    = 0x26,
  START_SCAN_NETWORKS = 0x27,
  GET_FW_VERSION      = 0x28,
  
  // server
  START_SERVER_TCP    = 0x30,
  
  // client
  DATA_SENT_TCP       = 0x32,
  AVAIL_DATA_TCP      = 0x33,
  GET_DATA_TCP        = 0x34,
  START_CLIENT_TCP    = 0x35,
  STOP_CLIENT_TCP     = 0x36,
  GET_CLIENT_STATE_TCP= 0x37,
  SEND_DATA_TCP       = 0x38,
  SEND_DATA_UDP       = 0x39,
  GET_REMOTE_DATA     = 0x3A,

  // All command with DATA_FLAG 0x40 send a 16bit Len
  
  // data
  //TEST_DATA_TXRX    = 0x40,
  GET_DATABUF_TCP     = 0x45,
  INSERT_DATABUF      = 0x46,
  
  // user defined commands start at 0x50
};
