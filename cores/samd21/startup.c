/*
	Meteca SA.  All right reserved.
	created by Dario Trimarchi and Chiara Ruggeri 2020
	email: support@meteca.org

	based on the project: dfll48m-initialization from Microchip
	Copyright (C) 2018 Microchip Technology Inc. and its subsidiaries
	Date: June 30, 2018

	This source code is distributed under creative commons license CC BY-NC-SA
	https://creativecommons.org/licenses/by-nc-sa/4.0/
	You are free to redistribute this code and/or modify it under
	the following terms:
	- Attribution: You must give appropriate credit, provide a link
	to the license, and indicate if changes were made.
	- Non Commercial: You may not use the material for commercial purposes.
	- Share Alike:  If you remix, transform, or build upon the material,
	you must distribute your contributions under the same license as the original.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "sam.h"
#include "variant.h"

#include <stdio.h>

/**
 * \brief SystemInit() configures the needed clocks and according Flash Read Wait States.
 * At reset:
 * - OSC8M clock source is enabled with a divider by 8 (1MHz).
 * - Generic Clock Generator 0 (GCLKMAIN) is using OSC8M as source.
 * We need to:
 * 1) Enable XOSC32K clock (External on-board 32.768Hz oscillator), will be used as DFLL48M reference.
 * 2) Put XOSC32K as source of Generic Clock Generator 1
 * 3) Put Generic Clock Generator 1 as source for Generic Clock Multiplexer 0 (DFLL48M reference)
 * 4) Enable DFLL48M clock
 * 5) Switch Generic Clock Generator 0 to DFLL48M. CPU will run at 48MHz.
 * 6) Modify PRESCaler value of OSCM to have 8MHz
 * 7) Put OSC8M as source for Generic Clock Generator 3
 */
// Constants for Clock generators
#define GENERIC_CLOCK_GENERATOR_MAIN 		(0u)
#define GENERIC_CLOCK_GENERATOR_XOSC32K 	(1u)
#define GENERIC_CLOCK_GENERATOR_OSC32K 		(1u)
#define GENERIC_CLOCK_GENERATOR_OSCULP32K 	(2u) /* Initialized at reset for WDT */
#define GENERIC_CLOCK_GENERATOR_OSC8M 		(3u)
// Constants for Clock multiplexers
#define GENERIC_CLOCK_MULTIPLEXER_DFLL48M 	(0u)

