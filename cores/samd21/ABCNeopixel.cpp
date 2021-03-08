#if defined (BRIKI_ABC) && defined (BRIKI_MBC_WB_SAMD)
#include "ABCNeopixel.h"

#ifdef __cplusplus
 extern "C" {
#endif
/* RGB LED object implementation 
* on the ABC there is a mini WS2812 RGB LED with embedded logic
*/
ws2812_rgb ledBuiltin(1, LED_BUILTIN);
boolean ledEnabled = false; // keep trace of the RGB LED status. If the LED is already begun than you can digitalWrite() it
boolean ledStatus = LOW;
// variables to store the current RGB coding and overall LED brightness
uint8_t rLed, gLed, bLed, bright;
#ifdef __cplusplus
}
#endif

// Object constructor
ws2812_rgb::ws2812_rgb(uint16_t n, uint8_t p) : begun(false), brightness(0), pixels(NULL), endTime(0)
{
    // setup LED configuration: GRB led sequence and update frequency
    uint16_t t = WS2812_GRB + WS2812_KHZ800;
    boolean newThreeBytesPerPixel = true;

    // R, G, B offsets in LED sequence.
    gOffset = 0;
    rOffset = 1;
    bOffset = 2;

    // If bytes-per-pixel has changed (and pixel data was previously
    // allocated), re-allocate to new size. Will clear any data.
    if(pixels) {
        boolean newThreeBytesPerPixel = true;
        if(newThreeBytesPerPixel != true) 
            length(numLEDs);
    }

    free(pixels); // Free existing data (if any)

    // Allocate new data -- note: ALL PIXELS ARE CLEARED
    numBytes = n * 3;
    if((pixels = (uint8_t *)malloc(numBytes))) {
        memset(pixels, 0, numBytes);
        numLEDs = n;
    } 
    else {
        numLEDs = numBytes = 0;
    }
    pin = p;
    rLed = 120;
    gLed = 40;
    bLed = 0;
    bright = 75;
}

/*!
  @brief   Deallocate Adafruit_NeoPixel object, set data pin back to INPUT.
*/
ws2812_rgb::~ws2812_rgb()
{
    free(pixels);
    if(pin >= 0) {
        //pinMode(pin, INPUT);
        PORT->Group[g_APinDescription[pin].ulPort].PINCFG[g_APinDescription[pin].ulPin].reg=(uint8_t)(PORT_PINCFG_INEN) ;
        PORT->Group[g_APinDescription[pin].ulPort].DIRCLR.reg = (uint32_t)(1<<g_APinDescription[pin].ulPin) ;
    }
}

/*!
  @brief   Configure NeoPixel pin for output.
*/
void ws2812_rgb::begin(void) {
    if(pin >= 0) {
        //pinMode(pin, OUTPUT);
        //digitalWrite(pin, LOW);
        PORT->Group[g_APinDescription[pin].ulPort].PINCFG[g_APinDescription[pin].ulPin].reg=(uint8_t)(PORT_PINCFG_INEN);
        PORT->Group[g_APinDescription[pin].ulPort].DIRSET.reg = (uint32_t)(1<<g_APinDescription[pin].ulPin);
        PORT->Group[g_APinDescription[pin].ulPort].OUTCLR.reg = (1ul << pin);
    }
    begun = true;
}

