/*
  Meteca SA.  All right reserved.
  created by Chiara Ruggeri
  email: support@meteca.org

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "Arduino.h"

extern bool tcEnabled[TCC_INST_NUM+TC_INST_NUM]; // used in wiring_analog.c for pwm management
void (*callbacks[TCC_INST_NUM+TC_INST_NUM])() = {NULL, NULL, NULL, NULL, NULL, NULL};
bool usedTimers[TCC_INST_NUM+TC_INST_NUM];
bool toneUsed = false;


// handlers
#ifdef __cplusplus
extern "C"{
#endif

void Tone_Handler (void);

void TCC0_Handler (void) __attribute__ ((weak));
void TCC0_Handler(void){
	if(callbacks[0] != NULL)
		callbacks[0]();
	Tcc* TCC = (Tcc*) TCC0;
	TCC->INTFLAG.bit.OVF = 1;
}

void TCC1_Handler (void) __attribute__ ((weak));
void TCC1_Handler(void){
	if(callbacks[1] != NULL)
		callbacks[1]();
	Tcc* TCC = (Tcc*) TCC1;
	TCC->INTFLAG.bit.OVF = 1;
}

void TCC2_Handler (void) __attribute__ ((weak));
void TCC2_Handler(void){
	if(callbacks[2] != NULL)
		callbacks[2]();
	Tcc* TCC = (Tcc*) TCC2;
	TCC->INTFLAG.bit.OVF = 1;
}

void TC3_Handler (void) __attribute__ ((weak));
void TC3_Handler(void){
	if(callbacks[3] != NULL)
		callbacks[3]();
	Tc* TC = (Tc*) TC3;
	TC->COUNT16.INTFLAG.bit.MC0 = 1;
}

void TC4_Handler (void) __attribute__ ((weak));
void TC4_Handler(void){
	if(callbacks[4] != NULL)
		callbacks[4]();
	Tc* TC = (Tc*) TC4;
	TC->COUNT16.INTFLAG.bit.MC0 = 1;
}

void TC5_Handler (void) __attribute__ ((weak));
void TC5_Handler(void){
	if(toneUsed)
		Tone_Handler();
	else{
		if(callbacks[5] != NULL)
			callbacks[5]();
		Tc* TC = (Tc*) TC5;
		TC->COUNT16.INTFLAG.bit.MC0 = 1;
	}
}

#ifdef __cplusplus
}
#endif


uint8_t availableTimers(){
	uint8_t available = 0;
	for(uint8_t i = 0; i < TCC_INST_NUM+TC_INST_NUM; i++){
		if(!tcEnabled[i] && !usedTimers[i])
			available++;
	}
	return available;
}

int8_t createTimer(uint32_t n_micros, void (*callback)()){
	// get first available timer instance
	int8_t tcNum = -1;
	for(uint8_t i = 0; i < TCC_INST_NUM+TC_INST_NUM; i++){
		if(!tcEnabled[i]){
			if(!usedTimers[i]){
				tcNum = i;
				break;
			}
		}
	}
	if(tcNum < 0) // no timer available
		return -1;

	uint16_t GCLK_CLKCTRL_IDs[] = {
        GCLK_CLKCTRL_ID(GCM_TCC0_TCC1), // TCC0
        GCLK_CLKCTRL_ID(GCM_TCC0_TCC1), // TCC1
        GCLK_CLKCTRL_ID(GCM_TCC2_TC3),  // TCC2
        GCLK_CLKCTRL_ID(GCM_TCC2_TC3),  // TC3
        GCLK_CLKCTRL_ID(GCM_TC4_TC5),   // TC4
        GCLK_CLKCTRL_ID(GCM_TC4_TC5),   // TC5
	};
	
	IRQn_Type TC_IRQn[] = {
        TCC0_IRQn, // TCC0
        TCC1_IRQn, // TCC1
        TCC2_IRQn, // TCC2
        TC3_IRQn,  // TC3
        TC4_IRQn,  // TC4
        TC5_IRQn,  // TC5
	};

	if(tcNum >= TCC_INST_NUM){
		// TC instance
		Tc* TCx = (Tc*)	g_apTCInstances[tcNum];

		GCLK->CLKCTRL.reg = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_IDs[tcNum]);
		while (GCLK->STATUS.bit.SYNCBUSY == 1);

		TCx->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
		// Set Timer counter Mode to 16 bits, match mode (timer resets when the count matches CC register)
		TCx->COUNT16.CTRLA.reg |= TC_CTRLA_MODE_COUNT16 | TC_CTRLA_WAVEGEN_MFRQ;
		while (TCx->COUNT16.STATUS.bit.SYNCBUSY == 1);

		uint16_t prescaler = 1;
		if(n_micros > 300000) {
			TCx->COUNT16.CTRLA.bit.PRESCALER = 0x07; // prescaler 1024
			prescaler = 1024;
		}
		else if(n_micros > 80000) {
			TCx->COUNT16.CTRLA.bit.PRESCALER = 0x06; // prescaler 256
			prescaler = 256;
		}
		else if(n_micros > 20000) {
			TCx->COUNT16.CTRLA.bit.PRESCALER = 0x05; // prescaler 64
			prescaler = 64;
		}
		else if(n_micros > 10000) {
			TCx->COUNT16.CTRLA.bit.PRESCALER = 0x04; // prescaler 16
			prescaler = 16;
		}
		else if(n_micros > 5000) {
			TCx->COUNT16.CTRLA.bit.PRESCALER = 0x03; // prescaler 8
			prescaler = 8;
		}
		else if(n_micros > 2500) {
			TCx->COUNT16.CTRLA.bit.PRESCALER = 0x02; // prescaler 4
			prescaler = 4;
		}
		else if(n_micros > 1000) {
			TCx->COUNT16.CTRLA.bit.PRESCALER = 0x01; // prescaler 2
			prescaler = 2;
		}
		else{
			TCx->COUNT16.CTRLA.bit.PRESCALER = 0x00; // prescaler 1
			prescaler = 1;
		}
		while (TCx->COUNT16.STATUS.bit.SYNCBUSY == 1);

		uint16_t cc = (SystemCoreClock / ((float)prescaler / ((float)n_micros / 1000000))) - 1;
		// Make sure the count is in a proportional position to where it was
		// to prevent any jitter or disconnect when changing the compare value.
		TCx->COUNT16.COUNT.reg = map(TCx->COUNT16.COUNT.reg, 0, TCx->COUNT16.CC[0].reg, 0, cc);
		TCx->COUNT16.CC[0].reg = cc;
		while (TCx->COUNT16.STATUS.bit.SYNCBUSY == 1);

		// Enable interrupt
		TCx->COUNT16.INTENSET.reg = 0;
		TCx->COUNT16.INTENSET.bit.MC0 = 1;
		callbacks[tcNum] = callback;

		NVIC_EnableIRQ(TC_IRQn[tcNum]);

		// Do not enable timer yet
		//TCx->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
		//while (TCx->COUNT16.STATUS.bit.SYNCBUSY == 1);
	}
	else{
		// TCC instance
		Tcc* TCCx = (Tcc*) g_apTCInstances[tcNum];

		REG_GCLK_CLKCTRL = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_IDs[tcNum]);
		while ( GCLK->STATUS.bit.SYNCBUSY == 1 );

		TCCx->CTRLA.reg &= ~TCC_CTRLA_ENABLE;
		while (TCCx->SYNCBUSY.bit.ENABLE == 1);
	
		uint16_t prescaler;
		if(n_micros > 300000) {
			TCCx->CTRLA.bit.PRESCALER = 0x07; // prescaler 1024
			prescaler = 1024;
		}
		else if(n_micros > 80000) {
			TCCx->CTRLA.bit.PRESCALER = 0x06; // prescaler 256
			prescaler = 256;
		}
		else if(n_micros > 20000) {
			TCCx->CTRLA.bit.PRESCALER = 0x05; // prescaler 64
			prescaler = 64;
		}
		else if(n_micros > 10000) {
			TCCx->CTRLA.bit.PRESCALER = 0x04; // prescaler 16
			prescaler = 16;
		}
		else if(n_micros > 5000) {
			TCCx->CTRLA.bit.PRESCALER = 0x03; // prescaler 8
			prescaler = 8;
		}
		else if(n_micros > 2500) {
			TCCx->CTRLA.bit.PRESCALER = 0x02; // prescaler 4
			prescaler = 4;
		}
		else if(n_micros > 1000) {
			TCCx->CTRLA.bit.PRESCALER = 0x01; // prescaler 2
			prescaler = 2;
		}
		else{
			TCCx->CTRLA.bit.PRESCALER = 0x00; // prescaler 1
			prescaler = 1;
		}

		uint32_t per = (int)(SystemCoreClock / ((float)prescaler / ((float)n_micros / 1000000))) - 1;
		TCCx->PER.reg = per; 
		while (TCCx->SYNCBUSY.bit.PER == 1);

		TCCx->CC[0].reg = 0xFFF;
		while (TCCx->SYNCBUSY.bit.CC0 == 1);

		// Use match mode so that the timer counter resets when the count matches the compare register
		TCCx->WAVE.reg |= TCC_WAVE_WAVEGEN_NFRQ;   // Set wave form configuration 
		while (TCCx->SYNCBUSY.bit.WAVE == 1); // wait for sync 

		// Enable the compare interrupt
		TCCx->INTENSET.reg = 0;
		TCCx->INTENSET.bit.OVF = 1;
		callbacks[tcNum] = callback;

		NVIC_EnableIRQ(TC_IRQn[tcNum]);

		// Do not enable timer yet
		//TCCx->CTRLA.reg |= TCC_CTRLA_ENABLE ;
		//while (TCCx->SYNCBUSY.bit.ENABLE == 1); // wait for sync
	}

	usedTimers[tcNum] = true;
	return tcNum;
}

void startTimer(uint8_t timerId){
	if(timerId > TCC_INST_NUM+TC_INST_NUM)
		// wrong ID number
		return;

	if(timerId >= TCC_INST_NUM){
		// TC instance
		Tc* TCx = (Tc*)	g_apTCInstances[timerId];
		// Enable timer
		TCx->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
		while (TCx->COUNT16.STATUS.bit.SYNCBUSY == 1);
	}
	else{
		// TCC instance
		Tcc* TCCx = (Tcc*) g_apTCInstances[timerId];
		// Enable timer
		TCCx->CTRLA.reg |= TCC_CTRLA_ENABLE ;
		while (TCCx->SYNCBUSY.bit.ENABLE == 1);
	}
}

void stopTimer(uint8_t timerId){
		if(timerId > TCC_INST_NUM+TC_INST_NUM)
		// wrong ID number
		return;

	if(timerId >= TCC_INST_NUM){
		// TC instance
		Tc* TCx = (Tc*)	g_apTCInstances[timerId];
		// Enable timer
		TCx->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
		while (TCx->COUNT16.STATUS.bit.SYNCBUSY == 1);
	}
	else{
		// TCC instance
		Tcc* TCCx = (Tcc*) g_apTCInstances[timerId];
		// Enable timer
		TCCx->CTRLA.reg &= ~TCC_CTRLA_ENABLE ;
		while (TCCx->SYNCBUSY.bit.ENABLE == 1);
	}
}

void destroyTimer(uint8_t timerId){
	if(timerId > TCC_INST_NUM+TC_INST_NUM)
		// wrong ID number
		return;

	IRQn_Type TC_IRQn[] = {
        TCC0_IRQn, // TCC0
        TCC1_IRQn, // TCC1
        TCC2_IRQn, // TCC2
        TC3_IRQn,  // TC3
        TC4_IRQn,  // TC4
        TC5_IRQn,  // TC5
	};

	if(timerId >= TCC_INST_NUM){
		// TC instance
		Tc* TCx = (Tc*)	g_apTCInstances[timerId];
		// reset and disable timer
		TCx->COUNT16.CTRLA.bit.SWRST = 1;
	}
	else{
		// TCC instance
		Tcc* TCCx = (Tcc*) g_apTCInstances[timerId];
		// reset and disable timer
		TCCx->CTRLA.bit.SWRST = 1;
	}

	NVIC_DisableIRQ(TC_IRQn[timerId]);
	callbacks[timerId] = NULL;
	usedTimers[timerId] = false;
}

void allocateTimer(uint8_t timerId){
	if(timerId > TCC_INST_NUM+TC_INST_NUM)
		// wrong ID number
		return;

	usedTimers[timerId] = true;
}

bool isTimerUsed(uint8_t timerId){
	if(timerId > TCC_INST_NUM+TC_INST_NUM)
		// wrong ID number
		return false;

	// check if timer is used as PWM
	if(tcEnabled[timerId])
		return true;

	// check if timer is already allocated
	if(usedTimers[timerId])
		return true;

	// free timer
	return false;
}