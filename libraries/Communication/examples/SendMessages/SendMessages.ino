/* SendMessages.ino
 * 
 * This example for MBC-WB shows how to use communication class to 
 * exchange raw messages between the 2 microcontrollers onboard.
 * 
 * Load the same sketch on both ESP32 and SAMD21 and open a serial
 * monitor to see the incoming messages.
 * Try to change message sent to see different outputs.
 * 
 * Please note that serial monitor will show only messages print from
 * SAMD21. To see ESP32 output you can combine this sketch with UartBridge.ino
 * or modify the ESP sketch in order to print the message through WiFi.
 */

char msg[] = "A generic message\r\n";
char buff[100];

void setup() {
  Serial.begin(115200);
  // register a buffer where incoming messages will be placed
  // buffer can be an array of any type
  communication.initExternalBuffer(buff);
}

void loop() {
  // send a message - message can be an array of any type
  communication.sendGenericData(msg, sizeof(msg));

  // check for incoming messages
  uint32_t avail = communication.availGenericData();
  // if there are available data read and consume them
  if(avail){
    communication.getGenericData();
    // availGenericData returns the number of available data in bytes.
    // make sure to scale this number according to the data type we are using
    uint32_t receivedChar = avail / sizeof(char);
    Serial.write((uint8_t*)buff, receivedChar);
  }

  // wait before sending another message
  delay(1000);
}