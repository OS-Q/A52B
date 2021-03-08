/*
  Copyright (c) 2014 Arduino LLC.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "Arduino.h"
#include "wiring_private.h"

#ifdef __cplusplus
extern "C" {
#endif

static int _readResolution = 12;
static int _ADCResolution = 12;
static int _writeResolution = 8;
uint8_t ADCinitialized = 0;
uint8_t DACinitialized = 0;

bool tcEnabled[TCC_INST_NUM+TC_INST_NUM];
static uint8_t resolution[TCC_INST_NUM+TC_INST_NUM];
static uint32_t maxValue[TCC_INST_NUM+TC_INST_NUM];

// Wait for synchronization of registers between the clock domains
static __inline__ void syncADC() __attribute__((always_inline, unused));
static void syncADC() {
  while (ADC->STATUS.bit.SYNCBUSY == 1)
    ;
}

// Wait for synchronization of registers between the clock domains
static __inline__ void syncDAC() __attribute__((always_inline, unused));
static void syncDAC() {
  while (DAC->STATUS.bit.SYNCBUSY == 1)
    ;
}

// Wait for synchronization of registers between the clock domains
static __inline__ void syncTC_16(Tc* TCx) __attribute__((always_inline, unused));
static void syncTC_16(Tc* TCx) {
  while (TCx->COUNT16.STATUS.bit.SYNCBUSY);
}

// Wait for synchronization of registers between the clock domains
static __inline__ void syncTCC(Tcc* TCCx) __attribute__((always_inline, unused));
static void syncTCC(Tcc* TCCx) {
  while (TCCx->SYNCBUSY.reg & TCC_SYNCBUSY_MASK);
}

void initADC(void){
	// Setting clock
	while(GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY);
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID( GCM_ADC ) | // Generic Clock ADC
					  GCLK_CLKCTRL_GEN_GCLK0     | // Generic Clock Generator 0 is source
					  GCLK_CLKCTRL_CLKEN ;
	while(ADC->STATUS.bit.SYNCBUSY == 1);          // Wait for synchronization of registers between the clock domains
	ADC->CTRLB.reg = ADC_CTRLB_PRESCALER_DIV512 |    // Divide Clock by 512.
							 ADC_CTRLB_RESSEL_12BIT;         // 12 bits resolution as default
	ADC->SAMPCTRL.reg = 0x3f;                        // Set max Sampling Time Length
	while( ADC->STATUS.bit.SYNCBUSY == 1 );          // Wait for synchronization of registers between the clock domains
	//ADC->INPUTCTRL.reg = ADC_INPUTCTRL_MUXNEG_GND;   // No Negative input (Internal Ground)
	ADC->INPUTCTRL.reg = ADC_INPUTCTRL_MUXNEG_IOGND;   // No Negative input (Internal Ground)

	ADCinitialized = 1;

	// Averaging (see datasheet table in AVGCTRL register description)
	#ifdef BRIKI_ABC
	analogHWAveraging(ADC_AVGCTRL_SAMPLENUM_8, DIV_FACT_8);
	#else
	analogHWAveraging(ADC_AVGCTRL_SAMPLENUM_1, DIV_FACT_1);
	#endif

	analogReference( AR_DEFAULT ) ; // Analog Reference is AREF pin (3.3v)
}

void initDAC(void){
  // Setting clock
  while ( GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY );
  GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID( GCM_DAC ) | // Generic Clock ADC
                      GCLK_CLKCTRL_GEN_GCLK0     | // Generic Clock Generator 0 is source
                      GCLK_CLKCTRL_CLKEN ;

  while ( DAC->STATUS.bit.SYNCBUSY == 1 ); // Wait for synchronization of registers between the clock domains
  DAC->CTRLB.reg = DAC_CTRLB_REFSEL_AVCC | // Using the 3.3V reference
                   DAC_CTRLB_EOEN ;        // External Output Enable (Vout)
  DACinitialized = 1;
}

void analogReadResolution(int res)
{
  if (!ADCinitialized) {
    initADC();
  }

  _readResolution = res;
  if (res > 10) {
    ADC->CTRLB.bit.RESSEL = ADC_CTRLB_RESSEL_12BIT_Val;
    _ADCResolution = 12;
  } else if (res > 8) {
    ADC->CTRLB.bit.RESSEL = ADC_CTRLB_RESSEL_10BIT_Val;
    _ADCResolution = 10;
  } else {
    ADC->CTRLB.bit.RESSEL = ADC_CTRLB_RESSEL_8BIT_Val;
    _ADCResolution = 8;
  }
  syncADC();
}

void analogReadGain(eAnalogGain g)
{
  if (!ADCinitialized) {
    initADC();
  }

  uint8_t gain = ADC_INPUTCTRL_GAIN_DIV2_Val;

  //syncADC();
  switch(g){
    case AG_0_5X:
    case AG_DEFAULT:
    break;

    case AG_1X:
      gain = ADC_INPUTCTRL_GAIN_1X_Val;
    break;

    case AG_2X:
      gain = ADC_INPUTCTRL_GAIN_2X_Val;
    break;

    case AG_4X:
      gain = ADC_INPUTCTRL_GAIN_4X_Val;
    break;

    case AG_8X:
      gain = ADC_INPUTCTRL_GAIN_8X_Val;
    break;

    case AG_16X:
      gain = ADC_INPUTCTRL_GAIN_16X_Val;
    break;
  }
  syncADC();
  ADC->INPUTCTRL.bit.GAIN = gain;
  syncADC();
}

void analogWriteResolution(int res)
{
  _writeResolution = res;
}

static inline uint32_t mapResolution(uint32_t value, uint32_t from, uint32_t to)
{
  if (from == to) {
    return value;
  }
  if (from > to) {
    return value >> (from-to);
  }
  return value << (to-from);
}

/*
 * Internal Reference is at 1.0v
 * External Reference should be between 1v and VDDANA-0.6v=2.7v
 *
 * Warning : On Arduino Zero board the input/output voltage for SAMD21G18 is 3.3 volts maximum
 */