/*!
  @brief   Transmit pixel data in RAM to NeoPixels.
  @note    On most architectures, interrupts are temporarily disabled in
           order to achieve the correct NeoPixel signal timing. This means
           that the Arduino millis() and micros() functions, which require
           interrupts, will lose small intervals of time whenever this
           function is called (about 30 microseconds per RGB pixel, 40 for
           RGBW pixels). There's no easy fix for this, but a few
           specialized alternative or companion libraries exist that use
           very device-specific peripherals to work around it.
*/
void ws2812_rgb::show(void) 
{
    if(!pixels) return;
    while(!canShow());
    noInterrupts(); // Need 100% focus on instruction timing

    #if defined(__arm__)

        // ARM MCUs 
        #if defined (__SAMD21E17A__) || defined(__SAMD21G18A__)  || defined(__SAMD21E18A__) || defined(__SAMD21J18A__) // Arduino Zero, Gemma/Trinket M0, SODAQ Autonomo and others
        // Tried this with a timer/counter, couldn't quite get adequate
        // resolution. So yay, you get a load of goofball NOPs...

        uint8_t  *ptr, *end, p, bitMask, portNum;
        uint32_t  pinMask;

        portNum =  g_APinDescription[pin].ulPort;
        pinMask =  1ul << g_APinDescription[pin].ulPin;
        ptr     =  pixels;
        end     =  ptr + numBytes;
        p       = *ptr++;
        bitMask =  0x80;

        volatile uint32_t *set = &(PORT->Group[portNum].OUTSET.reg),
                          *clr = &(PORT->Group[portNum].OUTCLR.reg);
        
        for(;;) {
            *set = pinMask;
            asm("nop; nop; nop; nop; nop; nop; nop; nop;");
            if(p & bitMask) {
            asm("nop; nop; nop; nop; nop; nop; nop; nop;"
                "nop; nop; nop; nop; nop; nop; nop; nop;"
                "nop; nop; nop; nop;");
            *clr = pinMask;
            }
            else {
            *clr = pinMask;
            asm("nop; nop; nop; nop; nop; nop; nop; nop;"
                "nop; nop; nop; nop; nop; nop; nop; nop;"
                "nop; nop; nop; nop;");
            }
            if(bitMask >>= 1) {
            asm("nop; nop; nop; nop; nop; nop; nop; nop; nop;");
            } else {
            if(ptr >= end) break;
            p       = *ptr++;
            bitMask = 0x80;
            }
        }
        
        // END ARM ----------------------------------------------------------------
        #elif defined(ESP8266) || defined(ESP32)

        // ESP8266 ----------------------------------------------------------------

        // ESP8266 show() is external to enforce ICACHE_RAM_ATTR execution
        espShow(pin, pixels, numBytes, is800KHz);

        #else
        #error Architecture not supported
        #endif
    #endif // __arm__

    // END ARCHITECTURE SELECT ------------------------------------------------
    interrupts();

    endTime = micros(); // Save EOD time for latch on next call
}

void ws2812_rgb::setPin(uint8_t p)
{
    if(begun && (pin >= 0)) pinMode(pin, INPUT);
        pin = p;
    if(begun) {
        //pinMode(p, OUTPUT);
        //digitalWrite(p, LOW);
        PORT->Group[g_APinDescription[pin].ulPort].PINCFG[g_APinDescription[pin].ulPin].reg=(uint8_t)(PORT_PINCFG_INEN);
        PORT->Group[g_APinDescription[pin].ulPort].DIRSET.reg = (uint32_t)(1<<g_APinDescription[pin].ulPin);
        PORT->Group[g_APinDescription[pin].ulPort].OUTCLR.reg = (1ul << pin);
    }
}

void ws2812_rgb::setColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b)
{
    if(n < numLEDs) {
        if(brightness) { // See notes in setBrightness()
            r = (r * brightness) >> 8;
            g = (g * brightness) >> 8;
            b = (b * brightness) >> 8;
        }

        uint8_t *p;
        p = &pixels[n * 3];    // 3 bytes per pixel
        p[rOffset] = r;        // R,G,B always stored
        p[gOffset] = g;
        p[bOffset] = b;
    }
}

