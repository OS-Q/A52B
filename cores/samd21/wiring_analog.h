/*
  Copyright (c) 2015 Arduino LLC.  All right reserved.

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

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * \brief SAMD products have only one reference for ADC
 */
typedef enum _eAnalogReference
{
  AR_DEFAULT,
  AR_INTERNAL,
  AR_EXTERNAL,
  AR_INTERNAL1V0,
  AR_INTERNAL1V65,
  AR_INTERNAL2V23
} eAnalogReference;

typedef enum _eAnalogGain
{
  AG_0_5X,
  AG_1X,
  AG_2X,
  AG_4X,
  AG_8X,
  AG_16X,
  AG_DEFAULT
} eAnalogGain;

typedef enum _eAnalogAccumuDepth
{
  ACCUM_1_SAMPLE = ADC_AVGCTRL_SAMPLENUM_1_Val,         // intermediate result precision = 12 bits, number of auto. right shift = 0
  ACCUM_2_SAMPLE = ADC_AVGCTRL_SAMPLENUM_2_Val,         // intermediate result precision = 13 bits, number of auto. right shift = 0
  ACCUM_4_SAMPLE = ADC_AVGCTRL_SAMPLENUM_4_Val,         // intermediate result precision = 14 bits, number of auto. right shift = 0
  ACCUM_8_SAMPLE = ADC_AVGCTRL_SAMPLENUM_8_Val,         // intermediate result precision = 15 bits, number of auto. right shift = 0
  ACCUM_16_SAMPLE = ADC_AVGCTRL_SAMPLENUM_16_Val,       // intermediate result precision = 16 bits, number of auto. right shift = 0
  ACCUM_32_SAMPLE = ADC_AVGCTRL_SAMPLENUM_32_Val,       // intermediate result precision = 17 bits, number of auto. right shift = 1
  ACCUM_64_SAMPLE = ADC_AVGCTRL_SAMPLENUM_64_Val,       // intermediate result precision = 18 bits, number of auto. right shift = 2
  ACCUM_128_SAMPLE = ADC_AVGCTRL_SAMPLENUM_128_Val,     // intermediate result precision = 19 bits, number of auto. right shift = 3
  ACCUM_256_SAMPLE = ADC_AVGCTRL_SAMPLENUM_256_Val,     // intermediate result precision = 20 bits, number of auto. right shift = 4
  ACCUM_512_SAMPLE = ADC_AVGCTRL_SAMPLENUM_512_Val,     // intermediate result precision = 21 bits, number of auto. right shift = 5
  ACCUM_1024_SAMPLE = ADC_AVGCTRL_SAMPLENUM_1024_Val    // intermediate result precision = 22 bits, number of auto. right shift = 6
} eAnalogAccumuDepth;

typedef enum _eAnalogDivFact
{
  DIV_FACT_1 = 0x00,        
  DIV_FACT_2 = 0x01,        
  DIV_FACT_4 = 0x02,        
  DIV_FACT_8 = 0x03,        
  DIV_FACT_16 = 0x04
} eAnalogDivFact;

typedef enum _eOversamplingVal
{
  OVERSAMPLED_RES_13BIT = ADC_AVGCTRL_SAMPLENUM_4_Val,    // number of samples to average = 4, number of auto. right shift = 0
  OVERSAMPLED_RES_14BIT = ADC_AVGCTRL_SAMPLENUM_16_Val,   // number of samples to average = 16, number of auto. right shift = 0
  OVERSAMPLED_RES_15BIT = ADC_AVGCTRL_SAMPLENUM_64_Val,   // number of samples to average = 64, number of auto. right shift = 2
  OVERSAMPLED_RES_16BIT = ADC_AVGCTRL_SAMPLENUM_256_Val,  // number of samples to average = 256, number of auto. right shift = 4
} eOversamplingVal;

/*
 * \brief Configures the reference voltage used for analog input (i.e. the value used as the top of the input range).
 * This function is kept only for compatibility with existing AVR based API.
 *
 * \param ulMmode Should be set to AR_DEFAULT.
 */
extern void analogReference( eAnalogReference ulMode ) ;

/*
 * \brief Writes an analog value (PWM wave) to a pin.
 *
 * \param ulPin
 * \param ulValue
 */
extern void analogWrite( uint32_t ulPin, uint32_t ulValue ) ;

/*
 * \brief Writes an analog value (PWM wave) to a pin with the given operational frequency.
 *
 * \param ulPin
 * \param ulValue
 * \param ulfrequency
 */
extern void analogWriteFreq(uint32_t ulpin, uint32_t ulvalue, uint32_t ulFrequency);

/*
 * \brief Reads the value from the specified analog pin.
 *
 * \param ulPin
 *
 * \return Read value from selected pin, if no error.
 */
extern uint32_t analogRead( uint32_t ulPin ) ;

/*
 * \brief Set the resolution of analogRead return values. Default is 10 bits (range from 0 to 1023).
 *
 * \param res
 */
extern void analogReadResolution(int res);

/*
 * \brief Set the gain of analogRead acquisition. Default is 1x, other options are: 0.5x, 2x, 4x, 8x, 16x.
 *
 * \param g
 */

extern void analogReadGain(eAnalogGain g);

/*
 * \brief Set the resolution of analogWrite parameters. Default is 8 bits (range from 0 to 255).
 *
 * \param res
 */
extern void analogWriteResolution(int res);

extern void analogOutputInit( void ) ;

extern void analogPrescaler(uint8_t val);

extern void analogReadCorrection (int offset, uint16_t gain);

extern void analogHWAveraging (eAnalogAccumuDepth acc, eAnalogDivFact div);

extern void analogOversamplingAndDecimate (eOversamplingVal ovs);

// NVM Software Calibration Area Mapping, pg 32. Address starting at NVMCTRL_OTP4. 
// NVM register access code modified from https://github.com/arduino/ArduinoCore-samd/blob/master/cores/arduino/USB/samd21_host.c
// ADC Linearity Calibration value. Should be written to the CALIB register.
#define NVM_ADC_LINEARITY_POS         27
#define NVM_ADC_LINEARITY_SIZE         8
// ADC Bias Calibration value. Should be written to the CALIB register.
#define NVM_ADC_BIASCAL_POS           35
#define NVM_ADC_BIASCAL_SIZE           3

extern void analogCalibrate(void);

#ifdef __cplusplus
}
#endif