void analogReference(eAnalogReference mode)
{
  if (!ADCinitialized) {
    initADC();
  }

  syncADC();
  switch (mode)
  {
    case AR_INTERNAL:
    case AR_INTERNAL2V23:
      ADC->INPUTCTRL.bit.GAIN = ADC_INPUTCTRL_GAIN_1X_Val;      // Gain Factor Selection
      ADC->REFCTRL.bit.REFSEL = ADC_REFCTRL_REFSEL_INTVCC0_Val; // 1/1.48 VDDANA = 1/1.48* 3V3 = 2.2297
      break;

    case AR_EXTERNAL:
      ADC->INPUTCTRL.bit.GAIN = ADC_INPUTCTRL_GAIN_1X_Val;      // Gain Factor Selection
      ADC->REFCTRL.bit.REFSEL = ADC_REFCTRL_REFSEL_AREFA_Val;
      break;

    case AR_INTERNAL1V0:
      ADC->INPUTCTRL.bit.GAIN = ADC_INPUTCTRL_GAIN_1X_Val;      // Gain Factor Selection
      ADC->REFCTRL.bit.REFSEL = ADC_REFCTRL_REFSEL_INT1V_Val;   // 1.0V voltage reference
      break;

    case AR_INTERNAL1V65:
      ADC->INPUTCTRL.bit.GAIN = ADC_INPUTCTRL_GAIN_1X_Val;      // Gain Factor Selection
      ADC->REFCTRL.bit.REFSEL = ADC_REFCTRL_REFSEL_INTVCC1_Val; // 1/2 VDDANA = 0.5* 3V3 = 1.65V
      break;

    case AR_DEFAULT:
    default:
      ADC->INPUTCTRL.bit.GAIN = ADC_INPUTCTRL_GAIN_DIV2_Val;
      //ADC->REFCTRL.bit.REFSEL = ADC_REFCTRL_REFSEL_INTVCC1_Val; // 1/2 VDDANA = 0.5* 3V3 = 1.65V
      ADC->REFCTRL.reg = ADC_REFCTRL_REFSEL_INTVCC1 | ADC_REFCTRL_REFCOMP;
      break;
  }
}

