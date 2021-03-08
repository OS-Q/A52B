#ifdef CUSTOM_VARIANT
	#include "variantCustom.h"
#else

#ifndef _VARIANT_ESP32_
#define _VARIANT_ESP32_

#include <stdint.h>
#include "WVariant.h"
#include "mbc_config.h"

//#define HW_VER_1_0	// HW ver. 1.0
#define HW_VER_1_1		// HW ver. 1.1

#ifdef BRIKI_ABC
	#define PINS_COUNT              74
#else
	#define PINS_COUNT              72
#endif

#define EXTERNAL_NUM_INTERRUPTS 16
#define NUM_DIGITAL_PINS        41
#define NUM_ANALOG_INPUTS       18
#define ANALOG_PIN_OFFSET1      26
#define ANALOG_PIN_OFFSET2      52

#define analogInputToDigitalPin(p)  ((p < NUM_ANALOG_INPUTS) ? ((p < 6) ? (p) + ANALOG_PIN_OFFSET1 : (p + ANALOG_PIN_OFFSET2 - 6)) : -1)
#define digitalPinToInterrupt(p)    (((p)<PINS_COUNT)?(p):-1)
#define digitalPinHasPWM(p)         (p < 34)

static const uint8_t TX = 5;
static const uint8_t RX = 6;

//-----------------------------------------------------------------------------------------
        
static const uint8_t UART_TX_0 = 1; 
static const uint8_t UART_RX_0 = 0; 

static const  uint8_t UART_TX_2 = 4;
static const  uint8_t UART_RX_2 = 3;
static const uint8_t SDA = 9;
static const uint8_t SCL = 10;

static const uint8_t SDA1 = 7;
static const uint8_t SCL1 = 8;

# define SPI_PIN_DEFAULT 1 // 1 new spi pins ,  0 for default 
static const uint8_t SPI_SS    = 35; 
static const uint8_t SPI_MOSI  = 36; 
static const uint8_t SPI_MISO  = 37; 
static const uint8_t SPI_SCK   = 10; 


// -----------------------------------------------------------------------------------------
static const uint8_t SS    = 41;
static const uint8_t MOSI  = 39;
static const uint8_t MISO  = 38;
static const uint8_t SCK   = 40;
static const uint8_t ACK_PIN    = 42;

static const uint8_t A0 = 26;  // SAMD analog
static const uint8_t A1 = 27;  // SAMD analog
static const uint8_t A2 = 28;  // SAMD analog
static const uint8_t A3 = 29;  // SAMD analog
static const uint8_t A4 = 30;  // SAMD analog
static const uint8_t A5 = 31;  // SAMD analog
static const uint8_t A6 = 52;  // SAMD analog
static const uint8_t A7 = 53;  // SAMD analog
static const uint8_t A8 = 54;  // SAMD analog
static const uint8_t A9 = 55;  // SAMD analog
static const uint8_t A10 = 56;  // SAMD analog
static const uint8_t A11 = 57;  // SAMD analog
static const uint8_t A12 = 58;  // SAMD analog
static const uint8_t A13 = 59;  // SAMD analog
static const uint8_t A14 = 60;  // ESP analog
static const uint8_t A15 = 61;  // ESP analog
static const uint8_t A16 = 62;  // ESP analog
static const uint8_t A17 = 63;  // ESP analog
// A18 to A26 works only if WiFi is disabled
static const uint8_t A18 = 64;  // ESP analog
static const uint8_t A19 = 65;  // ESP analog
static const uint8_t A20 = 66;  // ESP analog
static const uint8_t A21 = 67;  // ESP analog
static const uint8_t A22 = 68; // ESP analog
static const uint8_t A23 = 69; // ESP analog
static const uint8_t A24 = 70; // ESP analog
static const uint8_t A25 = 71; // ESP analog
static const uint8_t A26 = 72; // ESP analog

static const uint8_t T0 = 9;
static const uint8_t T1 = 255; // Not exposed pin - GPIO0
static const uint8_t T2 = 10;
static const uint8_t T3 = 32;
static const uint8_t T4 = 33;
static const uint8_t T5 = 34;
static const uint8_t T6 = 35;
static const uint8_t T7 = 1;
static const uint8_t T8 = 4;
static const uint8_t T9 = 2;


static const uint8_t DAC1 = 0;
static const uint8_t DAC2 = 3;

static const uint8_t COMPANION_RESET = 45;

static const uint8_t BOOT = 46;

#ifdef BRIKI_ABC
	static const uint8_t LED_BUILTIN = 74;
#else
	static const uint8_t LED_BUILTIN = 255; // avoid compiling error
#endif

typedef enum {
  // 0..25 - Digital pins
  // 0..10 - ESP GPIO
  espGpio1  = 5,
  espGpio3,
  // 11..15 - SAMD GPIO
  samPA23   = 11,
  samPA14,
  // 16..25 - SAMD GPIO
  samPA27   = 16,
  samPB11   = 21,
  samPB10,
  samPA12,
  samPA13,
  // 26..31 - Analog pins
  samPA2    = 26,
  samPA3,
  samPA4,
  samPA5,
  samPA6,
  samPA7,
  // 32..37 - ESP GPIO
  espGpio16 = 36,
  espGpio17,
  // end Arduino pinout
  // 38..39 - SWD
  samPA31,
  samPA30,
  // 40..42 - ESP Control (Enable, Boot, Power ON/OFF)
  #if defined HW_VER_1_0
    samPA22,
  #elif defined HW_VER_1_1
    samPA28,
  #endif
  samPA20,
  samPA15,
  // 43..47 SPI - connected to ESP
  samPA16,
  samPA17,
  samPA18,
  samPA19,
  samPA21,
  // 48..49 - USB
  samPA24,
  samPA25,
  // 50..51 - ESP programming UART
  samPB22,
  samPB23,
  // 52..72 - Additional analog pins
  samPB2,
  samPB3,
  samPB8,
  samPB9,
  samPA11,
  samPA10,
  samPA9,
  samPA8,
  espGpio32,
  espGpio33,
  espGpio35,
  espGpio34,
  espGpio25,
  espGpio27,
  espGpio26,
  espGpio4,
  espGpio2,
  espGpio15,
  espGpio13,
  espGpio12,
  espGpio14,
  // 73 - fake pin
  fakeGpio,
  #if defined HW_VER_1_1
    samPA22,
  #endif
} hwPinMap_e;


#endif //_VARIANT_ESP32_   

#endif /* CUSTOM_VARIANT */      