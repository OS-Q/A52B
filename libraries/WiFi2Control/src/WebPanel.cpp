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


#include "WebPanel.h"
#include "WiFi2Control.h"
#include <ESPmDNS.h>
#include "FFat.h"
#include "SPIFFS.h"


WebServer server(80);    //server UI
String HOSTNAME = DEF_HOSTNAME;

String newSSID_param;
String newPASSWORD_param;
const char* newSSID_par = "";
const char* newPASSWORD_par = "";
bool SERVER_STOP = false;       //check stop server

int tot = 0;
String staticIP_param ;
String netmask_param;
String gateway_param;
String dhcp = "on";
bool connect_wifi = false;
fs::FS* storage;

String getContentType(String filename){
  if(server.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  else if(filename.endsWith(".svg")) return "image/svg+xml";
  return "text/plain";
}

bool handleFileRead(String path){
  bool ret = false;
  if(path.endsWith("/")) path += "index.html";
  String contentType = getContentType(path);
  File file = storage->open(path, "r");
  if(!file.isDirectory()){
	size_t sent = server.streamFile(file, contentType);
	ret = true;
  }
  file.close();
  return ret;
}

String toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}

String toStringWifiMode(int mod) {
  String mode;
  switch (mod) {
    case 0:
      mode = "OFF";
      break;
    case 1:
      mode = "STA";
      break;
    case 2:
      mode = "AP";
      break;
    case 3:
      mode = "AP+STA";
      break;
    case 4:
      mode = "----";
      break;
    default:
      break;
  }
  return mode;
}

WiFiMode_t intToWifiMode(int mod) {
  WiFiMode_t mode;
  switch (mod) {
    case 0:
      mode = WIFI_OFF;
      break;
    case 1:
      mode = WIFI_STA;
      break;
    case 2:
      mode = WIFI_AP;
      break;
    case 3:
      mode = WIFI_AP_STA;
      break;
    case 4:
      break;
    default:
      break;
  }
  return mode;
}

String toStringWifiStatus(int state) {
  String status;
  switch (state) {
    case 0:
      status = "connecting";
      break;
    case 1:
      status = "unknown status";
      break;
    case 2:
      status = "wifi scan completed";
      break;
    case 3:
      status = "got IP address";
      // statements
      break;
    case 4:
      status = "connection failed";
      break;
    default:
      break;
  }
  return status;
}

String toStringEncryptionType(int thisType) {
  String eType;
  switch (thisType) {
    case WIFI_AUTH_WEP:
      eType = "WEP";
      break;
    case WIFI_AUTH_WPA_PSK:
      eType = "WPA";
      break;
    case WIFI_AUTH_WPA2_PSK:
      eType = "WPA2";
      break;
    case WIFI_AUTH_WPA_WPA2_PSK:
      eType = "WPA_WPA2";
      break;
    case WIFI_AUTH_WPA2_ENTERPRISE:
      eType = "WPA2 ENTERPRISE";
    case WIFI_AUTH_OPEN:
      eType = "None";
      break;
  }
  return eType;
}

IPAddress stringToIP(String address) {
  int p1 = address.indexOf('.'), p2 = address.indexOf('.', p1 + 1), p3 = address.indexOf('.', p2 + 1); //, 4p = address.indexOf(3p+1);
  String ip1 = address.substring(0, p1), ip2 = address.substring(p1 + 1, p2), ip3 = address.substring(p2 + 1, p3), ip4 = address.substring(p3 + 1);

  return IPAddress(ip1.toInt(), ip2.toInt(), ip3.toInt(), ip4.toInt());
}

// WebPanelClass
WebPanelClass::WebPanelClass(){}

void WebPanelClass::poll(){
  if(connect_wifi){
    wifi2control.connectedIndex_temp = -1;
    wifi2control.connectionTimeout = millis() + CONNECTION_TIMEOUT;
    wifi2control.connectionStatus = WL_IDLE_STATUS;

    WiFi.begin(newSSID_param.c_str(),newPASSWORD_param.c_str());
    memset(wifi2control.tempNetworkParam.strSSID, 0, 33);
    memset(wifi2control.tempNetworkParam.strPWD, 0, 65);
    strcpy(wifi2control.tempNetworkParam.strSSID, newSSID_param.c_str());
    strcpy(wifi2control.tempNetworkParam.strPWD, newPASSWORD_param.c_str());

    connect_wifi = false;
  }
  if(wifi2control.UI_alert){			//stop UI SERVER
  /*
    if(!SERVER_STOP){
      server.stop();
      SERVER_STOP = true;
    }
    */
  }
  else{
    server.handleClient();
  }
}