uint32_t analogRead(uint32_t pin)
{
  uint32_t valueRead = 0;

  if (pin < NUM_ANALOG_INPUTS){
	pin = analogInputToDigitalPin(pin);
  }

  // Handle read on pins connected to communication chip
  if(g_APinDescription[pin].ulPinType == PIO_EXTERNAL){
	#ifdef COMMUNICATION_LIB_ENABLED
		return sendGPIORead(READ_ANALOG, (uint8_t)pin);
	#else
		return 0;
	#endif
  }

  if (!ADCinitialized) {
    initADC();
  }

  pinPeripheral(pin, PIO_ANALOG);

  // Disable DAC, if analogWrite() was used previously to enable the DAC
  if ((g_APinDescription[pin].ulADCChannelNumber == ADC_Channel0) || (g_APinDescription[pin].ulADCChannelNumber == DAC_Channel0)) {
    syncDAC();
    DAC->CTRLA.bit.ENABLE = 0x00; // Disable DAC
    //DAC->CTRLB.bit.EOEN = 0x00; // The DAC output is turned off.
    syncDAC();
  }

  syncADC();
  ADC->INPUTCTRL.bit.MUXPOS = g_APinDescription[pin].ulADCChannelNumber; // Selection for the positive ADC input

  // Control A
  /*
   * Bit 1 ENABLE: Enable
   *   0: The ADC is disabled.
   *   1: The ADC is enabled.
   * Due to synchronization, there is a delay from writing CTRLA.ENABLE until the peripheral is enabled/disabled. The
   * value written to CTRL.ENABLE will read back immediately and the Synchronization Busy bit in the Status register
   * (STATUS.SYNCBUSY) will be set. STATUS.SYNCBUSY will be cleared when the operation is complete.
   *
   * Before enabling the ADC, the asynchronous clock source must be selected and enabled, and the ADC reference must be
   * configured. The first conversion after the reference is changed must not be used.
   */
  syncADC();
  ADC->CTRLA.bit.ENABLE = 0x01;             // Enable ADC

  // Start conversion
  syncADC();
  ADC->SWTRIG.bit.START = 1;

  // Waiting for the 1st conversion to complete
  while (ADC->INTFLAG.bit.RESRDY == 0);

  // Clear the Data Ready flag
  ADC->INTFLAG.reg = ADC_INTFLAG_RESRDY;

  // Start conversion again, since The first conversion after the reference is changed must not be used.
  syncADC();
  ADC->SWTRIG.bit.START = 1;

  // Store the value
  while (ADC->INTFLAG.bit.RESRDY == 0);   // Waiting for conversion to complete
  valueRead = ADC->RESULT.reg;

  syncADC();
  ADC->CTRLA.bit.ENABLE = 0x00;             // Disable ADC
  syncADC();

  return mapResolution(valueRead, _ADCResolution, _readResolution);
}


