/* ----------------------------------------------------------------------------
* Introduction:
* use RGB as builtin led
* Actuator: WS2812B
* http://www.peace-corp.co.jp/data/WS2812-2020_V1.0_EN.pdf
* 
* Compatibility:
* this sketch runs on Briki ABC - SAMD21
* 
* Description:
* Sketch shows how to use RGB LED as the classical Arduino LED BUILTIN.
* RGB management is based on a modified version of NeoPixel 
* library by Adafruit.
* Besides, two custom methods have been added: ledbuiltinSetColor(r, g, b)
* that allows to control LED color and ledBuiltinSetBrightness(br) to
* control brightness.
*
* Created: 11/06/2019 by Meteca
*/

void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  // set blue color
  ledBuiltinSetColor(0, 0, 150);
  // set brightness percentage
  ledBuiltinSetBrightness(75);
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on
  delay(1000);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off
  delay(1000);                       // wait for a second
}
