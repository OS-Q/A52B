#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include "Arduino.h"

#include "communication.h"

#include "ArduinoOTA.h"
#include "WiFiUdp.h"
#include "Update.h"
#include "ESPmDNS.h"
#include "WiFi.h"
#include "WiFiClientSecure.h"

TaskHandle_t loopTaskHandle = NULL;

#if CONFIG_AUTOSTART_ARDUINO

bool loopTaskWDTEnabled;

SemaphoreHandle_t UARTmutex;

// ---------------------------------------------- OTA Task
void OTATask(void *pvParameters)
{
  #ifdef OTA_RUNNING
    ArduinoOTAClass internalOTA;
    internalOTA.begin();
  #endif
  
  while (1) {
    #ifdef OTA_RUNNING
   	  internalOTA.handle();
    #endif
    vTaskDelay(1);
	#ifdef COMMUNICATION_LIB_ENABLED
	  communication.handleCommEvents();
	#endif
  }
}


void loopTask(void *pvParameters)
{
	setup();
    for(;;) {
		if(loopTaskWDTEnabled){
            esp_task_wdt_reset();
		}
        loop();
		#ifdef COMMUNICATION_LIB_ENABLED
			communication.handleCommEvents();
		#endif
    }
}

extern "C" void app_main()
{
	loopTaskWDTEnabled = false;
    initArduino();
	UARTmutex = xSemaphoreCreateRecursiveMutex();
	#ifdef COMMUNICATION_LIB_ENABLED
		communication.communicationBegin();
	#endif
	#if defined(OTA_RUNNING) && defined(MBC_AP_ENABLED)
		IPAddress default_IP(192,168,240,1);
		WiFi.begin();
		WiFi.mode(WIFI_AP_STA);
		WiFi.disconnect();
		delay(100);
		WiFi.softAPConfig(default_IP, default_IP, IPAddress(255, 255, 255, 0));   //set default ip for AP mode
		#ifdef MBC_AP_NAME
			char* softApssid = MBC_AP_NAME;
		#else
			String mac = WiFi.macAddress();
			char softApssid []= {'M', 'B', 'C', '-', 'W', 'B', '-', mac[9], mac[10], mac[12], mac[13], mac[15], mac[16], '\0'};
		#endif
		#ifdef MBC_AP_PASSWORD
			char* passphrase = MBC_AP_PASSWORD;
		#else
			char * passphrase = NULL;
		#endif
		WiFi.softAP(softApssid, passphrase);
	#endif

	xTaskCreateUniversal(loopTask, "loopTask", 8192, NULL, 1, &loopTaskHandle, CONFIG_ARDUINO_RUNNING_CORE);
	#if defined(OTA_RUNNING) || defined (COMMUNICATION_LIB_ENABLED)
	//create a task that will manage OTA update and communication events, with priority 1 and executed on core 0
	xTaskCreateUniversal(OTATask, "OTATask", 10000, NULL, 1, NULL, 0);
	#endif
}
	
#endif