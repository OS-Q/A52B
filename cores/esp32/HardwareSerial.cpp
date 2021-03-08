#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "variant.h"
#include "HardwareSerial.h"
#include "communication.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#ifndef RX1
#define RX1 9
#endif

#ifndef TX1
#define TX1 10
#endif

#ifndef RX2
#define RX2 16
#endif

#ifndef TX2
#define TX2 17
#endif

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_SERIAL)
HardwareSerial Serial(0);
#ifdef SERIAL_1_ENABLED
	HardwareSerial Serial1(1);
#endif // SERIAL_1_ENABLED
#ifdef SERIAL_2_ENABLED
	HardwareSerial Serial2(2);
#endif //SERIAL_2_ENABLED
#endif

extern SemaphoreHandle_t UARTmutex;

HardwareSerial::HardwareSerial(int uart_nr) : _uart_nr(uart_nr), _uart(NULL) {}

void HardwareSerial::begin(unsigned long baud, uint32_t config, int8_t rxPin, int8_t txPin, bool invert, unsigned long timeout_ms)
{
    if(0 > _uart_nr || _uart_nr > 2) {
        log_e("Serial number is invalid, please use 0, 1 or 2");
        return;
    }
    if(_uart) {
        end();
    }
    if(_uart_nr == 0 && rxPin < 0 && txPin < 0) {
        rxPin = 6;
        txPin = 5;
    }
    if(_uart_nr == 1 && rxPin < 0 && txPin < 0) {
        rxPin = UART_RX_0;//0;
        txPin = UART_TX_0;//1;
    }
    if(_uart_nr == 2 && rxPin < 0 && txPin < 0) {
        rxPin = UART_RX_2;//2;
        txPin = UART_TX_2;//3;
    }

    if (baud == AUTOBAUD){
      if (_uart_nr == 0){ // our companion-connected serial port
	    #ifdef COMMUNICATION_LIB_ENABLED
			// Ask our companion for current baud setting on hardware serial port, and set baudrate to same
			baud = sendSystemGet(SYSTEMCMD_GET_BAUD);
		#else
			// default
			baud = 115200;
		#endif // COMMUNICATION_LIB_ENABLED
        if (baud < 0){
          log_e("AUTOBAUD failed to get baudrate from companion. Is the companion listening? Will attempt baudrate detection.");
          baud = 0; // fall through to baudrate detection below
        }
      }
    }


	// acquire lock
	if(UARTmutex != NULL)
		xSemaphoreTakeRecursive(UARTmutex, portMAX_DELAY);


    _uart = uartBegin(_uart_nr, baud ? baud : 9600, config, rxPin, txPin, 256, invert);

    if(!baud) {
        uartStartDetectBaudrate(_uart);
        time_t startMillis = millis();
        unsigned long detectedBaudRate = 0;
        while(millis() - startMillis < timeout_ms && !(detectedBaudRate = uartDetectBaudrate(_uart))) {
            yield();
        }

        end();

        if(detectedBaudRate) {
            delay(100); // Give some time...
            _uart = uartBegin(_uart_nr, detectedBaudRate, config, rxPin, txPin, 256, invert);
        } else {
            log_e("Could not detect baudrate. Serial data at the port must be present within the timeout for detection to be possible");
            _uart = NULL;
        }
    }

	// release lock
	if(UARTmutex != NULL)
	  xSemaphoreGiveRecursive(UARTmutex);
}

void HardwareSerial::updateBaudRate(unsigned long baud)
{
	// acquire lock
	if(UARTmutex != NULL)
		xSemaphoreTakeRecursive(UARTmutex, portMAX_DELAY);

	uartSetBaudRate(_uart, baud);

	// release lock
	if(UARTmutex != NULL)
	  xSemaphoreGiveRecursive(UARTmutex);
}

void HardwareSerial::end()
{
	// acquire lock
	if(UARTmutex != NULL)
		xSemaphoreTakeRecursive(UARTmutex, portMAX_DELAY);

    if(uartGetDebug() == _uart_nr) {
        uartSetDebug(0);
    }
    uartEnd(_uart);
    _uart = 0;

	// release lock
	if(UARTmutex != NULL)
	  xSemaphoreGiveRecursive(UARTmutex);
}