void WebPanelClass::begin(){
  // try to open FFat partition
  if(!FFat.begin()){
	  // if FFat fails, try with SPIFFS
	  SPIFFS.begin();
	  storage = &SPIFFS;
  }
  else{
	  storage = &FFat;
  }
  
  //init hostname
  //retrieve user defined hostname
  String tmpHostname = Config.getParam("hostname");
  if( tmpHostname!="" )
    HOSTNAME = tmpHostname;
  WiFi.setHostname(HOSTNAME.c_str());

  MDNS.begin(HOSTNAME.c_str());

  // scan networks
  tot = WiFi.scanNetworks();

  //"wifi/info" information
  server.on("/wifi/info", []() {
    String ipadd = (WiFi.getMode() == 1 || WiFi.getMode() == 3) ? toStringIp(WiFi.localIP()) : toStringIp(WiFi.softAPIP());
    String staticadd = dhcp.equals("on") ? "0.0.0.0" : staticIP_param;
    int change = WiFi.getMode() == 1 ? 3 : 1;
    String cur_ssid = (WiFi.getMode() == 2 )? "none" : WiFi.SSID();

     server.send(200, "text/plain", String("{\"ssid\":\"" + cur_ssid + "\",\"hostname\":\"" + WiFi.getHostname() + "\",\"ip\":\"" + ipadd + "\",\"mode\":\"" + toStringWifiMode(WiFi.getMode()) + "\",\"chan\":\""
                                            + WiFi.channel() + "\",\"status\":\"" + toStringWifiStatus(WiFi.status()) + "\", \"gateway\":\"" + toStringIp(WiFi.gatewayIP()) + "\", \"netmask\":\"" + toStringIp(WiFi.subnetMask()) + "\",\"rssi\":\""
                                            + WiFi.RSSI() + "\",\"mac\":\"" + WiFi.macAddress() + "\", \"dhcp\": \"" + dhcp + "\", \"staticip\":\"" + staticadd +
                                            + "\", \"warn\": \"" + "<a href='#' class='pure-button button-primary button-larger-margin' onclick='changeWifiMode(" + change + ")'>Switch to " + toStringWifiMode(change) + " mode</a>\""
                                            + "}" ));
    });

 
    //"system/info" information
    server.on("/system/info", []() {
      server.send(200, "text/plain", String("{\"heap\":\""+ String(ESP.getFreeHeap()/1024)+" KB\",\"revision\":\"" + String(ESP.getChipRevision()) + "\",\"size\":\"" + (ESP.getFlashChipSize() / 1024 / 1024) + " MB\",\"baud\":\"9600\"}"));
    });

    server.on("/heap", []() {
      server.send(200, "text/plain", String(ESP.getFreeHeap()));
    });

    server.on("/system/update", []() {
      String newhostname = server.arg("name");
      WiFi.setHostname(newhostname.c_str());
      MDNS.begin(newhostname.c_str());
      MDNS.setInstanceName(newhostname);
      server.send(200, "text/plain", newhostname);
      Config.setParam("hostname", newhostname);
    });

    server.on("/wifi/netNumber", []() {
        tot = WiFi.scanNetworks();
        server.send(200, "text/plain", String(tot));
    });

    server.on("/wifi/scan", []() {
      tot = WiFi.scanNetworks();
      String scanResp = "";
      if (tot == 0) {
        server.send(200, "text/plain", "No networks found");
        #ifdef DEBUG_SERIALE
        Serial.println("No networks found");
        #endif
      }
      if (tot == -1 ) {
        server.send(500, "text/plain", "Error during scanning");
        #ifdef DEBUG_SERIALE
        Serial.println("Error during scanning");
        #endif
      }

      String tSSID;

      #ifdef DEBUG_SERIALE
      //Serial.println(tot);
      #endif

      scanResp += "{\"result\": { \"APs\" : [ ";
      //ETS_SPI_INTR_DISABLE();
      for (int8_t netIndex = 0; netIndex < tot; netIndex++) { 
        scanResp += "{\"enc\" : \"";
        scanResp += toStringEncryptionType (WiFi.encryptionType(netIndex));
        scanResp += "\",";
        scanResp += "\"essid\":\"";
        //scanResp += WiFi.SSID(netIndex);
        
        tSSID = WiFi.SSID(netIndex);
        scanResp += tSSID;

        #ifdef DEBUG_SERIALE
        //Serial.println(tSSID);
        #endif

        scanResp += "\",";
        scanResp += "\"rssi\" :\"";
        scanResp += String((WiFi.RSSI(netIndex)));
        scanResp += "\"}";
        if (netIndex != tot - 1)
          scanResp += ",";
      } 
      scanResp += "]}}";

      //ETS_SPI_INTR_ENABLE();
      server.send(200, "text/plain", scanResp);
    });

    server.on("/connect", []() {
      newSSID_param = server.arg("essid");
      newPASSWORD_param = server.arg("passwd");
      server.send(200, "text/plain", "1");
      //WiFi.begin(newSSID_param.c_str(),newPASSWORD_param.c_str());
      Config.setParam("ssid", newSSID_param);
      Config.setParam("password", newPASSWORD_param);
      connect_wifi = true;
    });

    server.on("/connstatus", []() {
        String ipadd = (WiFi.getMode() == 1 || WiFi.getMode() == 3) ? toStringIp(WiFi.localIP()) : toStringIp(WiFi.softAPIP());
        server.send(200, "text/plain", String("{\"url\":\"got IP address\", \"ip\":\""+ipadd+"\", \"modechange\":\"no\", \"ssid\":\""+WiFi.SSID()+"\", \"reason\":\"-\", \"status\":\""+ toStringWifiStatus(WiFi.status()) +"\"}"));
    });


    server.on("/setmode", []() {
      int newMode = server.arg("mode").toInt();

      switch (newMode){
        case 1 :
        case 3 :
          server.send(200, "text/plain", String("Mode changed " + toStringWifiMode(WiFi.getMode())));
          WiFi.mode(intToWifiMode(newMode));
          break;
        case 2 :
          server.send(200, "text/plain", String("Mode changed " + toStringWifiMode(WiFi.getMode())));
          WiFi.mode(WIFI_AP);
          break;
      }

    });

    server.on("/special", []() {
       dhcp = server.arg("dhcp");
       staticIP_param = server.arg("staticip");
       netmask_param = server.arg("netmask");
       gateway_param = server.arg("gateway");

       if (dhcp == "off") {
         server.send(200, "text/plain", String("{\"url\":\"" + staticIP_param + "\"}"));
         delay(1000);
         WiFi.config(stringToIP(staticIP_param), stringToIP(gateway_param), stringToIP(netmask_param));
       }
       else {
        server.send(200, "text/plain",  "1");
        // wait for the response to reach the server
        delay(1000);
        WiFi.disconnect();

        wifi2control.connectionTimeout = millis() + CONNECTION_TIMEOUT;
        wifi2control.connectionStatus = WL_IDLE_STATUS;

        IPAddress resetIP(0, 0, 0, 0);
        WiFi.config(resetIP, resetIP, resetIP);
        wifi2control.autoconnect();
       }
     });

     server.on("/boardInfo", []() {
        StaticJsonBuffer<200> jsonBuffer;
        JsonObject& boardInfo = jsonBuffer.createObject();
        String output = "";
         
        boardInfo["fw_name"] = FW_NAME;
        boardInfo["fw_version"] = FW_VERSION;
        boardInfo["build_date"] = BUILD_DATE;
         
        boardInfo.printTo(output);
        server.send(200, "text/json", output);
      });

    //called when the url is not defined here
    //use it to load content from storage
    server.onNotFound([](){
	  if(!handleFileRead(server.uri()))
        server.send(404, "text/plain", "FileNotFound");
    });

    server.begin();

}

