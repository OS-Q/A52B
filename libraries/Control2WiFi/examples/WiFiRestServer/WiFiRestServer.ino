/* WiFiRestServer.ino
 *
 * This example shows how to access digital and analog pins
 * of the board through REST calls. It demonstrates how you
 * can create your own API when using REST style calls through
 * the browser.
 *
 * Possible commands created in this shetch:
 *
 * "/briki/digital/9"     -> digitalRead(9)
 * "/briki/digital/9/1"   -> digitalWrite(9, HIGH)
 * "/briki/analog/2/123"  -> analogWrite(2, 123)
 * "/briki/analog/2"      -> analogRead(2)
 * "/briki/mode/9/input"  -> pinMode(9, INPUT)
 * "/briki/mode/9/output" -> pinMode(9, OUTPUT)
 *
 * Connect to the board's web panel through a browser (type the
 * board's ip if the board is already connected to a network or
 * connect your computer to the board's SSID and type the default
 * address 192.168.240.1 in order to access the web panel). Open
 * the tab "Wi-Fi Pin Control" to send commands to the board by
 * pressing the related buttons.
 * Alternatively you can directly send the REST command by typing
 * on a browser the board's address and the desired command.
 * e.g. http://yourIpAddress:8080/briki/digital/9/1
 */


#include "Control2WiFi.h"

// server will listen on port 8080
WiFiServer server(8080);
WiFiClient client;


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
    Serial.println("WiFi module linked!");

    Serial.println("Attempting to connect to a stored network");

	/*
       In this case we are trying to connect to a stored network.
	   Look at ConnectToASavedNetwork example to have further information.
    */
    if (WiFi.begin() != WL_CONNECTED) {

      if (WiFi.status() != WL_CONNECTED) {
		/*
           if the connection fails due to an error...
        */
        Serial.println("Connection error! No network memorized or match found.");
      }
    }
    server.begin();
  }


}

