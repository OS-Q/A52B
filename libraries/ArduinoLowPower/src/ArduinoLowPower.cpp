#include "ArduinoLowPower.h"
#include "WInterrupts.h"

#include <time.h>
#define EPOCH_TIME_OFF      946684800  // This is 1st January 2000, 00:00:00 in epoch time

const uint8_t GENERIC_CLOCK_GENERATOR_MAIN = 0;

static bool _rtcConfigured = false;
voidFuncPtr RTC_callBack = NULL;

#define DIR_R      0
#define OUT_R      1
#define CTRL_R     2
#define WRCONFIG_R 3
uint32_t portConfig[2][4];
uint8_t pmux[2][16];
uint8_t pincfg[2][32];


void beginRTC();
void disableGPIOs();
void enableGPIOs();

void ArduinoLowPowerClass::idle(bool sleepOnExit, idle_e idleMode) {
	if(sleepOnExit){
		// automatically come back in idle mode after interrupt that woke the board up is executed
		SCB->SCR |= SCB_SCR_SLEEPONEXIT_Msk;
	}
	SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
	PM->SLEEP.reg = idleMode;
	__DSB();
	__WFI();
}

void ArduinoLowPowerClass::idle(uint32_t millis, bool sleepOnExit, idle_e idleMode) {
	setAlarmIn(millis);
	idle(sleepOnExit, idleMode);
}

void ArduinoLowPowerClass::sleep(bool sleepOnExit) {
	#ifdef USB_ENABLED
		// Disable USB if enabled
		bool usbEnabled = USBDevice.isEnabled();
		if(usbEnabled)
			USBDevice.disable();
	#endif // USB_ENABLED
	// Disable systick interrupt:  See https://www.avrfreaks.net/forum/samd21-samd21e16b-sporadically-locks-and-does-not-wake-standby-sleep-mode
	SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
	if(sleepOnExit){
		// automatically come back in standby mode after interrupt that woke the board up is executed
		SCB->SCR |= SCB_SCR_SLEEPONEXIT_Msk;
	}
	SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
	__DSB();
	__WFI();
	// Enable systick interrupt
	SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;

	#ifdef USB_ENABLED
		// enable USB only if it was previously enabled
		if(usbEnabled)
			USBDevice.enable();
	#endif // USB_ENABLED
}

void ArduinoLowPowerClass::sleep(uint32_t millis, bool sleepOnExit) {
	setAlarmIn(millis);
	sleep(sleepOnExit);
}

void ArduinoLowPowerClass::deepSleep(bool sleepOnExit) {
	disableGPIOs();
	PM->APBCMASK.reg = (uint32_t) 0x0;
	SYSCTRL->BOD33.bit.ENABLE = 0;
	sleep(sleepOnExit);

	SYSCTRL->BOD33.bit.ENABLE = 1;
	// restore GPIOs state
	enableGPIOs();
	// Clock SERCOM for Serial
	PM->APBCMASK.reg |= PM_APBCMASK_SERCOM0 | PM_APBCMASK_SERCOM1 | PM_APBCMASK_SERCOM2 | PM_APBCMASK_SERCOM3 | PM_APBCMASK_SERCOM4 | PM_APBCMASK_SERCOM5 ;
	// Clock TC/TCC for Pulse and Analog
	PM->APBCMASK.reg |= PM_APBCMASK_TCC0 | PM_APBCMASK_TCC1 | PM_APBCMASK_TCC2 | PM_APBCMASK_TC3 | PM_APBCMASK_TC4 | PM_APBCMASK_TC5 ;
	// Clock ADC/DAC for Analog
	PM->APBCMASK.reg |= PM_APBCMASK_ADC | PM_APBCMASK_DAC ;
}

void ArduinoLowPowerClass::deepSleep(uint32_t millis, bool sleepOnExit) {
	disableGPIOs();
	PM->APBCMASK.reg = (uint32_t) 0x0;
	SYSCTRL->BOD33.bit.ENABLE = 0;
	sleep(millis, sleepOnExit);

	SYSCTRL->BOD33.bit.ENABLE = 1;
	// restore GPIOs state
	enableGPIOs();
	// Clock SERCOM for Serial
	PM->APBCMASK.reg |= PM_APBCMASK_SERCOM0 | PM_APBCMASK_SERCOM1 | PM_APBCMASK_SERCOM2 | PM_APBCMASK_SERCOM3 | PM_APBCMASK_SERCOM4 | PM_APBCMASK_SERCOM5 ;
	// Clock TC/TCC for Pulse and Analog
	PM->APBCMASK.reg |= PM_APBCMASK_TCC0 | PM_APBCMASK_TCC1 | PM_APBCMASK_TCC2 | PM_APBCMASK_TC3 | PM_APBCMASK_TC4 | PM_APBCMASK_TC5 ;
	// Clock ADC/DAC for Analog
	PM->APBCMASK.reg |= PM_APBCMASK_ADC | PM_APBCMASK_DAC ;
}

