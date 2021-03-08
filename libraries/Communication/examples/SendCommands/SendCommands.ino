/* SendCommands.ino
 * 
 * This example for MBC-WB shows how to use communication class to
 * exchange commands between the 2 microcontrollers onboard.
 * 
 * Load the same sketch on both ESP32 and SAMD21 and open a serial
 * monitor to see the incoming commands.
 * Try to change command content to see different outputs.
 * 
 * Both the MCUs should have the same command ID registered in order
 * for the command to be correctly received.
 * 
 * Please note that serial monitor will show only messages print from
 * SAMD21. To see ESP32 output you can combine this sketch with UartBridge.ino
 * or modify the ESP sketch in order to print the message through WiFi.
 */

#define COMMAND_ID 0x15

uint8_t commandData[50];

void setup() {
  Serial.begin(115200);
  
  // register command ID, a callback function called on each received command and a buffer where command data will be put
  communication.registerCommand(COMMAND_ID, dataReceivedCallback, commandData);
}

void loop() {
  // send a command to companion chip
  uint8_t command[] = {'H', 'E', 'L', 'L', 'O'};
  communication.sendCommand(COMMAND_ID, command, sizeof(command));

  // do other tasks...
  delay(1000);
  
  // manually poll communication events if our tasks require long computational time
  communication.handleCommEvents();
}

void dataReceivedCallback(uint32_t commandLen){
  // companion sent us a command
  Serial.println("received command from companion:");
  Serial.write(commandData, commandLen);
  Serial.println();

  // parse command and move GPIOs, read sensors, send data over WiFi ...
}