// Right now, PWM output only works on the pins with
// hardware support.  These are defined in the appropriate
// pins_*.c file.  For the rest of the pins, we default
// to digital output.
void analogWrite(uint32_t pin, uint32_t value)
{
  PinDescription pinDesc = g_APinDescription[pin];
  uint32_t attr = pinDesc.ulPinAttribute;

  // Handle write on pins connected to communication chip
  if(g_APinDescription[pin].ulPinType == PIO_EXTERNAL){
	#ifdef COMMUNICATION_LIB_ENABLED
	sendGPIOWrite(WRITE_ANALOG, (uint8_t)pin, value);
	#endif
	return;
  }
  
  if (((attr & PIN_ATTR_ANALOG) == PIN_ATTR_ANALOG) && ((pinDesc.ulADCChannelNumber == ADC_Channel0) || (pinDesc.ulADCChannelNumber == DAC_Channel0)))
  {
    // DAC handling code
	if (!DACinitialized) {
      initDAC();
    }

    value = mapResolution(value, _writeResolution, 10);

    syncDAC();
    DAC->DATA.reg = value & 0x3FF;  // DAC on 10 bits.
    syncDAC();
    DAC->CTRLA.bit.ENABLE = 0x01;     // Enable DAC
    syncDAC();
    return;
  }

  if ((attr & PIN_ATTR_PWM) == PIN_ATTR_PWM)
  {
    uint32_t tcNum = GetTCNumber(pinDesc.ulPWMChannel);
    uint8_t tcChannel = GetTCChannelNumber(pinDesc.ulPWMChannel);

    if (attr & PIN_ATTR_TIMER) {
      #if !(ARDUINO_SAMD_VARIANT_COMPLIANCE >= 10603)
      // Compatibility for cores based on SAMD core <=1.6.2
      if (pinDesc.ulPinType == PIO_TIMER_ALT) {
        pinPeripheral(pin, PIO_TIMER_ALT);
      } else
      #endif
      {
        pinPeripheral(pin, PIO_TIMER);
      }
    } else {
      // We suppose that attr has PIN_ATTR_TIMER_ALT bit set...
      pinPeripheral(pin, PIO_TIMER_ALT);
    }

    if (!tcEnabled[tcNum]) {
      value = mapResolution(value, _writeResolution, 16);
      tcEnabled[tcNum] = true;
	  resolution[tcNum] = 16;
	  maxValue[tcNum] = 1 << 16;

      uint16_t GCLK_CLKCTRL_IDs[] = {
        GCLK_CLKCTRL_ID(GCM_TCC0_TCC1), // TCC0
        GCLK_CLKCTRL_ID(GCM_TCC0_TCC1), // TCC1
        GCLK_CLKCTRL_ID(GCM_TCC2_TC3),  // TCC2
        GCLK_CLKCTRL_ID(GCM_TCC2_TC3),  // TC3
        GCLK_CLKCTRL_ID(GCM_TC4_TC5),   // TC4
        GCLK_CLKCTRL_ID(GCM_TC4_TC5),   // TC5
        GCLK_CLKCTRL_ID(GCM_TC6_TC7),   // TC6
        GCLK_CLKCTRL_ID(GCM_TC6_TC7),   // TC7
      };
      GCLK->CLKCTRL.reg = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_IDs[tcNum]);
      while (GCLK->STATUS.bit.SYNCBUSY == 1);

      // Set PORT
      if (tcNum >= TCC_INST_NUM) {
        // -- Configure TC
        Tc* TCx = (Tc*) GetTC(pinDesc.ulPWMChannel);
        // Disable TCx
        TCx->COUNT16.CTRLA.bit.ENABLE = 0;
        syncTC_16(TCx);
        // Set Timer counter Mode to 16 bits, normal PWM
        TCx->COUNT16.CTRLA.reg |= TC_CTRLA_MODE_COUNT16 | TC_CTRLA_WAVEGEN_NPWM;
        syncTC_16(TCx);
        // Set the initial value
        TCx->COUNT16.CC[tcChannel].reg = (uint32_t) value;
        syncTC_16(TCx);
        // Enable TCx
        TCx->COUNT16.CTRLA.bit.ENABLE = 1;
        syncTC_16(TCx);
      } else {
        // -- Configure TCC
        Tcc* TCCx = (Tcc*) GetTC(pinDesc.ulPWMChannel);
        // Disable TCCx
        TCCx->CTRLA.bit.ENABLE = 0;
        syncTCC(TCCx);
        // Set TCCx as normal PWM
        TCCx->WAVE.reg |= TCC_WAVE_WAVEGEN_NPWM;
        syncTCC(TCCx);
        // Set the initial value
        TCCx->CC[tcChannel].reg = (uint32_t) value;
        syncTCC(TCCx);
        // Set PER to maximum counter value (resolution : 0xFFFF)
        TCCx->PER.reg = 0xFFFF;
        syncTCC(TCCx);
        // Enable TCCx
        TCCx->CTRLA.bit.ENABLE = 1;
        syncTCC(TCCx);
      }
    } else {
      value = mapResolution(value, _writeResolution, resolution[tcNum]);
	  // scale value to maxValue if needed
	  if(maxValue[tcNum] != (1 << resolution[tcNum]))
	     value = maxValue[tcNum] * value / (1 << resolution[tcNum]);
	  if (tcNum >= TCC_INST_NUM) {
        Tc* TCx = (Tc*) GetTC(pinDesc.ulPWMChannel);
        if(resolution[tcNum] == 16){
          TCx->COUNT16.CC[tcChannel].reg = (uint32_t) value;
          syncTC_16(TCx);
        }
        else{
          TCx->COUNT8.CC[tcChannel].reg = (uint32_t) value;
          while(TCx->COUNT8.STATUS.bit.SYNCBUSY);
        }
      } else {
        Tcc* TCCx = (Tcc*) GetTC(pinDesc.ulPWMChannel);
        TCCx->CTRLBSET.bit.LUPD = 1;
        syncTCC(TCCx);
		TCCx->CCB[tcChannel].reg = (uint32_t) value;
        syncTCC(TCCx);
        TCCx->CTRLBCLR.bit.LUPD = 1;
        syncTCC(TCCx);
      }
    }
    return;
  }

  // -- Defaults to digital write
  pinMode(pin, OUTPUT);
  value = mapResolution(value, _writeResolution, 8);
  if (value < 128) {
    digitalWrite(pin, LOW);
  } else {
    digitalWrite(pin, HIGH);
  }
}