void ArduinoLowPowerClass::setAlarmIn(uint32_t millis) {

	if (!_rtcConfigured) {
		attachInterruptWakeup(RTC_ALARM_WAKEUP, NULL, 0);
	}

	// get epoch from RTC
	RTC->MODE2.READREQ.reg = RTC_READREQ_RREQ;
	while (RTC->MODE2.STATUS.bit.SYNCBUSY);

	RTC_MODE2_CLOCK_Type clockTime;
	clockTime.reg = RTC->MODE2.CLOCK.reg;
	struct tm tm;

	tm.tm_isdst = -1;
	tm.tm_yday = 0;
	tm.tm_wday = 0;
	tm.tm_year = clockTime.bit.YEAR + 100;
	tm.tm_mon = clockTime.bit.MONTH - 1;
	tm.tm_mday = clockTime.bit.DAY;
	tm.tm_hour = clockTime.bit.HOUR;
	tm.tm_min = clockTime.bit.MINUTE;
	tm.tm_sec = clockTime.bit.SECOND;
  
	uint32_t now = mktime(&tm);
	// set alarm
	now = now + millis/1000;
	time_t t = now;
    struct tm* tmp = gmtime(&t);
	RTC->MODE2.Mode2Alarm[0].ALARM.bit.DAY = tmp->tm_mday;
	while (RTC->MODE2.STATUS.bit.SYNCBUSY);
	RTC->MODE2.Mode2Alarm[0].ALARM.bit.MONTH = tmp->tm_mon + 1;
	while (RTC->MODE2.STATUS.bit.SYNCBUSY);
	RTC->MODE2.Mode2Alarm[0].ALARM.bit.YEAR = tmp->tm_year - 100;
	while (RTC->MODE2.STATUS.bit.SYNCBUSY);
	RTC->MODE2.Mode2Alarm[0].ALARM.bit.SECOND = tmp->tm_sec;
	while (RTC->MODE2.STATUS.bit.SYNCBUSY);
	RTC->MODE2.Mode2Alarm[0].ALARM.bit.MINUTE = tmp->tm_min;
	while (RTC->MODE2.STATUS.bit.SYNCBUSY);
	RTC->MODE2.Mode2Alarm[0].ALARM.bit.HOUR = tmp->tm_hour;
	while (RTC->MODE2.STATUS.bit.SYNCBUSY);

	// enable alarm
	RTC->MODE2.Mode2Alarm[0].MASK.bit.SEL = RTC_MODE2_MASK_SEL_HHMMSS_Val;
	while (RTC->MODE2.STATUS.bit.SYNCBUSY);
}

void ArduinoLowPowerClass::attachInterruptWakeup(uint32_t pin, voidFuncPtr callback, uint32_t mode) {

	if (pin > PINS_COUNT) {
		// check for external wakeup sources
		// RTC library should call this API to enable the alarm subsystem
		switch (pin) {
			case RTC_ALARM_WAKEUP:
				beginRTC();
				RTC_callBack = callback;
			/*case UART_WAKEUP:*/
		}
		return;
	}

	EExt_Interrupts in = g_APinDescription[pin].ulExtInt;
	if (in == NOT_AN_INTERRUPT || in == EXTERNAL_INT_NMI)
    		return;

	//pinMode(pin, INPUT_PULLUP);
	attachInterrupt(pin, callback, mode);

	// enable EIC clock
	GCLK->CLKCTRL.bit.CLKEN = 0; //disable GCLK module
	while (GCLK->STATUS.bit.SYNCBUSY);

	GCLK->CLKCTRL.reg = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK6 | GCLK_CLKCTRL_ID( GCM_EIC )) ;  //EIC clock switched on GCLK6
	while (GCLK->STATUS.bit.SYNCBUSY);

	GCLK->GENCTRL.reg = (GCLK_GENCTRL_GENEN | GCLK_GENCTRL_SRC_OSCULP32K | GCLK_GENCTRL_ID(6));  //source for GCLK6 is OSCULP32K
	while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY);

	GCLK->GENCTRL.bit.RUNSTDBY = 1;  //GCLK6 run standby
	while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY);

	// Enable wakeup capability on pin in case being used during sleep
	EIC->WAKEUP.reg |= (1 << in);

	/* Errata: Make sure that the Flash does not power all the way down
     	* when in sleep mode. */

	NVMCTRL->CTRLB.bit.SLEEPPRM = NVMCTRL_CTRLB_SLEEPPRM_DISABLED_Val;
}

