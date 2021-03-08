/* ----------------------------------------------------------------------------
* Introduction:
* Read microphone sensor connected to ABC
* Sensor: MP23ABS1
* https://www.st.com/resource/en/datasheet/mp23abs1.pdf
* 
* Compatibility: 
* this sketch can run on both SAMD21 and ESP32 thanks to 
* GPIO virtualization system.
* Microphone is directly connected to SAMD21.
* If this sketch is loaded on ESP32, it is advisible to run UartBridge 
* example on SAMD21 (File > Examples > MbcUtility > UartBridge) to see Serial output.
* 
* Description:
* Sketch allows to acquire acustic pressure values observed
* by the sensor, amplified by signal conditional chain.
* Microphone output is connected to Analog input 9 (A9).
* Data read are printed on USB.
* By using Serial Plotter (Tools > Serial Plotter menu)
* it is possible to visualize the signal wave shape.
*
* Created: 11/06/2019 by Meteca
*/

void setup() {
  // Serial initialization
  Serial.begin(115200);
  // Initialize A9 as input
  pinMode(A9, INPUT);
}

void loop() {
  // read microphone values and print them on serial monitor
  Serial.println(analogRead(A9));
  delay(1);
}
