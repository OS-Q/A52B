/* WiFiChatServer.ino
 * 
 * A simple server that distributes any incoming messages to all
 * connected clients. You can see the client's input in the serial monitor as well.
 *
 * Change ssid and password accordingly to your network. Once connected,
 * open a telnet client at the given ip address. The ip address
 * will show up on the Serial monitor after connection.
 */



#include <Control2WiFi.h>

// server will listen on port 23
WiFiServer server(23);

char ssid[] = "yourNetwork";     //  your network SSID (name)
char pass[] = "secretPassword";  // your network password


boolean alreadyConnected = false; // whether or not the client was connected previously


void setup() {
  Serial.begin(115200);
  
  WiFi.init();
  
  Serial.println("Checking WiFi module linkage...");

  if (WiFi.status() == WL_NO_WIFI_MODULE_COMM) {
  	/*
       notify the user if the previous sequence of operations to establish
       the communication between companion chip and the main mcu fail.
    */
    Serial.println("Communication with WiFi module not established.");
  }
  else {
    /*
       otherwise we have a correct communication between the two chips
       and we can connect to a preferred network
    */
    Serial.println("\nWiFi module linked!");
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);

    /*
       this is one of the way (suppling SSID and PWD) that can be used to
       establish a connection to a network - other ways are showed in other
       examples, please check them out.
    */
    if (WiFi.begin(ssid, pass) != WL_CONNECTED) {
      if (WiFi.status() != WL_CONNECTED) {
        /*
           if the connection fails due to an error...
        */
        Serial.println("Connection error! Check ssid and password and try again.");
      }
    }
    else {
      /*
         you're now connected, so print out the network data
      */
      Serial.println("You're connected to the network ");
      printWifiStatus();
    }
    server.begin();
  }
}

void loop() {
  /*
     Check and notify changes on connection status.
  */
  if (WiFi.connectionStatus == WL_CONNECTED && WiFi.notify == true) {
    WiFi.notify = false;
    Serial.print("You're connected to the network ");
    printWifiStatus();
  }
  else if (WiFi.connectionStatus == WL_DISCONNECTED && WiFi.notify == true) {
    WiFi.notify = false;
    Serial.println("You've been disconnected");
  }	
	
  // listen for clients
  WiFiClient client = server.available();


  // when the client sends the first byte, say hello:
  if (client) {
    if (!alreadyConnected) {
      // clear out the input buffer:
      client.flush();
      Serial.println("We have a new client");
      client.println("Hello, client!");
      alreadyConnected = true;
    }

    if (client.available() > 0) {
		  // read the whole message
		  while(client.available() > 0){
			  // read the bytes incoming from the client:
			  char thisChar = client.read();
			  // echo the bytes to the serial monitor:
			  Serial.write(thisChar);
		  }
		  // reply to the client
		  server.write("Message received!");
    }
  }
}

void printWifiStatus()
{
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