void ArduinoLowPowerClass::changeClockFrequency(frequencies frequency){
	uint16_t src = GCLK_GENCTRL_SRC_DFLL48M;
	uint32_t clock = 48000000;
	uint8_t div = 1;
	switch(frequency){
		case FREQ_32MHz:
			src = GCLK_GENCTRL_SRC_DFLL48M;
			clock = 32000000;
			div = 2;
			break;
		case FREQ_16MHz:
			src = GCLK_GENCTRL_SRC_DFLL48M;
			clock = 16000000;
			div = 3;
			break;
		case FREQ_8MHz:
			src = GCLK_GENCTRL_SRC_OSC8M;
			clock = 8000000;
			div = 1;
			break;
		case FREQ_4MHz:
			src = GCLK_GENCTRL_SRC_OSC8M;
			clock = 4000000;
			div = 2;
			break;
		case FREQ_1MHz:
			src = GCLK_GENCTRL_SRC_OSC8M;
			clock = 1000000;
			div = 8;
			break;
		case FREQ_48MHz:
		default:
			break;
	}

	// set prescaler and change frequency
	GCLK->GENDIV.reg =  (GCLK_GENDIV_DIV(div) | GCLK_GENDIV_ID(GENERIC_CLOCK_GENERATOR_MAIN));
	GCLK->GENCTRL.reg = GCLK_GENCTRL_ID( GENERIC_CLOCK_GENERATOR_MAIN ) | // Generic Clock Generator 0
                      src | // Selected source
                      GCLK_GENCTRL_GENEN ;

	// change clock reference
	SystemCoreClock = clock;
	// reload systick config
	SysTick_Config(clock / 1000);
}

ArduinoLowPowerClass LowPower;

