/* UartBridge.ino
 * 
 * This example for MBC-WB shows how to access serial output of esp32 
 * through samd21.
 * 
 * It listen for incoming data on the uart connected to the esp32 and
 * print them to the serial monitor.
 *
 * At the same way, all incoming data available from the serial monitor
 * will be sent to the uart connected to esp32.
 * 
 * If in esp32 there's an example that uses Serial class, you can see
 * the output in the serial monitor by loading this example in samd21.
 */

void setup() {
  Serial.begin(115200);
  // modify baud rate accordingly to the one chosen in the esp sketch
  SerialCompanion.begin(115200);
}

void loop() {
  // send all incoming messages from ESP to USB
  while(SerialCompanion.available())
    Serial.write(SerialCompanion.read());

  // send all incoming messages from USB to ESP
  while(Serial.available())
    SerialCompanion.write(Serial.read());
}