void SystemInit(void)
{
	uint32_t tempDFLL48CalibrationCoarse; // used to retrieve DFLL48 coarse calibration value from NVM

#ifndef VERY_LOW_POWER
	/* Set 1 Flash Wait State for 48MHz, cf tables 20.9 and 35.27 in SAMD21 Datasheet */
	NVMCTRL->CTRLB.bit.RWS = NVMCTRL_CTRLB_RWS_HALF_Val; /* 1 wait state required @ 3.3V & 48MHz */

	/* Turn on the digital interface clock */
	PM->APBAMASK.reg |= PM_APBAMASK_GCLK;
#else
	NVMCTRL_CTRLB_Type nvm_regb = {
		.bit.CACHEDIS = 0, //cache enabled
		.bit.READMODE = NVMCTRL_CTRLB_READMODE_LOW_POWER_Val,
		.bit.SLEEPPRM = NVMCTRL_CTRLB_SLEEPPRM_WAKEUPINSTANT_Val,
		.bit.RWS = 3 // 3 wait states
	};

	NVMCTRL->CTRLB.reg = nvm_regb.reg;
#endif // VERY_LOW_POWER

#if defined(CRYSTALLESS)
/* ----------------------------------------------------------------------------------------------
* 1) Enable OSC32K clock (Internal 32.768Hz oscillator)
*/
#	ifndef VERY_LOW_POWER
	uint32_t calib = (*((uint32_t *)FUSES_OSC32K_CAL_ADDR) & FUSES_OSC32K_CAL_Msk) >> FUSES_OSC32K_CAL_Pos;

	SYSCTRL->OSC32K.reg = SYSCTRL_OSC32K_CALIB(calib) |
						  SYSCTRL_OSC32K_STARTUP(0x6u) | // cf table 15.10 of product datasheet in chapter 15.8.6
						  SYSCTRL_OSC32K_EN32K |
						  SYSCTRL_OSC32K_ENABLE;

	// Wait for oscillator stabilization
	while ((SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_OSC32KRDY) == 0);

#	endif // VERY_LOW_POWER
#else  // has crystal

/* ----------------------------------------------------------------------------------------------
* 1) Enable XOSC32K clock (External on-board 32.768Hz oscillator)
*/
#	ifndef VERY_LOW_POWER
	SYSCTRL_XOSC32K_Type sysctrl_xosc32k = {
		.bit.WRTLOCK = 0,	/* XOSC32K configuration is not locked */
		.bit.STARTUP = 0x2, /* 3 cycle start-up time */
		.bit.ONDEMAND = 0,	/* Osc. is always running when enabled */
		.bit.RUNSTDBY = 0,	/* Osc. is disabled in standby sleep mode */
		.bit.AAMPEN = 0,	/* Disable automatic amplitude control */
		.bit.EN32K = 1,		/* 32kHz output is disabled */
		.bit.XTALEN = 1		/* Crystal connected to XIN32/XOUT32 */
	};

	SYSCTRL->XOSC32K.reg = sysctrl_xosc32k.reg;
	SYSCTRL->XOSC32K.bit.ENABLE = 1; /* separate call, as described in chapter 15.6.3 */

	// Wait for XOSC32K to stabilize
	while (!SYSCTRL->PCLKSR.bit.XOSC32KRDY);
#	endif // VERY_LOW_POWER
#endif // CRYSTALLESS

/* ----------------------------------------------------------------------------------------------
* 2) Put XOSC32K as source of Generic Clock Generator 1
*/
#ifndef VERY_LOW_POWER
	// Set the Generic Clock Generator 1 output divider to 1
	// Configure GCLK->GENDIV settings
	GCLK_GENDIV_Type gclk1_gendiv = {
		.bit.DIV = 1,							  /* Set output division factor = 1 */
		.bit.ID = GENERIC_CLOCK_GENERATOR_XOSC32K /* Apply division factor to Generator 1 */
	};
	GCLK->GENDIV.reg = gclk1_gendiv.reg;

	// Configure Generic Clock Generator 1 with XOSC32K as source
	GCLK_GENCTRL_Type gclk1_genctrl = {
		.bit.RUNSTDBY = 0, /* Generic Clock Generator is stopped in stdby */
		.bit.DIVSEL = 0,   /* Use GENDIV.DIV value to divide the generator */
		.bit.OE = 0,	   /* Disable generator output to GCLK_IO[1] */
		.bit.OOV = 0,	   /* GCLK_IO[1] output value when generator is off */
		.bit.IDC = 1,	   /* Generator duty cycle is 50/50 */
		.bit.GENEN = 1,	   /* Enable the generator */
#if defined(CRYSTALLESS)
		.bit.SRC = GCLK_GENCTRL_SRC_OSC32K_Val,	 /* Generator source: XOSC32K output = 0x05 */
		.bit.ID = GENERIC_CLOCK_GENERATOR_OSC32K /* Generator ID: 1 */
#else
		.bit.SRC = GCLK_GENCTRL_SRC_XOSC32K_Val,  /* Generator source: XOSC32K output = 0x05 */
		.bit.ID = GENERIC_CLOCK_GENERATOR_XOSC32K /* Generator ID: 1 */
#endif
	};
	// Write these settings
	GCLK->GENCTRL.reg = gclk1_genctrl.reg;
	// GENCTRL is Write-Synchronized...so wait for write to complete
	while (GCLK->STATUS.bit.SYNCBUSY)
		;
#else
	GCLK_GENDIV_Type gclk1_gendiv = {
		.bit.DIV = 1,						 /* Set output division factor = 1 */
		.bit.ID = GCLK_CLKCTRL_GEN_GCLK1_Val /* Apply division factor to Generator 1 */
	};
	// Write these settings
	GCLK->GENDIV.reg = gclk1_gendiv.reg;

	// Configure Generic Clock Generator 1 with XOSC32K as source
	GCLK_GENCTRL_Type gclk1_genctrl = {
		.bit.RUNSTDBY = 0,						   /* Generic Clock Generator is stopped in stdby */
		.bit.DIVSEL = 0,						   /* Use GENDIV.DIV value to divide the generator */
		.bit.OE = 0,							   /* Disable generator output to GCLK_IO[1] */
		.bit.OOV = 0,							   /* GCLK_IO[1] output value when generator is off */
		.bit.IDC = 1,							   /* Generator duty cycle is 50/50 */
		.bit.GENEN = 1,							   /* Enable the generator */
		.bit.SRC = GCLK_GENCTRL_SRC_OSCULP32K_Val, /* Generator source: OSCULP32K output */
		.bit.ID = GCLK_CLKCTRL_GEN_GCLK1_Val	   /* Generator ID: 1 */
	};
	// Write these settings
	GCLK->GENCTRL.reg = gclk1_genctrl.reg;
	// GENCTRL is Write-Synchronized...so wait for write to complete
	while (GCLK->STATUS.bit.SYNCBUSY)
		;
#endif // VERY_LOW_POWER

/* ----------------------------------------------------------------------------------------------
* 3) Put Generic Clock Generator 1 as source for Generic Clock Multiplexer 0 (DFLL48M reference)
*/
	GCLK_CLKCTRL_Type gclk_clkctrl = {
		.bit.WRTLOCK = 0,					   /* Generic Clock is not locked from subsequent writes */
		.bit.CLKEN = 1,						   /* Enable the Generic Clock */
		.bit.GEN = GCLK_CLKCTRL_GEN_GCLK1_Val, /* Generic Clock Generator 1 is the source = 0x01*/
		.bit.ID = GCLK_CLKCTRL_ID_DFLL48_Val   /* Generic Clock Multiplexer 0 (DFLL48M Reference) */
	};
	// Write these settings
	GCLK->CLKCTRL.reg = gclk_clkctrl.reg;

/* ----------------------------------------------------------------------------------------------
* 4) Enable DFLL48M clock
* DFLL Configuration in Closed Loop mode, cf product data sheet chapter
* 17.6.7.1 - Closed-Loop Operation
* Enable the DFLL48M in open loop mode. Without this step, attempts to go into closed loop mode at 48 MHz will
* result in Processor Reset (you'll be at the in the Reset_Handler in startup_samd21.c).
* PCLKSR.DFLLRDY must be one before writing to the DFLL Control register
* Note that the DFLLRDY bit represents status of register synchronization - NOT clock stability
* (see Data Sheet 17.6.14 Synchronization for detail)
*/

/* Remove the OnDemand mode, Bug http://avr32.icgroup.norway.atmel.com/bugzilla/show_bug.cgi?id=9905 */
	while (!SYSCTRL->PCLKSR.bit.DFLLRDY)
		;
	SYSCTRL->DFLLCTRL.reg = (uint16_t)(SYSCTRL_DFLLCTRL_ENABLE);
	while (!SYSCTRL->PCLKSR.bit.DFLLRDY)
		;

	// Set up the Multiplier, Coarse and Fine steps
	SYSCTRL_DFLLMUL_Type sysctrl_dfllmul = {
		.bit.CSTEP = 31,  /* Coarse step - use half of the max value (63) */
		.bit.FSTEP = 511, /* Fine step - use half of the max value (1023) */
		.bit.MUL = 1465	  /* Multiplier = MAIN_CLK_FREQ (48MHz) / EXT_32K_CLK_FREQ (32768 Hz) */
	};
	// Write these settings
	SYSCTRL->DFLLMUL.reg = sysctrl_dfllmul.reg;
	// Wait for synchronization
	while (!SYSCTRL->PCLKSR.bit.DFLLRDY)
		;

	// To reduce lock time, load factory calibrated values into DFLLVAL (cf. Data Sheet 17.6.7.1)
	// Location of value is defined in Data Sheet Table 10-5. NVM Software Calibration Area Mapping

	// Get factory calibrated value for "DFLL48M COARSE CAL" from NVM Software Calibration Area
	tempDFLL48CalibrationCoarse = *(uint32_t *)FUSES_DFLL48M_COARSE_CAL_ADDR;
	tempDFLL48CalibrationCoarse &= FUSES_DFLL48M_COARSE_CAL_Msk;
	tempDFLL48CalibrationCoarse = tempDFLL48CalibrationCoarse >> FUSES_DFLL48M_COARSE_CAL_Pos;
	// Write the coarse calibration value
	SYSCTRL->DFLLVAL.bit.COARSE = tempDFLL48CalibrationCoarse;
	// Switch DFLL48M to Closed Loop mode and enable WAITLOCK
	while (!SYSCTRL->PCLKSR.bit.DFLLRDY)
		;
	SYSCTRL->DFLLCTRL.reg |= (uint16_t)(SYSCTRL_DFLLCTRL_MODE | SYSCTRL_DFLLCTRL_WAITLOCK);

/* ----------------------------------------------------------------------------------------------
* 5) Switch Generic Clock Generator 0 to DFLL48M. CPU will run at 48MHz.
*/
// Now that DFLL48M is running, switch CLKGEN0 source to it to run the core at 48 MHz.
// Enable output of Generic Clock Generator 0 (GCLK_MAIN) to the GCLK_IO[0] GPIO Pin
	GCLK_GENCTRL_Type gclk_genctrl0 = {
		.bit.RUNSTDBY = 0,						 /* Generic Clock Generator is stopped in standby */
		.bit.DIVSEL = 0,						 /* Use GENDIV.DIV value to divide the generator */
		.bit.OE = 0,							 /* Disable generator output to GCLK_IO[0]; to enable put this bit to 1 */
		.bit.OOV = 0,							 /* GCLK_IO[0] output value when generator is off */
		.bit.IDC = 1,							 /* Generator duty cycle is 50/50 */
		.bit.GENEN = 1,							 /* Enable the generator */
		.bit.SRC = GCLK_GENCTRL_SRC_DFLL48M_Val, /* Generator source: DFLL48M output = 0x07*/
		.bit.ID = GENERIC_CLOCK_GENERATOR_MAIN	 /* Generator ID: 0 */
	};
	GCLK->GENCTRL.reg = gclk_genctrl0.reg;
	// GENCTRL is Write-Synchronized...so wait for write to complete
	while (GCLK->STATUS.bit.SYNCBUSY)
		;

	/*
	// Direct the GCLK_IO[0] output to PA27 - uncomment this section to output the main clock to the pin
	PORT_WRCONFIG_Type port0_wrconfig = {
		.bit.HWSEL = 1,			// Pin# (27) - falls in the upper half of the 32-pin PORT group
		.bit.WRPINCFG = 1,		// Update PINCFGy registers for all pins selected
		.bit.WRPMUX = 1,		// Update PMUXn registers for all pins selected
		.bit.PMUX = 7,			// Peripheral Function H selected (GCLK_IO[0])
		.bit.PMUXEN = 1,		// Enable peripheral Multiplexer
		.bit.PINMASK = (uint16_t)(1 << (27-16)) /* Select the pin(s) to be configured
	};
	// Write these settings
	PORT->Group[0].WRCONFIG.reg = port0_wrconfig.reg;
  */

/* ----------------------------------------------------------------------------------------------
* 6) Modify PRESCaler value of OSC8M to have 8MHz
*/
  #ifndef VERY_LOW_POWER
	SYSCTRL->OSC8M.bit.PRESC = SYSCTRL_OSC8M_PRESC_0_Val; //CMSIS 4.5 changed the prescaler defines
	SYSCTRL->OSC8M.bit.ONDEMAND = 0;

/* ----------------------------------------------------------------------------------------------
* 7) Put OSC8M as source for Generic Clock Generator 3
*/
	// Set the Generic Clock Generator 3 output divider to 1
	// Configure GCLK->GENDIV settings
	GCLK_GENDIV_Type gclk3_gendiv = {
		.bit.DIV = 1,							/* Set output division factor = 1 */
		.bit.ID = GENERIC_CLOCK_GENERATOR_OSC8M /* Apply division factor to Generator 3 */
	};
	// Write these settings
	GCLK->GENDIV.reg = gclk3_gendiv.reg;

	// Configure Generic Clock Generator 3 with OSC8M as source
	GCLK_GENCTRL_Type gclk3_genctrl = {
		.bit.RUNSTDBY = 0,						/* Generic Clock Generator is stopped in stdby */
		.bit.DIVSEL = 0,						/* Use GENDIV.DIV value to divide the generator */
		.bit.OE = 0,							/* Disable generator output to GCLK_IO[1] */
		.bit.OOV = 0,							/* GCLK_IO[2] output value when generator is off */
		.bit.IDC = 1,							/* Generator duty cycle is 50/50 */
		.bit.GENEN = 1,							/* Enable the generator */
		.bit.SRC = GCLK_GENCTRL_SRC_OSC8M_Val,	/* Generator source: OSC8M output = 0x06 */
		.bit.ID = GENERIC_CLOCK_GENERATOR_OSC8M /* Generator ID: 3 */
	};
	// Write these settings
	GCLK->GENCTRL.reg = gclk3_genctrl.reg;
	// GENCTRL is Write-Synchronized...so wait for write to complete
	while (GCLK->STATUS.bit.SYNCBUSY)
		;
	#endif

/* ----------------------------------------------------------------------------------------------
* 8) Set CPU and APBx BUS Clocks to 48MHz
*/
	/*
   * Now that all system clocks are configured, we can set CPU and APBx BUS clocks.
   * There values are normally the one present after Reset.
   */
	PM->CPUSEL.reg = PM_CPUSEL_CPUDIV_DIV1;
	PM->APBASEL.reg = PM_APBASEL_APBADIV_DIV1_Val;
	PM->APBBSEL.reg = PM_APBBSEL_APBBDIV_DIV1_Val;
	PM->APBCSEL.reg = PM_APBCSEL_APBCDIV_DIV1_Val;

	SystemCoreClock = VARIANT_MCK;

/* ----------------------------------------------------------------------------------------------
* 9) Load ADC factory calibration values
*/

	// ADC Bias Calibration
	uint32_t bias = (*((uint32_t *)ADC_FUSES_BIASCAL_ADDR) & ADC_FUSES_BIASCAL_Msk) >> ADC_FUSES_BIASCAL_Pos;

	// ADC Linearity bits 4:0
	uint32_t linearity = (*((uint32_t *)ADC_FUSES_LINEARITY_0_ADDR) & ADC_FUSES_LINEARITY_0_Msk) >> ADC_FUSES_LINEARITY_0_Pos;

	// ADC Linearity bits 7:5
	linearity |= ((*((uint32_t *)ADC_FUSES_LINEARITY_1_ADDR) & ADC_FUSES_LINEARITY_1_Msk) >> ADC_FUSES_LINEARITY_1_Pos) << 5;

	ADC->CALIB.reg = ADC_CALIB_BIAS_CAL(bias) | ADC_CALIB_LINEARITY_CAL(linearity);

/* ----------------------------------------------------------------------------------------------
* 10) Disable automatic NVM write operations
*/
	NVMCTRL->CTRLB.bit.MANW = 1;
}