size_t HardwareSerial::setRxBufferSize(size_t new_size) {
	// acquire lock
	if(UARTmutex != NULL)
		xSemaphoreTakeRecursive(UARTmutex, portMAX_DELAY);

	size_t bsize = uartResizeRxBuffer(_uart, new_size);

	// release lock
	if(UARTmutex != NULL)
	  xSemaphoreGiveRecursive(UARTmutex);

	return bsize;
}

void HardwareSerial::setDebugOutput(bool en)
{
    if(_uart == 0) {
        return;
    }

	// acquire lock
	if(UARTmutex != NULL)
		xSemaphoreTakeRecursive(UARTmutex, portMAX_DELAY);

    if(en) {
        uartSetDebug(_uart);
    } else {
        if(uartGetDebug() == _uart_nr) {
            uartSetDebug(0);
        }
    }
	// release lock
	if(UARTmutex != NULL)
	  xSemaphoreGiveRecursive(UARTmutex);
}

int HardwareSerial::available(void)
{
	// acquire lock
	if(UARTmutex != NULL)
		xSemaphoreTakeRecursive(UARTmutex, portMAX_DELAY);

	int avail = uartAvailable(_uart);

	// release lock
	if(UARTmutex != NULL)
	  xSemaphoreGiveRecursive(UARTmutex);

    return avail;
}
int HardwareSerial::availableForWrite(void)
{
	// acquire lock
	if(UARTmutex != NULL)
		xSemaphoreTakeRecursive(UARTmutex, portMAX_DELAY);

	int avail = uartAvailableForWrite(_uart);

	// release lock
	if(UARTmutex != NULL)
	  xSemaphoreGiveRecursive(UARTmutex);

    return avail;
}

int HardwareSerial::peek(void)
{
    if (available()) {

		// acquire lock
		if(UARTmutex != NULL)
			xSemaphoreTakeRecursive(UARTmutex, portMAX_DELAY);

		int peek = uartPeek(_uart); 
		// release lock
		if(UARTmutex != NULL)
		  xSemaphoreGiveRecursive(UARTmutex);

        return peek;
    }
    return -1;
}

int HardwareSerial::read(void)
{
    if(available()) {
		// acquire lock
		if(UARTmutex != NULL)
			xSemaphoreTakeRecursive(UARTmutex, portMAX_DELAY);

		int read = uartRead(_uart);

		// release lock
		if(UARTmutex != NULL)
		  xSemaphoreGiveRecursive(UARTmutex);
        return read;
    }
    return -1;
}

void HardwareSerial::flush()
{
	// acquire lock
	if(UARTmutex != NULL)
		xSemaphoreTakeRecursive(UARTmutex, portMAX_DELAY);

    uartFlush(_uart);

	// release lock
	if(UARTmutex != NULL)
	  xSemaphoreGiveRecursive(UARTmutex);
}

size_t HardwareSerial::write(uint8_t c)
{
	// acquire lock
	if(UARTmutex != NULL)
		xSemaphoreTakeRecursive(UARTmutex, portMAX_DELAY);

    uartWrite(_uart, c);

	// release lock
	if(UARTmutex != NULL)
	  xSemaphoreGiveRecursive(UARTmutex);

    return 1;
}

size_t HardwareSerial::write(const uint8_t *buffer, size_t size)
{
	// acquire lock
	if(UARTmutex != NULL)
		xSemaphoreTakeRecursive(UARTmutex, portMAX_DELAY);

    uartWriteBuf(_uart, buffer, size);

	// release lock
	if(UARTmutex != NULL)
	  xSemaphoreGiveRecursive(UARTmutex);

	return size;
}
uint32_t  HardwareSerial::baudRate()
{
	// acquire lock
	if(UARTmutex != NULL)
		xSemaphoreTakeRecursive(UARTmutex, portMAX_DELAY);

	uint32_t baud = uartGetBaudRate(_uart);

	// release lock
	if(UARTmutex != NULL)
	  xSemaphoreGiveRecursive(UARTmutex);

  return baud;

}

HardwareSerial::operator bool() const
{
	#ifdef COMMUNICATION_LIB_ENABLED
		return isSerialOpen();
	#else
		return false;
	#endif
}