void beginRTC(){
  uint16_t tmp_reg = 0;
  
  PM->APBAMASK.reg |= PM_APBAMASK_RTC; // turn on digital interface clock
  // Configure the 32768Hz Oscillator
  SYSCTRL->XOSC32K.reg = SYSCTRL_XOSC32K_ONDEMAND |
                         SYSCTRL_XOSC32K_RUNSTDBY |
                         SYSCTRL_XOSC32K_EN32K |
                         SYSCTRL_XOSC32K_XTALEN |
                         SYSCTRL_XOSC32K_STARTUP(6) |
                         SYSCTRL_XOSC32K_ENABLE;

  // If the RTC is in clock mode and the reset was
  // not due to POR or BOD, preserve the clock time
  // POR causes a reset anyway, BOD behaviour is?
  bool validTime = false;
  RTC_MODE2_CLOCK_Type oldTime;

  if ((PM->RCAUSE.reg & (PM_RCAUSE_SYST | PM_RCAUSE_WDT | PM_RCAUSE_EXT))) {
    if (RTC->MODE2.CTRL.reg & RTC_MODE2_CTRL_MODE_CLOCK) {
      validTime = true;
      oldTime.reg = RTC->MODE2.CLOCK.reg;
    }
  }

  // Setup clock GCLK2 with OSC32K divided by 32
  GCLK->GENDIV.reg = GCLK_GENDIV_ID(2)|GCLK_GENDIV_DIV(4);
  while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY);
  GCLK->GENCTRL.reg = (GCLK_GENCTRL_GENEN | GCLK_GENCTRL_SRC_OSC32K | GCLK_GENCTRL_ID(2) | GCLK_GENCTRL_DIVSEL );
  while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY);
  GCLK->CLKCTRL.reg = (uint32_t)((GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK2 | (RTC_GCLK_ID << GCLK_CLKCTRL_ID_Pos)));
  while (GCLK->STATUS.bit.SYNCBUSY);

  RTC->MODE2.CTRL.reg &= ~RTC_MODE2_CTRL_ENABLE; // disable RTC
  while (GCLK->STATUS.bit.SYNCBUSY);

  RTC->MODE2.CTRL.reg |= RTC_MODE2_CTRL_SWRST; // software reset
  while (GCLK->STATUS.bit.SYNCBUSY);

  tmp_reg |= RTC_MODE2_CTRL_MODE_CLOCK; // set clock operating mode
  tmp_reg |= RTC_MODE2_CTRL_PRESCALER_DIV1024; // set prescaler to 1024 for MODE2
  tmp_reg &= ~RTC_MODE2_CTRL_MATCHCLR; // disable clear on match
  
  //According to the datasheet RTC_MODE2_CTRL_CLKREP = 0 for 24h
  tmp_reg &= ~RTC_MODE2_CTRL_CLKREP; // 24h time representation

  RTC->MODE2.READREQ.reg &= ~RTC_READREQ_RCONT; // disable continuously mode

  RTC->MODE2.CTRL.reg = tmp_reg;
  while (GCLK->STATUS.bit.SYNCBUSY);

  NVIC_EnableIRQ(RTC_IRQn); // enable RTC interrupt 
  NVIC_SetPriority(RTC_IRQn, 0x00);

  RTC->MODE2.INTENSET.reg |= RTC_MODE2_INTENSET_ALARM0; // enable alarm interrupt
  RTC->MODE2.Mode2Alarm[0].MASK.bit.SEL = RTC_MODE2_MASK_SEL_OFF_Val; // default alarm match is off (disabled)
  
  while (GCLK->STATUS.bit.SYNCBUSY);

  RTC->MODE2.CTRL.reg |= RTC_MODE2_CTRL_ENABLE; // enable RTC
  while (GCLK->STATUS.bit.SYNCBUSY);
  RTC->MODE2.CTRL.reg &= ~RTC_MODE2_CTRL_SWRST; // software reset remove
  while (GCLK->STATUS.bit.SYNCBUSY);

  // If valid, restore the time value, else use first valid time value
  if ((validTime) && (oldTime.reg != 0L)) {
    RTC->MODE2.CLOCK.reg = oldTime.reg;
  }
  else {
	// default is 1st January 2000, 00:00:00
    RTC->MODE2.CLOCK.reg = RTC_MODE2_CLOCK_YEAR(2000 - 2000) | RTC_MODE2_CLOCK_MONTH(1) 
        | RTC_MODE2_CLOCK_DAY(1) | RTC_MODE2_CLOCK_HOUR(0) 
        | RTC_MODE2_CLOCK_MINUTE(0) | RTC_MODE2_CLOCK_SECOND(0);
  }
  while (GCLK->STATUS.bit.SYNCBUSY);

  _rtcConfigured = true;
}

