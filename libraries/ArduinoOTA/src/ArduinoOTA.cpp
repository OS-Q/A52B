#ifndef LWIP_OPEN_SRC
#define LWIP_OPEN_SRC
#endif
#include <functional>
#include <WiFiUdp.h>
#include "ArduinoOTA.h"
#include "ESPmDNS.h"
#include "MD5Builder.h"
#include "Update.h"
#include "WiFiClientSecure.h"
#include "esp_task_wdt.h"

extern SemaphoreHandle_t UARTmutex;
extern TaskHandle_t loopTaskHandle;

// #define OTA_DEBUG Serial

ArduinoOTAClass::ArduinoOTAClass()
: _port(0)
, _initialized(false)
, _rebootOnSuccess(true)
, _mdnsEnabled(true)
, _state(OTA_IDLE)
, _size(0)
, _cmd(0)
, _ota_port(0)
, _ota_timeout(1000)
, _start_callback(NULL)
, _end_callback(NULL)
, _error_callback(NULL)
, _progress_callback(NULL)
, _url("")
, _remoteFile(false)
, _remoteCli(NULL)
, _loopSuspended(false)
{
}

ArduinoOTAClass::~ArduinoOTAClass(){
    _udp_ota.stop();
}

ArduinoOTAClass& ArduinoOTAClass::onStart(THandlerFunction fn) {
    _start_callback = fn;
    return *this;
}

ArduinoOTAClass& ArduinoOTAClass::onEnd(THandlerFunction fn) {
    _end_callback = fn;
    return *this;
}

ArduinoOTAClass& ArduinoOTAClass::onProgress(THandlerFunction_Progress fn) {
    _progress_callback = fn;
    return *this;
}

ArduinoOTAClass& ArduinoOTAClass::onError(THandlerFunction_Error fn) {
    _error_callback = fn;
    return *this;
}

ArduinoOTAClass& ArduinoOTAClass::setPort(uint16_t port) {
    if (!_initialized && !_port && port) {
        _port = port;
    }
    return *this;
}

ArduinoOTAClass& ArduinoOTAClass::setHostname(const char * hostname) {
    if (!_initialized && !_hostname.length() && hostname) {
        _hostname = hostname;
    }
    return *this;
}

String ArduinoOTAClass::getHostname() {
    return _hostname;
}

ArduinoOTAClass& ArduinoOTAClass::setPassword(const char * password) {
    if (!_initialized && !_password.length() && password) {
        MD5Builder passmd5;
        passmd5.begin();
        passmd5.add(password);
        passmd5.calculate();
        _password = passmd5.toString();
    }
    return *this;
}

ArduinoOTAClass& ArduinoOTAClass::setPasswordHash(const char * password) {
    if (!_initialized && !_password.length() && password) {
        _password = password;
    }
    return *this;
}

ArduinoOTAClass& ArduinoOTAClass::setRebootOnSuccess(bool reboot){
    _rebootOnSuccess = reboot;
    return *this;
}

ArduinoOTAClass& ArduinoOTAClass::setMdnsEnabled(bool enabled){
    _mdnsEnabled = enabled;
    return *this;
}

