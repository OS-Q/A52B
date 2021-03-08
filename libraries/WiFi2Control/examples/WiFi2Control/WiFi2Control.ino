/*
 * WiFi2Control.ino
 * 
 * Load this sketch on esp32 to start using Control2WiFi library on samd21 and access
 * WiFi functionalities through samd21 chip.
 * 
 * To modify board's parameters change config.h file in WiFiLink library.
 */

#include "WiFi2Control.h"
#include "WebPanel.h"

void setup() {
  // start WiFiLink and WebPanel
  wifi2control.begin();
  webPanel.begin();
  // connect to one of the previously connected networks (if any)
  wifi2control.autoconnect();
}

void loop() {
  // check and process out any events from web panel or wifi link library
  wifi2control.poll();
  webPanel.poll();
}