void disableGPIOs(){
	// save current GPIO configuration
	portConfig[PORTA][DIR_R] = PORT->Group[PORTA].DIR.reg;
	portConfig[PORTB][DIR_R] = PORT->Group[PORTB].DIR.reg;
	portConfig[PORTA][OUT_R] = PORT->Group[PORTA].OUT.reg;
	portConfig[PORTB][OUT_R] = PORT->Group[PORTB].OUT.reg;
	portConfig[PORTA][CTRL_R] = PORT->Group[PORTA].CTRL.reg;
	portConfig[PORTB][CTRL_R] = PORT->Group[PORTB].CTRL.reg;
	portConfig[PORTA][WRCONFIG_R] = PORT->Group[PORTA].WRCONFIG.reg;
	portConfig[PORTB][WRCONFIG_R] = PORT->Group[PORTB].WRCONFIG.reg;
	for(uint8_t i = 0; i < 2; i++)
		for(uint8_t j = 0; j < 16; j++)
			pmux[i][j] = PORT->Group[i].PMUX[j].reg;
	for(uint8_t i = 0; i < 2; i++)
		for(uint8_t j = 0; j < 32; j++)
			pincfg[i][j] = PORT->Group[i].PINCFG[j].reg;

	// Reset configuration

	// PORTA
	PORT->Group[PORTA].DIR.reg &= (uint32_t) 0x10100000;
	PORT->Group[PORTA].OUT.reg &= (uint32_t) 0x10100000;
	PORT->Group[PORTA].CTRL.reg &= (uint32_t) 0x10100000;
	PORT->Group[PORTA].WRCONFIG.reg = (uint32_t) 0x0;
	uint8_t* reg = (uint8_t*)&PORT->Group[PORTA].PMUX[0].reg;
	uint8_t i;
	for(i = 0; i < 10; i++){
		*reg = (uint8_t) 0x0;
		reg ++;
	}
	PORT->Group[PORTA].PMUX[10].reg &= (uint8_t) 0x0F;
	for(i = 11; i < 14; i++){
		*reg = (uint8_t) 0x0;
		reg ++;
	}
	PORT->Group[PORTA].PMUX[14].reg &= (uint8_t) 0x0F;
	PORT->Group[PORTA].PMUX[15].reg = (uint8_t) 0x6;
	reg = (uint8_t*)&PORT->Group[PORTA].PINCFG[0].reg;
	for(i = 0; i < 20; i++){
		*reg = (uint8_t) 0x40;
		reg++;
	}
	for(i = 21; i < 24; i++){
		*reg = (uint8_t) 0x40;
		reg++;
	}
	PORT->Group[PORTA].PINCFG[24].reg = (uint8_t) 0x0;
	PORT->Group[PORTA].PINCFG[25].reg = (uint8_t) 0x0;
	PORT->Group[PORTA].PINCFG[26].reg = (uint8_t) 0x1;
	PORT->Group[PORTA].PINCFG[27].reg = (uint8_t) 0x40;
	PORT->Group[PORTA].PINCFG[29].reg = (uint8_t) 0x1;
	PORT->Group[PORTA].PINCFG[30].reg = (uint8_t) 0x41;
	PORT->Group[PORTA].PINCFG[31].reg = (uint8_t) 0x40;

	// PORTB
	PORT->Group[PORTB].DIR.reg = (uint32_t) 0x0;
	PORT->Group[PORTB].OUT.reg = (uint32_t) 0x0;
	PORT->Group[PORTB].CTRL.reg = (uint32_t) 0x0;
	PORT->Group[PORTB].WRCONFIG.reg = (uint32_t) 0x0;
	reg = (uint8_t*)&PORT->Group[PORTB].PMUX[0].reg;
	for(uint8_t i = 0; i < 16; i++){
		*reg = (uint8_t) 0x0;
		reg ++;
	}
	reg = (uint8_t*)&PORT->Group[PORTB].PINCFG[0].reg;
	for(uint8_t i = 0; i < 19; i++){
		*reg = (uint8_t) 0x40;
		reg++;
	}
	for(; i < 22; i++){
		*reg = (uint8_t) 0x01;
		reg++;
	}
	PORT->Group[PORTB].PINCFG[22].reg = (uint8_t) 0x40;
	PORT->Group[PORTB].PINCFG[23].reg = (uint8_t) 0x40;
	for(i = 24; i < 30; i++){
		*reg = (uint8_t) 0x01;
		reg++;
	}
	PORT->Group[PORTB].PINCFG[30].reg = (uint8_t) 0x40;
	PORT->Group[PORTB].PINCFG[31].reg = (uint8_t) 0x40;
}

void enableGPIOs(){
	// Restore saved configuration
	PORT->Group[PORTA].DIR.reg = portConfig[PORTA][DIR_R];
	PORT->Group[PORTB].DIR.reg = portConfig[PORTB][DIR_R];
	PORT->Group[PORTA].OUT.reg = portConfig[PORTA][OUT_R];
	PORT->Group[PORTB].OUT.reg = portConfig[PORTB][OUT_R];
	PORT->Group[PORTA].CTRL.reg = portConfig[PORTA][CTRL_R];
	PORT->Group[PORTB].CTRL.reg = portConfig[PORTB][CTRL_R];
	PORT->Group[PORTA].WRCONFIG.reg = portConfig[PORTA][WRCONFIG_R];
	PORT->Group[PORTB].WRCONFIG.reg = portConfig[PORTB][WRCONFIG_R];
	for(uint8_t i = 0; i < 2; i++)
		for(uint8_t j = 0; j < 16; j++)
			PORT->Group[i].PMUX[j].reg = pmux[i][j];
	for(uint8_t i = 0; i < 2; i++)
		for(uint8_t j = 0; j < 32; j++)
			PORT->Group[i].PINCFG[j].reg = pincfg[i][j];
}

void RTC_Handler(void)
{
  if (RTC_callBack != NULL) {
    RTC_callBack();
  }

  RTC->MODE2.INTFLAG.reg = RTC_MODE2_INTFLAG_ALARM0; // must clear flag at end
}