void ArduinoOTAClass::begin() {
    if (_initialized){
        log_w("already initialized");
        return;
    }

    if (!_port) {
        _port = 3232;
    }

    if(!_udp_ota.begin(_port)){
        log_e("udp bind failed");
        return;
    }


    if (!_hostname.length()) {
        char tmp[20];
        uint8_t mac[6];
        WiFi.macAddress(mac);
        sprintf(tmp, "esp32-%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        _hostname = tmp;
    }
    if(_mdnsEnabled){
        MDNS.begin(_hostname.c_str());
        MDNS.enableArduino(_port, (_password.length() > 0));
    }
    _initialized = true;
    _state = OTA_IDLE;
    log_i("OTA server at: %s.local:%u", _hostname.c_str(), _port);
}

int ArduinoOTAClass::parseInt(){
    char data[INT_BUFFER_SIZE];
    uint8_t index = 0;
    char value;
    while(_udp_ota.peek() == ' ') _udp_ota.read();
    while(index < INT_BUFFER_SIZE - 1){
        value = _udp_ota.peek();
        if(value < '0' || value > '9'){
            data[index++] = '\0';
            return atoi(data);
        }
        data[index++] = _udp_ota.read();
    }
    return 0;
}

String ArduinoOTAClass::readStringUntil(char end){
    String res = "";
    int value;
    while(true){
        value = _udp_ota.read();
        if(value <= 0 || value == end){
            return res;
        }
        res += (char)value;
    }
    return res;
}

void ArduinoOTAClass::_onRx(){
    if (_state == OTA_IDLE) {
        int cmd = parseInt();
        if (cmd != U_FLASH && cmd != U_SPIFFS && cmd != U_COMPANION)
            return;
        _cmd  = cmd;
        _ota_port = parseInt();
		// check if next parameter is a number (size) or not (http link)
		while(_udp_ota.peek() == ' ') _udp_ota.read();
		char value = _udp_ota.peek();
		if(value == 'D'){
			// discard read value
			_udp_ota.read();
			// suspend Arduino task
			vTaskSuspend(loopTaskHandle);
			_loopSuspended = true;
			// keep reading until next value
			while(_udp_ota.peek() == ' ') _udp_ota.read();
			value = _udp_ota.peek();
		}
		if(value >= '0' && value <='9'){
			// it's a number. read size and md5
			_size = parseInt();
			_udp_ota.read();
			_md5 = readStringUntil('\n');
			_md5.trim();
			if(_md5.length() != 32){
				log_e("bad md5 length");
				return;
			}
		}
		else{
			// it's a string. get the url pointing to the firmware to be update
			_url = readStringUntil('\n');
			String host;
			uint16_t port = 80;
			_remoteFile = true;
			// check for : (http: or https:
			int16_t index = _url.indexOf(':');
			if(index > 0){
				String protocol = _url.substring(0, index);
				if(protocol.equals("https")){
					port = 443;
					_remoteCli = new WiFiClientSecure();
				}
				else
					_remoteCli = new WiFiClient();
				_url.remove(0, (index + 3)); // remove http:// or https://
			}
			else // use http as default and cross your fingers
				_remoteCli = new WiFiClient();
				
			index = _url.indexOf('/');
			host = _url.substring(0, index);
			_url.remove(0, index); // remove host part
			// get port
			index = host.indexOf(':');
			if(index >= 0) {
				String h = host.substring(0, index); // hostname
				host.remove(0, (index + 1)); // remove hostname + :
				port = host.toInt(); // get port
				host = h;
			}
			
			// try to connect
			if(!_remoteCli->connect(host.c_str(), port)){
				_udpMessageReply("Failed connection to given URL");
				// resume Arduino task
				if(_loopSuspended){
					_loopSuspended = false;
					vTaskResume(loopTaskHandle);
				}
				if (_error_callback) _error_callback(OTA_CONNECT_ERROR);
				_state = OTA_IDLE;
				delete _remoteCli;
				return;
			}
			
			// send request to remote server
			String header = "GET " + _url + F(" HTTP/1.1");
			header += String(F("\r\nHost: ")) + host;
			if (port != 80 && port != 443){
				header += ':';
				header += String(port);
			}
			header += String(F("\r\nConnection: keep-alive"));
			header += "\r\n\r\n";
			if(_remoteCli->write((const uint8_t *) header.c_str(), header.length()) != header.length()){
				_udpMessageReply("Failed connection to given URL");
				// resume Arduino task
				if(_loopSuspended){
					_loopSuspended = false;
					vTaskResume(loopTaskHandle);
				}
				if (_error_callback) _error_callback(OTA_CONNECT_ERROR);
				_state = OTA_IDLE;
				_remoteCli->stop();
				delete _remoteCli;
				return;
			}
			
			// remove header from response
			uint32_t timeout = millis();
			while(1){
				size_t len = _remoteCli->available();
				if(len > 0) {
					timeout = millis();
					String headerLine = _remoteCli->readStringUntil('\n');
					headerLine.trim(); // remove \r
					
					if(headerLine.startsWith("HTTP/1.")) {
						uint16_t returnCode = headerLine.substring(9, headerLine.indexOf(' ', 9)).toInt();
						if(returnCode != 200){ // error
							_udpMessageReply("File request failed");
							// resume Arduino task
							if(_loopSuspended){
								_loopSuspended = false;
								vTaskResume(loopTaskHandle);
							}
							if (_error_callback) _error_callback(OTA_CONNECT_ERROR);
							_state = OTA_IDLE;
							_remoteCli->stop();
							delete _remoteCli;
							return;
						}
					}
					else if(headerLine.indexOf(':') > 0) {
						String headerName = headerLine.substring(0, headerLine.indexOf(':'));
						String headerValue = headerLine.substring(headerLine.indexOf(':') + 1);
						headerValue.trim();
						if(headerName.equalsIgnoreCase("Content-Length")){
							_size = headerValue.toInt();
						}
					}
				
					if(headerLine.length() == 0) // end of header
						break;
				}
				if((millis() - timeout) > 5000){
					_udpMessageReply("File request failed");
					// resume Arduino task
					if(_loopSuspended){
						_loopSuspended = false;
						vTaskResume(loopTaskHandle);
					}
					if (_error_callback) _error_callback(OTA_CONNECT_ERROR);
					_state = OTA_IDLE;
					_remoteCli->stop();
					delete _remoteCli;
					return;
				}
			}		
		}
        if (_password.length()){
            MD5Builder nonce_md5;
            nonce_md5.begin();
            nonce_md5.add(String(micros()));
            nonce_md5.calculate();
            _nonce = nonce_md5.toString();

            _udp_ota.beginPacket(_udp_ota.remoteIP(), _udp_ota.remotePort());
            _udp_ota.printf("AUTH %s", _nonce.c_str());
            _udp_ota.endPacket();
            _state = OTA_WAITAUTH;
            return;
        } else {
			_udpMessageReply("OK " + String(_size));
            _ota_ip = _udp_ota.remoteIP();
            _state = OTA_RUNUPDATE;
        }
    } else if (_state == OTA_WAITAUTH) {
        int cmd = parseInt();
        if (cmd != U_AUTH) {
            log_e("%d was expected. got %d instead", U_AUTH, cmd);
			// resume Arduino task
			if(_loopSuspended){
				_loopSuspended = false;
				vTaskResume(loopTaskHandle);
			}
            _state = OTA_IDLE;
            return;
        }
        _udp_ota.read();
        String cnonce = readStringUntil(' ');
        String response = readStringUntil('\n');
        if (cnonce.length() != 32 || response.length() != 32) {
            log_e("auth param fail");
			// resume Arduino task
			if(_loopSuspended){
				_loopSuspended = false;
				vTaskResume(loopTaskHandle);
			}
            _state = OTA_IDLE;
            return;
        }

        String challenge = _password + ":" + String(_nonce) + ":" + cnonce;
        MD5Builder _challengemd5;
        _challengemd5.begin();
        _challengemd5.add(challenge);
        _challengemd5.calculate();
        String result = _challengemd5.toString();

        if(result.equals(response)){
			_udpMessageReply("OK"  + String(_size));
            _ota_ip = _udp_ota.remoteIP();
            _state = OTA_RUNUPDATE;
        } else {
            log_w("Authentication Failed");
			_udpMessageReply("Authentication Failed");
			// resume Arduino task
			if(_loopSuspended){
				_loopSuspended = false;
				vTaskResume(loopTaskHandle);
			}
            if (_error_callback) _error_callback(OTA_AUTH_ERROR);
            _state = OTA_IDLE;
        }
    }
}

void ArduinoOTAClass::_runUpdate() {
    if (!Update.begin(_size, _cmd)) {

        log_e("Begin ERROR: %s", Update.errorString());
		// resume Arduino task
		if(_loopSuspended){
			_loopSuspended = false;
			vTaskResume(loopTaskHandle);
		}

        if (_error_callback) {
            _error_callback(OTA_BEGIN_ERROR);
        }
        _state = OTA_IDLE;
        return;
    }
    Update.setMD5(_md5.c_str());

    if (_start_callback) {
        _start_callback();
    }
    if (_progress_callback) {
        _progress_callback(0, _size);
    }

	disableGPIOInterrupt();

    WiFiClient client;
    if (!client.connect(_ota_ip, _ota_port)) {
		// resume Arduino task
		if(_loopSuspended){
			_loopSuspended = false;
			vTaskResume(loopTaskHandle);
		}
        if (_error_callback) {
            _error_callback(OTA_CONNECT_ERROR);
        }
        _state = OTA_IDLE;
    }

    uint32_t written = 0, total = 0, tried = 0;

    while (!Update.isFinished()) {
        size_t waited = _ota_timeout;
        size_t available;
		if(_remoteFile)
			available = _remoteCli->available();
		else
			available = client.available();
        while (!available && waited){
            delay(1);
            waited -=1 ;
			if(_remoteFile)
				available = _remoteCli->available();
			else
				available = client.available();
        }
        if (!waited){
            if(written && tried++ < 3){
                log_i("Try[%u]: %u", tried, written);
                if(!client.printf("%u", written)){
                    log_e("failed to respond");
					// resume Arduino task
					if(_loopSuspended){
						_loopSuspended = false;
						vTaskResume(loopTaskHandle);
					}
                    _state = OTA_IDLE;
                    break;
                }
                continue;
            }
            log_e("Receive Failed");
			// resume Arduino task
			if(_loopSuspended){
				_loopSuspended = false;
				vTaskResume(loopTaskHandle);
			}
            if (_error_callback) {
                _error_callback(OTA_RECEIVE_ERROR);
            }
            _state = OTA_IDLE;
            Update.abort();
            return;
        }
        if(!available){
            log_e("No Data: %u", waited);
			// resume Arduino task
			if(_loopSuspended){
				_loopSuspended = false;
				vTaskResume(loopTaskHandle);
			}
            _state = OTA_IDLE;
            break;
        }
        tried = 0;
        static uint8_t buf[1460];
        if(available > 1460){
            available = 1460;
        }

		size_t r;
		if(_remoteFile)
			r = _remoteCli->read(buf, available);
		else
			r = client.read(buf, available);
        if(r != available){
            log_w("didn't read enough! %u != %u", r, available);
        }

        written = Update.write(buf, r);
        if (written > 0) {
            if(written != r){
                log_w("didn't write enough! %u != %u", written, r);
            }
            if(!client.printf("%u", written)){
                log_w("failed to respond");
            }
            total += written;
            if(_progress_callback) {
                _progress_callback(total, _size);
            }
			vTaskDelay(1); // needed if ota runs on core 0. It gives time core 0 to run write flash tasks
        } else {
            log_e("Write ERROR: %s", Update.errorString());
        }
    }

	if(_remoteFile){
		_remoteCli->stop();
		delete _remoteCli;
		_remoteFile = false;
	}
	
	if(_cmd == U_COMPANION){
		// acquire lock
		if(UARTmutex != NULL)
			xSemaphoreTakeRecursive(UARTmutex, portMAX_DELAY); // make sure that no one will use Serial apart this thread starting from now
	}
    if (Update.end()) {
        client.println("OK");
        client.stop();
        delay(10);
        if (_end_callback) {
            _end_callback();
        }
        //let serial/network finish tasks that might be given in _end_callback
        delay(100);
		if(_cmd == U_COMPANION){
			// restart companion micro
			Update.resetCompanion();

			// release lock
			if(UARTmutex != NULL)
			  xSemaphoreGiveRecursive(UARTmutex); // release exclusive use of serial
		  	
		}
        if(_rebootOnSuccess){
		    //ESP.restart();
			// it seems that restart function has some issue in a multithread environment.
			// force a reset with watchdog
			esp_task_wdt_init(1,true);
			esp_task_wdt_add(NULL);
			delay(800);
			digitalWrite(COMPANION_RESET, LOW);
			delay(50);
			digitalWrite(COMPANION_RESET, HIGH);
			while(true);
        }
    } else {
		if(_cmd == U_COMPANION){
			// if something went wrong during end phase generate a pulse on reset signal of companion micro
			// in this way we don't leave companion micro in an unknown state.
			/*digitalWrite(COMPANION_RESET, LOW);
			delay(50);
			digitalWrite(COMPANION_RESET, HIGH);
			*/ // [cr] check
			// release lock
			if(UARTmutex != NULL)
			  xSemaphoreGiveRecursive(UARTmutex); // release exclusive use of serial
						
		    //ESP.restart();
			// it seems that restart function has some issue in a multithread environment.
			// force a reset with watchdog
			esp_task_wdt_init(1,true);
			esp_task_wdt_add(NULL);
			delay(800);
			digitalWrite(COMPANION_RESET, LOW);
			delay(50);
			digitalWrite(COMPANION_RESET, HIGH);
			while(true);
		}
		// resume Arduino task
		if(_loopSuspended){
			_loopSuspended = false;
			vTaskResume(loopTaskHandle);
		}
        if (_error_callback) {
            _error_callback(OTA_END_ERROR);
        }
        Update.printError(client);
        client.stop();
        delay(10);
        log_e("Update ERROR: %s", Update.errorString());
        _state = OTA_IDLE;
    }
	enableGPIOInterrupt();
}

void ArduinoOTAClass::end() {
    _initialized = false;
    _udp_ota.stop();
    if(_mdnsEnabled){
        MDNS.end();
    }
    _state = OTA_IDLE;
    log_i("OTA server stopped.");
}

void ArduinoOTAClass::handle() {
    if (!_initialized) {
        return; 
    }
    if (_state == OTA_RUNUPDATE) {
        _runUpdate();
        _state = OTA_IDLE;
    }
    if(_udp_ota.parsePacket()){
        _onRx();
    }
    _udp_ota.flush(); // always flush, even zero length packets must be flushed.
}

int ArduinoOTAClass::getCommand() {
    return _cmd;
}

void ArduinoOTAClass::setTimeout(int timeoutInMillis) {
    _ota_timeout = timeoutInMillis;
}

void ArduinoOTAClass::_udpMessageReply(String msg){
	_udp_ota.beginPacket(_udp_ota.remoteIP(), _udp_ota.remotePort());
	_udp_ota.println(msg);
	_udp_ota.endPacket();
}

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_ARDUINOOTA)
ArduinoOTAClass ArduinoOTA;
#endif