WebPanelClass webPanel;


// Configuration class
bool Configuration::setParam(String param, String value){
  if(value.length()>1 && param.length()>1){
    String paramConfig = "";
    DynamicJsonBuffer jsonBuffer;
    File configFile = storage->open(CONFIGFILENAME,"r");
    if(configFile){     //read Config file
      while(configFile.available()){
        paramConfig = paramConfig + char(configFile.read());
      }
    }
    else{
      paramConfig = JSONEMPTY;
    }
    configFile.close();
    JsonObject& json = jsonBuffer.parseObject(paramConfig);
    //write param to config file
    configFile = storage->open(CONFIGFILENAME, "w");
    json[param] = value;
    json.printTo(configFile);
    configFile.close();
    return true;
  }
  else{
    return false;
  }
}

String Configuration::getParam(String param){
  if(param.length()>1){
    File configFile = storage->open(CONFIGFILENAME, "r");
    if (!configFile) {
      return EMPTY;
    }
    size_t size = configFile.size();
    std::unique_ptr<char[]> buf(new char[size]);
    configFile.readBytes(buf.get(), size);

    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.parseObject(buf.get());

    if (!json.success()) {
      return EMPTY;
    }
    configFile.close();
    return json[param];
  }
  else{
    return EMPTY;
  }
}

Configuration Config;