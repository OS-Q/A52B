#ifndef _ARDUINO_LOW_POWER_H_
#define _ARDUINO_LOW_POWER_H_

#include <Arduino.h>

#if defined BRIKI_MBC_WB_ESP
#error "Sorry, LowPower library doesn't work with esp32 processor"
#endif

#define RTC_ALARM_WAKEUP	0xFF

//typedef void (*voidFuncPtr)( void ) ;
typedef void (*onOffFuncPtr)( bool ) ;

typedef enum frequencies {
	FREQ_48MHz,
	FREQ_32MHz,
	FREQ_16MHz,
	FREQ_8MHz,
	FREQ_4MHz,
	FREQ_1MHz
};

/*
* Sleep Mode Control - 16.6.2.8 paragraph on the DS
*______________________________________________________________________________________________________________________________________________
* Sleep Mode | CPU Clock | AHB Clock | APB Clock |                    Oscillators                    | Main Clock | Regulator Mode | RAM Mode  |
*            |           |           |           |       ONDEMAND = 0      |       ONDEMAND = 1      |            |                |           |
* ___________|___________|___________|___________|_RUNSTDBY=0_|_RUNSTDBY=1_|_RUNSTDBY=0_|_RUNSTDBY=1_|____________|________________|___________|
*   IDLE_0   |   Stop    |   Run     |   Run     |    Run     |    Run     | Run if req.| Run if req.|    Run     |     Normal     |  Normal   |
*   IDLE_1   |   Stop    |   Stop    |   Run     |    Run     |    Run     | Run if req.| Run if req.|    Run     |     Normal     |  Normal   |
*   IDLE_2   |   Stop    |   Stop    |   Stop    |    Run     |    Run     | Run if req.| Run if req.|    Run     |     Normal     |  Normal   |
*   STANDBY  |   Stop    |   Stop    |   Stop    |    Stop    |    Run     |    Stop    | Run if req.|    Stop    |   Low Power    | Low Power |
*____________|___________|___________|___________|____________|____________|____________|____________|____________|________________|___________|
*/
typedef enum idle_e {
	IDLE_0,			
	IDLE_1,
	IDLE_2
};

class ArduinoLowPowerClass {
	public:
		void idle(bool sleepOnExit = false, idle_e idleMode = IDLE_2);
		void idle(uint32_t millis, bool sleepOnExit = false, idle_e idleMode = IDLE_2);
		void idle(int millis, bool sleepOnExit = false, idle_e idleMode = IDLE_2) {
			idle((uint32_t)millis, sleepOnExit, idleMode);
		}

		void sleep(bool sleepOnExit = false);
		void sleep(uint32_t millis, bool sleepOnExit = false);
		void sleep(int millis, bool sleepOnExit = false) {
			sleep((uint32_t)millis, sleepOnExit);
		}

		// wrap call to sleep function
		void deepSleep(bool sleepOnExit = false);
		void deepSleep(uint32_t millis, bool sleepOnExit = false);
		void deepSleep(int millis, bool sleepOnExit = false) {
			deepSleep((uint32_t)millis, sleepOnExit);
		}

		void attachInterruptWakeup(uint32_t pin, voidFuncPtr callback, uint32_t mode);

		void changeClockFrequency(frequencies frequency);

	private:
		void setAlarmIn(uint32_t millis);
};

extern ArduinoLowPowerClass LowPower;

#endif