// Right now, PWM output only works on the pins with
// hardware support.  These are defined in the appropriate
// pins_*.c file.  For the rest of the pins, we default
// to digital output.
void analogWriteFreq(uint32_t pin, uint32_t value, uint32_t frequency)
{
  PinDescription pinDesc = g_APinDescription[pin];
  uint32_t attr = pinDesc.ulPinAttribute;

  // Handle write on pins connected to communication chip
  if(g_APinDescription[pin].ulPinType == PIO_EXTERNAL){
	#ifdef COMMUNICATION_LIB_ENABLED
	sendGPIOWrite(WRITE_ANALOG_F, (uint8_t)pin, value, frequency);
	#endif
	return;
  }

  if (((attr & PIN_ATTR_ANALOG) == PIN_ATTR_ANALOG) && ((pinDesc.ulADCChannelNumber == ADC_Channel0) && (pinDesc.ulADCChannelNumber == DAC_Channel0)))
  {
    // DAC handling code
	if (!DACinitialized) {
      initDAC();
    }

    value = mapResolution(value, _writeResolution, 10);

    syncDAC();
    DAC->DATA.reg = value & 0x3FF;  // DAC on 10 bits.
    syncDAC();
    DAC->CTRLA.bit.ENABLE = 0x01;     // Enable DAC
    syncDAC();
    return;
  }

  if ((attr & PIN_ATTR_PWM) == PIN_ATTR_PWM)
  {

    uint32_t tcNum = GetTCNumber(pinDesc.ulPWMChannel);
    uint8_t tcChannel = GetTCChannelNumber(pinDesc.ulPWMChannel);

    if (attr & PIN_ATTR_TIMER) {
      #if !(ARDUINO_SAMD_VARIANT_COMPLIANCE >= 10603)
      // Compatibility for cores based on SAMD core <=1.6.2
      if (pinDesc.ulPinType == PIO_TIMER_ALT) {
        pinPeripheral(pin, PIO_TIMER_ALT);
      } else
      #endif
      {
        pinPeripheral(pin, PIO_TIMER);
      }
    } else {
      // We suppose that attr has PIN_ATTR_TIMER_ALT bit set...
      pinPeripheral(pin, PIO_TIMER_ALT);
    }

    tcEnabled[tcNum] = true;

    uint16_t GCLK_CLKCTRL_IDs[] = {
        GCLK_CLKCTRL_ID(GCM_TCC0_TCC1), // TCC0
        GCLK_CLKCTRL_ID(GCM_TCC0_TCC1), // TCC1
        GCLK_CLKCTRL_ID(GCM_TCC2_TC3),  // TCC2
        GCLK_CLKCTRL_ID(GCM_TCC2_TC3),  // TC3
        GCLK_CLKCTRL_ID(GCM_TC4_TC5),   // TC4
        GCLK_CLKCTRL_ID(GCM_TC4_TC5),   // TC5
        GCLK_CLKCTRL_ID(GCM_TC6_TC7),   // TC6
        GCLK_CLKCTRL_ID(GCM_TC6_TC7),   // TC7
    };
    GCLK->CLKCTRL.reg = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_IDs[tcNum]);
    while (GCLK->STATUS.bit.SYNCBUSY == 1);

    // Set PORT
    if (tcNum >= TCC_INST_NUM) { // TC instance
        // -- Configure TC
        Tc* TCx = (Tc*) GetTC(pinDesc.ulPWMChannel);
		// reset both 16 bit and 8 bit registers
		TCx->COUNT16.CTRLA.bit.SWRST = 1;
		syncTC_16(TCx);
		TCx->COUNT8.CTRLA.bit.SWRST = 1;
		while(TCx->COUNT8.STATUS.bit.SYNCBUSY);
		// we need to use 16 bit TC register for frequencies between 1 and 137Hz
		if(frequency <= 137){
			value = mapResolution(value, _writeResolution, 16);
			// Set Timer counter Mode to 16 bits, normal PWM
			TCx->COUNT16.CTRLA.reg |= TC_CTRLA_MODE_COUNT16 | TC_CTRLA_WAVEGEN_NPWM;
			// set prescaler that better fits given frequency
			if(frequency > 68)
				TCx->COUNT16.CTRLA.bit.PRESCALER = 0x03; // prescaler 8
			else if(frequency > 28)
				TCx->COUNT16.CTRLA.bit.PRESCALER = 0x04; // prescaler 16
			else if(frequency > 7)
				TCx->COUNT16.CTRLA.bit.PRESCALER = 0x05; // prescaler 64
			else
				TCx->COUNT16.CTRLA.bit.PRESCALER = 0x06; // prescaler 256
			syncTC_16(TCx);
			// Set the initial value
			TCx->COUNT16.CC[tcChannel].reg = (uint32_t) value;
			syncTC_16(TCx);
			// Enable TCx
			TCx->COUNT16.CTRLA.bit.ENABLE = 1;
			syncTC_16(TCx);

			resolution[tcNum] = 16;
			maxValue[tcNum] = 1 << 16;
		}
		else{
			// use 8 bit TC register for frequencies above 137Hz. We are able to be more precise with 8bit register
			value = mapResolution(value, _writeResolution, 8);
			uint8_t per;
			// Set Timer counter Mode to 8 bits, normal PWM
			TCx->COUNT8.CTRLA.reg |= TC_CTRLA_MODE_COUNT8 | TC_CTRLA_WAVEGEN_NPWM;
			// chose the right prescaler and PER value depending by frequency
			if(frequency < 732){
				TCx->COUNT8.CTRLA.bit.PRESCALER = 0x07; // 1024
				per = SystemCoreClock / (1024 * frequency) - 1;
				TCx->COUNT8.PER.reg = per;
				while(TCx->COUNT8.STATUS.bit.SYNCBUSY);
			}
			else if(frequency < 5000){
				TCx->COUNT8.CTRLA.bit.PRESCALER = 0x06; // 256
				per = SystemCoreClock / (256 * frequency) - 1;
				TCx->COUNT8.PER.reg = per;
				while(TCx->COUNT8.STATUS.bit.SYNCBUSY);
			}
			else if(frequency < 23000){
				TCx->COUNT8.CTRLA.bit.PRESCALER = 0x05; // 64
				per = SystemCoreClock / (64 * frequency) - 1;
				TCx->COUNT8.PER.reg = per;
				while(TCx->COUNT8.STATUS.bit.SYNCBUSY);
			}
			else if(frequency < 187500){
				TCx->COUNT8.CTRLA.bit.PRESCALER = 0x03; // 8
				per = SystemCoreClock / (8 * frequency) - 1;
				TCx->COUNT8.PER.reg = per;
				while(TCx->COUNT8.STATUS.bit.SYNCBUSY);
			}
			else{ // up to 24MHz
				TCx->COUNT8.CTRLA.bit.PRESCALER = 0x00; // 1
				per = SystemCoreClock / frequency - 1;
				TCx->COUNT8.PER.reg = per;
				while(TCx->COUNT8.STATUS.bit.SYNCBUSY);
			}
			// scale value proportionally to PER value
			value = (value * per) / 256;
			// Set the initial value
			TCx->COUNT8.CC[tcChannel].reg = (uint32_t) value;
			while(TCx->COUNT8.STATUS.bit.SYNCBUSY);
			// Enable TCx
			TCx->COUNT8.CTRLA.bit.ENABLE = 1;
			while(TCx->COUNT8.STATUS.bit.SYNCBUSY);

			resolution[tcNum] = 8;
			maxValue[tcNum] = per;
		}
    } else { // TCC instance
        uint32_t per = SystemCoreClock / frequency;
        // -- Configure TCC
        Tcc* TCCx = (Tcc*) GetTC(pinDesc.ulPWMChannel);
        // Disable TCCx
        TCCx->CTRLA.bit.ENABLE = 0;
        // Reset TCCx instance
        TCCx->CTRLA.bit.SWRST = 1;
        syncTCC(TCCx);
        // Set TCCx as normal PWM
        TCCx->WAVE.reg |= TCC_WAVE_WAVEGEN_NPWM;
		per = (per > 0xFFFFFF) ? 0xFFFFFF : per; // 0xFFFFFF is the maximum allowed value
		if(frequency < 187500){
			// use 8 prescaler as to avoid overflow during value calculation
			TCCx->CTRLA.bit.PRESCALER = 0x03; // 8
			per = SystemCoreClock / (frequency * 8);
			value = mapResolution(value, _writeResolution, 8);
			value = per * value / 256;

			resolution[tcNum] = 8;
			maxValue[tcNum] = per;
		}
		else{
			value = mapResolution(value, _writeResolution, 24);
			// scale value proportionally to PER value
			value = per * value / 0xFFFFFF;
			resolution[tcNum] = 24;
			maxValue[tcNum] = per;
		}
        syncTCC(TCCx);
        // Set the initial value
        TCCx->CC[tcChannel].reg = (uint32_t) value;
        syncTCC(TCCx);
        // Set PER to maximum counter value
        TCCx->PER.reg = per;
        syncTCC(TCCx);
        // Enable TCCx
        TCCx->CTRLA.bit.ENABLE = 1;
        syncTCC(TCCx);
    }
    return;
  }

  // -- Defaults to digital write
  pinMode(pin, OUTPUT);
  value = mapResolution(value, _writeResolution, 8);
  if (value < 128) {
    digitalWrite(pin, LOW);
  } else {
    digitalWrite(pin, HIGH);
  }
}