void ws2812_rgb::setColor(uint16_t n, uint32_t c)
{
    if(n < numLEDs) {
        uint8_t *p,
        r = (uint8_t)(c >> 16),
        g = (uint8_t)(c >>  8),
        b = (uint8_t)c;
        
        if(brightness) { // See notes in setBrightness()
            r = (r * brightness) >> 8;
            g = (g * brightness) >> 8;
            b = (b * brightness) >> 8;
        }
        p = &pixels[n * 3];
        p[rOffset] = r;
        p[gOffset] = g;
        p[bOffset] = b;
    }
}

void ws2812_rgb::setBrightness(uint8_t br)
{
    // Stored brightness value is different than what's passed.
    // This simplifies the actual scaling math later, allowing a fast
    // 8x8-bit multiply and taking the MSB. 'brightness' is a uint8_t,
    // adding 1 here may (intentionally) roll over...so 0 = max brightness
    // (color values are interpreted literally; no scaling), 1 = min
    // brightness (off), 255 = just below max brightness.
    uint8_t newBrightness = br + 1;
   
    if(newBrightness != brightness) { // Compare against prior value
    // Brightness has changed -- re-scale existing data in RAM,
    // This process is potentially "lossy," especially when increasing
    // brightness. The tight timing in the WS2811/WS2812 code means there
    // aren't enough free cycles to perform this scaling on the fly as data
    // is issued. So we make a pass through the existing color data in RAM
    // and scale it (subsequent graphics commands also work at this
    // brightness level). If there's a significant step up in brightness,
    // the limited number of steps (quantization) in the old data will be
    // quite visible in the re-scaled version. For a non-destructive
    // change, you'll need to re-render the full strip data. C'est la vie.
        uint8_t c,
                *ptr           = pixels,
                oldBrightness = brightness - 1; // De-wrap old brightness value
        uint16_t scale;
        if(oldBrightness == 0) scale = 0; // Avoid /0
        else if(br == 255) scale = 65535 / oldBrightness;
        else scale = (((uint16_t)newBrightness << 8) - 1) / oldBrightness;
        for(uint16_t i=0; i<numBytes; i++) {
            c      = *ptr;
            *ptr++ = (c * scale) >> 8;
        }
        brightness = newBrightness;
    }
}

void ws2812_rgb::clear(void)
{
    memset(pixels, 0, numBytes);
    show();
}

void ws2812_rgb::length(uint16_t n)
{
    free(pixels); // Free existing data (if any)

    // Allocate new data -- note: ALL PIXELS ARE CLEARED
    numBytes = n * 3;
    if((pixels = (uint8_t *)malloc(numBytes))) {
        memset(pixels, 0, numBytes);
        numLEDs = n;
    } 
    else {
        numLEDs = numBytes = 0;
    }
}

boolean ws2812_rgb::canShow(void) 
{
    return (micros() - endTime) >= 300L;
}

/*
C wrappers
*/

extern "C" void ledEn (void)
{
    if(!ledEnabled){
        ledBuiltin.begin();
        ledBuiltin.setBrightness(bright);
        ledBuiltin.setColor(0, ledBuiltin.Color(rLed, gLed, bLed));
        ledBuiltin.show();
        ledEnabled = true;
    }
}

extern "C" void ledOn (void)
{
    if(ledEnabled){
        ledBuiltin.setColor(0, ledBuiltin.Color(rLed, gLed, bLed));
        ledBuiltin.show();
        ledStatus = HIGH;
    }
}

extern "C" void ledOff (void)
{
    if(ledEnabled){
        ledBuiltin.clear();
        ledStatus = LOW;
	}
}

extern "C" void ledToggle (void)
{
    if(ledStatus == HIGH)
        ledOff();
    else
        ledOn();
}

extern "C" void ledSetColor(uint8_t r, uint8_t g, uint8_t b)
{
    rLed = r;
    gLed = g;
    bLed = b;
    ledBuiltin.setColor(0, ledBuiltin.Color(rLed, gLed, bLed));
}

extern "C" void ledSetBrightness(uint8_t br)
{
    bright = br;
    ledBuiltin.setBrightness(bright);
}

#endif