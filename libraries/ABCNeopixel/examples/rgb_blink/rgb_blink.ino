#include "ABCNeopixel.h"

ws2812_rgb rgb(1, LED_BUILTIN);

void setup() {
  // put your setup code here, to run once:
  rgb.begin();
  rgb.setBrightness(50);
  rgb.setColor(0, rgb.Color(140, 50, 2));
}

void loop() {
  rgb.clear();
  delay(100);
  rgb.setColor(0, rgb.Color(140, 50, 2));
  rgb.show();
  delay(100);
}