// sets the ADC clock relative to the peripheral clock. pg 864
void analogPrescaler(uint8_t val) 
{
  if (!ADCinitialized) {
    initADC();
  }

  syncADC();
  ADC->CTRLB.bit.PRESCALER = val;
  syncADC();
}

void analogCalibrate(void) 
{ 
  if (!ADCinitialized) {
    initADC();
  }
  syncADC();
  // read NVM register
  uint32_t adc_linearity = (*((uint32_t *)(NVMCTRL_OTP4) // original position
          + (NVM_ADC_LINEARITY_POS / 32)) // move to the correct 32 bit window, read value
          >> (NVM_ADC_LINEARITY_POS % 32)) // shift value to match the desired position
          & ((1 << NVM_ADC_LINEARITY_SIZE) - 1); // apply a bitmask for the desired size

  uint32_t adc_biascal = (*((uint32_t *)(NVMCTRL_OTP4)
          + (NVM_ADC_BIASCAL_POS / 32))
          >> (NVM_ADC_BIASCAL_POS % 32))
          & ((1 << NVM_ADC_BIASCAL_SIZE) - 1);
  
  // write values to CALIB register
  ADC->CALIB.bit.LINEARITY_CAL = adc_linearity;
  ADC->CALIB.bit.BIAS_CAL = adc_biascal;
  syncADC();
}