void loop() {
  // listen for incoming clients
  WiFiClient client = server.available();
  if (client) {
    // Process request
    process(client);
    // Close connection and free resources.
    delay(1);
    client.stop();
  }

  /*
     In the case of a connect event, notify it to the user.
  */
  if (WiFi.connectionStatus == WL_CONNECTED && WiFi.notify == true) {
    WiFi.notify = false;
    Serial.print("You're connected to the network ");
    printWifiStatus();
  }
  /*
     In the case of a disconnect event, notify it to the user.
  */
  else if (WiFi.connectionStatus == WL_DISCONNECTED && WiFi.notify == true) {
    WiFi.notify = false;
    Serial.println("You've been disconnected");
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

bool listen_service(WiFiClient client, String service)
{
  //check service
  String currentLine = "";
  while (client.connected()) {
    if (client.available() > 0) {
      char c = client.read();
      currentLine += c;
      if (c == '\n') {
        client.println("HTTP/1.1 200 OK");
		client.println("Connection: close");
        client.println();
        return 0;
      }
      else if (currentLine.endsWith(service + "/")) {
        return 1;
      }
    }
  }
}

// -----------------------------------------------------------------
void digitalCommand(WiFiClient client) {
  int pin, value;

  // Read pin number
  String pinNumber;
  char c = client.read();
  // If the next character is a '/' it means we have an URL
  // with a value like: "/digital/9/1"
  // If the next character is a ' ' it means we have an URL
  // with a value like: "/digital/9"

  while (c != ' ' && c != '/') {
    pinNumber += c;
    c = client.read();
  }
  pin = pinNumber.toInt();

  if (c == '/') {
    value = client.parseInt();
    //value = 1;
    digitalWrite(pin, value);

    // Send feedback to client
    client.print("HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\n\r\nPin D" + String(pin) + " set to: " + String(value));
  }
  else {
    value = digitalRead(pin);

    // Send feedback to client
    client.print("HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\n\r\nPin D" + String(pin) + " reads: " + String(value));
  }
}

// -----------------------------------------------------------------
void analogCommand(WiFiClient client) {
  int pin, value, truePin;
  bool analog = false;
  char pinType = 'D';
  // Read pin number
  String pinNumber;

  char c = client.read(); 
  if(c == 'A'){
    analog = true;
    pinType = 'A';
    c = client.read();
  }

  // If the next character is a '/' it means we have an URL
  // with a value like: "/analog/5/120"
  // If the next character is a ' ' it means we have an URL
  // with a value like: "/analog/13"
  while (c != ' ' && c != '/') {
    pinNumber += c;
    c = client.read();
  }

  pin = pinNumber.toInt();
  
  truePin = pin;
    
  if(analog){
    switch(pin){
      case 0:
        truePin = A0;
      break;
  
      case 1:
        truePin = A1;
      break;
  
      case 2:
        truePin = A2;
      break;
      
      case 3:
        truePin = A3;
      break;
  
      case 4:
        truePin = A4;
      break;
  
      case 5:
        truePin = A5;
      break;
    }
  } 
   
  // If the next character is a '/' it means we have an URL
  // with a value like: "/analog/5/120"
  if (c == '/') {
    // Read value and execute command
    String analogValue;
    char c = client.read();
    while (c != ' ' && c != '/') {
      analogValue += c;
      c = client.read();
    }
    value = analogValue.toInt();
    analogWrite(truePin, value);

    // Send feedback to client
    client.print("HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\n\r\nPin " + String(pinType) + String(pin) + " set to: " + String(value));
  }
  else {
    // Read analog pin
    value = analogRead(truePin);

    // Send feedback to client
    client.print("HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\n\r\nPin " + String(pinType) + String(pin) + " reads: " + String(value));
  }
}

// -----------------------------------------------------------------
void modeCommand(WiFiClient client) {
  int pin, truePin;
  char feedback[100];

  // Read pin number
  String pinNumber;
  bool analog = false;
  char pinType = 'D';
  char c = client.read();
  if(c == 'A'){
    analog = true;
    pinType = 'A';
    c = client.read();
  }
  while (c != ' ' && c != '/') {
    pinNumber += c;
    c = client.read();
  }
  pin = pinNumber.toInt();

  truePin = pin;

  if(analog){
    switch(pin){
      case 0:
        truePin = A0;
      break;
  
      case 1:
        truePin = A1;
      break;
  
      case 2:
        truePin = A2;
      break;
      
      case 3:
        truePin = A3;
      break;
  
      case 4:
        truePin = A4;
      break;
  
      case 5:
        truePin = A5;
      break;
    }
  }
  // If the next character is not a '/' we have a malformed URL
  if (c != '/') {
    client.print("HTTP/1.1 200 OK\r\n\r\nError!\r\n\r\n");

    return;
  }

  String mode = client.readStringUntil(' ');

  if (mode == "input") {
    pinMode(truePin, INPUT);

    // Send feedback to client
    client.print("HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\n\r\nPin " + String(pinType) + String(pin) + " configured as INPUT!");

    return;
  }

  if (mode == "output") {
    pinMode(truePin, OUTPUT);

    // Send feedback to client
    client.print("HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\n\r\nPin " + String(pinType) + String(pin) + " configured as OUTPUT!\r\n");
    return;
  }

  client.print(String("HTTP/1.1 200 OK\r\n\r\nError!\r\nInvalid mode" + mode + "\r\n"));
}

// -----------------------------------------------------------------
void process(WiFiClient client)
{
  // read the command//
  if (listen_service(client, "briki")) {
    String command = client.readStringUntil('/');

    if (command == "mode") {
      modeCommand(client);
    }
    else if (command == "digital") {
      digitalCommand(client);
    }
    else if (command == "analog") {
      analogCommand(client);
    }
    else {
      client.print(String("HTTP/1.1 200 OK\r\n\r\nError!\r\nUnknown command : " + command + "\r\n\r\n"));
    }
  }
}
