/* ScanNetworks.ino
 *
 * This example shows how to scan nearby WiFi networks.
 *
 * Open the serial monitor to see the scanned networks with
 * related RSSI (signal strength) and encryption type.
 */

#include "Control2WiFi.h"

void setup() {
  Serial.begin(115200);

  WiFi.init();


  Serial.println("WiFi scan networks.");
  Serial.println();

  if (WiFi.status() == WL_NO_WIFI_MODULE_COMM) {
    /*
       notify the user if the previous sequence of operations to establish
       the communication between companion chip and the main mcu fail.
    */
    Serial.println("Communication with WiFi module not established.");
    while (1); // do not continue
  }
  else {
    /*
       otherwise we have a correct communication between the two chips
       and we can start scanning nearby networks
    */
    Serial.println("WiFi module linked!");
  }
}

void loop() {
  // start scanning networks
  scanNetworks();
}

void printEncryptionType(int thisType) {
	// read the encryption type and print out the name:
	switch (thisType) {
		case ENC_TYPE_WEP:
		Serial.println("WEP");
		break;
		case ENC_TYPE_WPA_PSK:
		Serial.println("WPA");
		break;
		case ENC_TYPE_WPA2_PSK:
		Serial.println("WPA2");
		break;
		case ENC_TYPE_NONE:
		Serial.println("None");
		break;
		case ENC_TYPE_WPA_WPA2_PSK:
		Serial.println("WPA_WPA2");
		break;
		case ENC_TYPE_WPA2_ENTERPRISE:
		Serial.println("WPA2 Enterprise");
		break;
		default:
		Serial.println("Unknown");
		break;
	}
}

void listNetworks()
{
  // scan for nearby networks:
  Serial.println("** Scan Networks **");
  int16_t numSsid = -1;


  numSsid = WiFi.scanNetworks();

  if (numSsid == 0) {
    Serial.println("Scanning Error");
    return;
  }

  // print the list of networks seen:
  Serial.print("number of available networks:");
  Serial.println(numSsid);

  // print the network number and name for each network found:
  for (int thisNet = 0; thisNet < numSsid; thisNet++) {

    char ssid[33];
    int32_t rssi;
    uint8_t enc;

    bool ret_code = WiFi.getScannedNetwork(thisNet, ssid, rssi, enc);
    if (ret_code) {
      Serial.print(thisNet + 1);
      Serial.print(") ");
      Serial.print(ssid);
      Serial.print("\tSignal: ");
      Serial.print(rssi);
      Serial.print(" dBm");
      Serial.print("\tEncryption: ");
      printEncryptionType(enc);
    }
    else
      Serial.println("Error while parsing data of network no. " + String(thisNet));
  }
}

void scanNetworks(void)
{
  // scan for existing networks:
  Serial.println("\nScanning available networks...");
  listNetworks();

  // wait 2 seconds before starting a new scan
  delay(2000);
}