/* Averaging is a feature that increases the sample accuracy, at the cost of a reduced sampling rate. 
This feature is suitable when operating in noisy conditions.
Averaging is done by accumulating m samples and dividing the result by m. 
The averaged result is available in the RESULT register

params: - eAnalogAccumuDepth acc: number of accumulated samples
        - eAnalogDivFact div: number of right shift to scale the result

Final result of ADC conversion is 12 bits in each case.
*/
void analogHWAveraging (eAnalogAccumuDepth acc, eAnalogDivFact div)
{
  if (!ADCinitialized) {
    initADC();
  }

  syncADC();
  switch(acc) {
    case ACCUM_1_SAMPLE:
      ADC->CTRLB.reg = ADC_CTRLB_PRESCALER_DIV512 |    // Divide Clock by 512.
				               ADC_CTRLB_RESSEL_12BIT;         // 12 bits resolution as default
      ADC->SAMPCTRL.reg = 0x3f;                        // Set max Sampling Time Length
    break;

    case ACCUM_2_SAMPLE:
      ADC->CTRLB.reg = ADC_CTRLB_PRESCALER_DIV256 |    // Divide Clock by 256.
				               ADC_CTRLB_RESSEL_16BIT;         // averaging enabled
      ADC->SAMPCTRL.reg = 0x0;                         // Set max Sampling Time Length
    break;

    case ACCUM_4_SAMPLE:
      ADC->CTRLB.reg = ADC_CTRLB_PRESCALER_DIV128 |    // Divide Clock by 128.
				               ADC_CTRLB_RESSEL_16BIT;         // averaging enabled
      ADC->SAMPCTRL.reg = 0x0;                         // Set max Sampling Time Length
    break;

    case ACCUM_8_SAMPLE:
      ADC->CTRLB.reg = ADC_CTRLB_PRESCALER_DIV64 |     // Divide Clock by 64.
				               ADC_CTRLB_RESSEL_16BIT;         // averaging enabled
      ADC->SAMPCTRL.reg = 0x0;                         // Set max Sampling Time Length
    break;

    case ACCUM_16_SAMPLE:
      ADC->CTRLB.reg = ADC_CTRLB_PRESCALER_DIV32 |     // Divide Clock by 32.
				               ADC_CTRLB_RESSEL_16BIT;         // averaging enabled
      ADC->SAMPCTRL.reg = 0x0;                         // Set max Sampling Time Length
    break;

    case ACCUM_32_SAMPLE:
      ADC->CTRLB.reg = ADC_CTRLB_PRESCALER_DIV16 |     // Divide Clock by 16.
				               ADC_CTRLB_RESSEL_16BIT;         // averaging enabled
      ADC->SAMPCTRL.reg = 0x0;                         // Set max Sampling Time Length
    break;

    case ACCUM_64_SAMPLE:
      ADC->CTRLB.reg = ADC_CTRLB_PRESCALER_DIV8 |      // Divide Clock by 8.
				               ADC_CTRLB_RESSEL_16BIT;         // averaging enabled
      ADC->SAMPCTRL.reg = 0x0;                         // Set max Sampling Time Length
    break;

    case ACCUM_128_SAMPLE:
    case ACCUM_256_SAMPLE:
    case ACCUM_512_SAMPLE:
    case ACCUM_1024_SAMPLE:
      ADC->CTRLB.reg = ADC_CTRLB_PRESCALER_DIV4 |      // Divide Clock by 4.
				               ADC_CTRLB_RESSEL_16BIT;         // averaging enabled
      ADC->SAMPCTRL.reg = 0x0;                         // Set max Sampling Time Length
    break;
  }

  ADC->AVGCTRL.reg = acc | ADC_AVGCTRL_ADJRES(div);
  syncADC();
}

/*
By using oversampling and decimation, the ADC resolution can be increased from 12 bits up to 16 bits,
for the cost of reduced effective sampling rate.
To increase the resolution by n bits, 4^n samples must be accumulated. The result must then be rightshifted
by n bits. This right-shift is a combination of the automatic right-shift and the value written to
AVGCTRL.ADJRES.
*/
void analogOversamplingAndDecimate (eOversamplingVal ovs)
{
  if (!ADCinitialized) {
    initADC();
  }

  eAnalogDivFact div = DIV_FACT_1;

  syncADC();
  switch (ovs) {
    case OVERSAMPLED_RES_13BIT:
      div = DIV_FACT_2;
    break;

    case OVERSAMPLED_RES_14BIT:
      div = DIV_FACT_4;
    break;

    case OVERSAMPLED_RES_15BIT:
      div = DIV_FACT_8;
    break;

    case OVERSAMPLED_RES_16BIT:
      div = DIV_FACT_16;
    break;
  }

  ADC->AVGCTRL.reg = ovs | ADC_AVGCTRL_ADJRES(div);

  syncADC();
}

#ifdef __cplusplus
}
#endif