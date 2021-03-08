#include "Update.h"
#include "Arduino.h"
#include "esp_spi_flash.h"
#include "esp_ota_ops.h"
#include "esp_image_format.h"
#include "driver/periph_ctrl.h"
#include "driver/uart.h"

static const uint16_t crc16Table[256] = {
    0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
    0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,
    0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,
    0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,
    0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,
    0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,
    0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,
    0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,
    0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,
    0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
    0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,
    0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,
    0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,
    0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,
    0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,
    0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,
    0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,
    0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,
    0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,
    0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
    0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,
    0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
    0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,
    0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634,
    0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,
    0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,
    0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,
    0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92,
    0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,
    0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
    0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,
    0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0
};

static uint16_t crc16Calc(const uint8_t *data, int len){
    uint16_t crc16 = 0;

    while (len-- > 0)
        crc16 = (crc16 << 8) ^ crc16Table[((crc16 >> 8) ^ *(uint8_t*) data++) & 0xff];
    return crc16;
}

static const char * _err2str(uint8_t _error){
    if(_error == UPDATE_ERROR_OK){
        return ("No Error");
    } else if(_error == UPDATE_ERROR_WRITE){
        return ("Flash Write Failed");
    } else if(_error == UPDATE_ERROR_ERASE){
        return ("Flash Erase Failed");
    } else if(_error == UPDATE_ERROR_READ){
        return ("Flash Read Failed");
    } else if(_error == UPDATE_ERROR_SPACE){
        return ("Not Enough Space");
    } else if(_error == UPDATE_ERROR_SIZE){
        return ("Bad Size Given");
    } else if(_error == UPDATE_ERROR_STREAM){
        return ("Stream Read Timeout");
    } else if(_error == UPDATE_ERROR_MD5){
        return ("MD5 Check Failed");
    } else if(_error == UPDATE_ERROR_MAGIC_BYTE){
        return ("Wrong Magic Byte");
    } else if(_error == UPDATE_ERROR_ACTIVATE){
        return ("Could Not Activate The Firmware");
    } else if(_error == UPDATE_ERROR_NO_PARTITION){
        return ("Partition Could Not be Found");
    } else if(_error == UPDATE_ERROR_BAD_ARGUMENT){
        return ("Bad Argument");
    } else if(_error == UPDATE_ERROR_ABORT){
        return ("Aborted");
    }
    return ("UNKNOWN");
}

static bool _partitionIsBootable(const esp_partition_t* partition){
    uint8_t buf[4];
    if(!partition){
        return false;
    }
    if(!ESP.flashRead(partition->address, (uint32_t*)buf, 4)) {
        return false;
    }

    if(buf[0] != ESP_IMAGE_HEADER_MAGIC) {
        return false;
    }
    return true;
}

static bool _enablePartition(const esp_partition_t* partition){
    uint8_t buf[4];
    if(!partition){
        return false;
    }
    if(!ESP.flashRead(partition->address, (uint32_t*)buf, 4)) {
        return false;
    }
    buf[0] = ESP_IMAGE_HEADER_MAGIC;

    return ESP.flashWrite(partition->address, (uint32_t*)buf, 4);
}

UpdateClass::UpdateClass()
: _error(0)
, _buffer(0)
, _bufferLen(0)
, _size(0)
, _progress_callback(NULL)
, _progress(0)
, _command(U_FLASH)
, _partition(NULL)
{
}

UpdateClass& UpdateClass::onProgress(THandlerFunction_Progress fn) {
    _progress_callback = fn;
    return *this;
}

void UpdateClass::_reset() {
    if (_buffer)
        delete[] _buffer;
    _buffer = 0;
    _bufferLen = 0;
    _progress = 0;
    _size = 0;
    _command = U_FLASH;

    if(_ledPin != -1) {
      digitalWrite(_ledPin, !_ledOn); // off
    }
}

bool UpdateClass::canRollBack(){
    if(_buffer){ //Update is running
        return false;
    }
    const esp_partition_t* partition = esp_ota_get_next_update_partition(NULL);
    return _partitionIsBootable(partition);
}

bool UpdateClass::rollBack(){
    if(_buffer){ //Update is running
        return false;
    }
    const esp_partition_t* partition = esp_ota_get_next_update_partition(NULL);
    return _partitionIsBootable(partition) && !esp_ota_set_boot_partition(partition);
}

bool UpdateClass::begin(size_t size, int command, int ledPin, uint8_t ledOn) {
    if(_size > 0){
        log_w("already running");
        return false;
    }

    _ledPin = ledPin;
    _ledOn = !!ledOn; // 0(LOW) or 1(HIGH)

    _reset();
    _error = 0;

    if(size == 0) {
        _error = UPDATE_ERROR_SIZE;
        return false;
    }

    if (command == U_FLASH || command == U_COMPANION) {
        _partition = esp_ota_get_next_update_partition(NULL);
        if(!_partition){
            _error = UPDATE_ERROR_NO_PARTITION;
            return false;
        }
        log_d("OTA Partition: %s", _partition->label);
    }
    else if (command == U_SPIFFS) {
        _partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, NULL);
        if(!_partition){
			_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, NULL);
			if(!_partition){
				_error = UPDATE_ERROR_NO_PARTITION;
				return false;
			}
        }
    }
    else {
        _error = UPDATE_ERROR_BAD_ARGUMENT;
        log_e("bad command %u", command);
        return false;
    }

    if(size == UPDATE_SIZE_UNKNOWN){
        size = _partition->size;
    } else if(size > _partition->size){
        _error = UPDATE_ERROR_SIZE;
        log_e("too large %u > %u", size, _partition->size);
        return false;
    }

    //initialize
    _buffer = (uint8_t*)malloc(SPI_FLASH_SEC_SIZE);
    if(!_buffer){
        log_e("malloc failed");
        return false;
    }
    _size = size;
    _command = command;
    _md5.begin();
    return true;
}

void UpdateClass::_abort(uint8_t err){
    _reset();
    _error = err;
}

void UpdateClass::abort(){
    _abort(UPDATE_ERROR_ABORT);
}

bool UpdateClass::_writeBuffer(){
    //first bytes of new firmware
    if(!_progress && _command == U_FLASH){
        //check magic
        if(_buffer[0] != ESP_IMAGE_HEADER_MAGIC){
            _abort(UPDATE_ERROR_MAGIC_BYTE);
            return false;
        }
        //remove magic byte from the firmware now and write it upon success
        //this ensures that partially written firmware will not be bootable
        _buffer[0] = 0xFF;
    }
    if (!_progress && _progress_callback) {
        _progress_callback(0, _size);
    }
    if(!ESP.flashEraseSector((_partition->address + _progress)/SPI_FLASH_SEC_SIZE)){
        _abort(UPDATE_ERROR_ERASE);
        return false;
    }
    if (!ESP.flashWrite(_partition->address + _progress, (uint32_t*)_buffer, _bufferLen)) {
        _abort(UPDATE_ERROR_WRITE);
        return false;
    }
    //restore magic or md5 will fail
    if(!_progress && _command == U_FLASH){
        _buffer[0] = ESP_IMAGE_HEADER_MAGIC;
    }
    _md5.add(_buffer, _bufferLen);
    _progress += _bufferLen;
    _bufferLen = 0;
    if (_progress_callback) {
        _progress_callback(_progress, _size);
    }
    return true;
}

bool UpdateClass::_verifyHeader(uint8_t data) {
    if(_command == U_FLASH) {
        if(data != ESP_IMAGE_HEADER_MAGIC) {
            _abort(UPDATE_ERROR_MAGIC_BYTE);
            return false;
        }
        return true;
    } else if(_command == U_SPIFFS || _command == U_COMPANION) {
        return true;
    }
    return false;
}

bool UpdateClass::_verifyEnd() {
    if(_command == U_FLASH) {
        if(!_enablePartition(_partition) || !_partitionIsBootable(_partition)) {
            _abort(UPDATE_ERROR_READ);
            return false;
        }

        if(esp_ota_set_boot_partition(_partition)){
            _abort(UPDATE_ERROR_ACTIVATE);
            return false;
        }
        _reset();
        return true;
    } else if(_command == U_SPIFFS) {
        _reset();
        return true;
    } else if(_command == U_COMPANION) {
		return _flashCompanion();
	}

    return false;
}

bool UpdateClass::setMD5(const char * expected_md5){
    if(strlen(expected_md5) != 32)
    {
        return false;
    }
    _target_md5 = expected_md5;
    return true;
}

bool UpdateClass::end(bool evenIfRemaining){
    if(hasError() || _size == 0){
        return false;
    }

    if(!isFinished() && !evenIfRemaining){
        log_e("premature end: res:%u, pos:%u/%u\n", getError(), progress(), _size);
        _abort(UPDATE_ERROR_ABORT);
        return false;
    }

    if(evenIfRemaining) {
        if(_bufferLen > 0) {
            _writeBuffer();
        }
        _size = progress();
    }

    _md5.calculate();
    if(_target_md5.length()) {
        if(_target_md5 != _md5.toString()){
            _abort(UPDATE_ERROR_MD5);
            return false;
        }
    }

    return _verifyEnd();
}

size_t UpdateClass::write(uint8_t *data, size_t len) {
    if(hasError() || !isRunning()){
        return 0;
    }

    if(len > remaining()){
        _abort(UPDATE_ERROR_SPACE);
        return 0;
    }

    size_t left = len;

    while((_bufferLen + left) > SPI_FLASH_SEC_SIZE) {
        size_t toBuff = SPI_FLASH_SEC_SIZE - _bufferLen;
        memcpy(_buffer + _bufferLen, data + (len - left), toBuff);
        _bufferLen += toBuff;
        if(!_writeBuffer()){
            return len - left;
        }
        left -= toBuff;
    }
    memcpy(_buffer + _bufferLen, data + (len - left), left);
    _bufferLen += left;
    if(_bufferLen == remaining()){
        if(!_writeBuffer()){
            return len - left;
        }
    }
    return len;
}

size_t UpdateClass::writeStream(Stream &data) {
    size_t written = 0;
    size_t toRead = 0;
    if(hasError() || !isRunning())
        return 0;

    if(!_verifyHeader(data.peek())) {
        _reset();
        return 0;
    }

    if(_ledPin != -1) {
        pinMode(_ledPin, OUTPUT);
    }

    while(remaining()) {
        if(_ledPin != -1) {
            digitalWrite(_ledPin, _ledOn); // Switch LED on
        }
        size_t bytesToRead = SPI_FLASH_SEC_SIZE - _bufferLen;
        if(bytesToRead > remaining()) {
            bytesToRead = remaining();
        }

        toRead = data.readBytes(_buffer + _bufferLen,  bytesToRead);
        if(toRead == 0) { //Timeout
            delay(100);
            toRead = data.readBytes(_buffer + _bufferLen, bytesToRead);
            if(toRead == 0) { //Timeout
                _abort(UPDATE_ERROR_STREAM);
                return written;
            }
        }
        if(_ledPin != -1) {
            digitalWrite(_ledPin, !_ledOn); // Switch LED off
        }
        _bufferLen += toRead;
        if((_bufferLen == remaining() || _bufferLen == SPI_FLASH_SEC_SIZE) && !_writeBuffer())
            return written;
        written += toRead;
    }
    return written;
}

void UpdateClass::printError(Stream &out){
    out.println(_err2str(_error));
}

const char * UpdateClass::errorString(){
    return _err2str(_error);
}

void UpdateClass::resetCompanion(){
	uint8_t cmd[20];
	// send WADDR,VALUE# command
	uint32_t resetAddr = 0xE000ED0C;
	uint32_t resetVal = 0x05FA0004;
	uint8_t len = snprintf((char*) cmd, sizeof(cmd), "W%08X,%08X#", resetAddr, resetVal);
	Serial.write(cmd, len);
}

bool UpdateClass::_flashCompanion(){
	// flash firmware using BOSSAC protocol. Companion chip is supposed to have sam_ba bootloader running
	if(!_initializeBootloader())
		return false;
	
	if(!_eraseCompanion())
		return false;


	// write firmware in 4096-bytes chunks
	uint16_t chunkLen = 4096;
	uint8_t chunk[chunkLen];
	uint32_t offset = 0;
	while(offset < _progress){
		uint16_t toRead = chunkLen;
		if(offset + chunkLen > _progress)
			toRead = chunkLen - (offset + chunkLen - _progress);
			memset(chunk, 0, chunkLen);
		if (!ESP.flashRead(_partition->address + offset, (uint32_t*)chunk, toRead))
			// error while reading. abort
			return false;
		uint16_t toWrite = toRead;
		// Ceil to nearest pagesize (64 bytes)
		if(toRead< 4096)
			toWrite = ((toRead + 64 - 1) / 64) * 64;
		// send data to SRAM
		// send SADDR,LEN# command
		uint8_t cmd[20];
		uint8_t len = snprintf((char*) cmd, sizeof(cmd), "S%08X,%08X#", COMPANION_RAM_ADDR, toWrite);
		Serial.write(cmd, len);
		// wait for start - 'C'
		if(!_checkCompanionReply('C'))
			return false;
		// send the whole chunk
		_writeInBlocks(chunk, toWrite);

		// write data to flash
		// send YSOURCEADDR,0# command
		len = snprintf((char*) cmd, sizeof(cmd), "Y%08X,0#", COMPANION_RAM_ADDR);
		Serial.write(cmd, len);
		// reply should be "Y\r\n"
		if(!_checkCompanionReply('Y')) // error, abort
			return false;

		// send YDESTADDR,LEN# command
		len = snprintf((char*) cmd, sizeof(cmd), "Y%08X,%08X#", COMPANION_FLASH_ADDR + offset, toWrite);
		Serial.write(cmd, len);
		// reply should be "Y\r\n"
		if(!_checkCompanionReply('Y')) // error, abort
			return false;

		offset += toWrite;
	}
	return true;
}
		
bool UpdateClass::_initializeBootloader(){	
	// start bootloader on companion chip by toggling twice reset pin
	pinMode(COMPANION_RESET, OUTPUT);
	digitalWrite(COMPANION_RESET, LOW);
	delay(100);
	digitalWrite(COMPANION_RESET, HIGH);
	delay(100);
	digitalWrite(COMPANION_RESET, LOW);
	delay(100);
	digitalWrite(COMPANION_RESET, HIGH);
	
	// initialize Serial at the expected baud rate
	Serial.begin(921600);
	Serial.flush();
	Serial.end();
	uart_driver_delete(UART_NUM_0);
	periph_module_disable( PERIPH_UART0_MODULE );
	delay( 20 );
	periph_module_enable( PERIPH_UART0_MODULE );
	periph_module_disable( PERIPH_UART1_MODULE );
	delay( 20 );
	periph_module_enable( PERIPH_UART1_MODULE );
	periph_module_disable( PERIPH_UART2_MODULE );
	delay( 20 );
	periph_module_enable( PERIPH_UART2_MODULE );
	Serial.begin(921600);
		
	// send start sequence
	Serial.write('#');
	Serial.read();
	Serial.read();
	Serial.read();
	Serial.write("N#");
	// '\r\n' expected
	_checkCompanionReply('\r');

	// ask for version and check reply
	Serial.write("V#");
	// example reply: v2.0 [Arduino:XYZ] Nov 21 2018 11:53:29
	return _checkCompanionReply('v');
}

bool UpdateClass::_eraseCompanion(){
	// send XADDR# command
	uint8_t cmd[20];
	uint8_t len = snprintf((char*) cmd, sizeof(cmd), "X%08X#", COMPANION_FLASH_ADDR);
	Serial.write(cmd, len);

	// reply should be "X\r\n"
	return _checkCompanionReply('X');
}

bool UpdateClass::_checkCompanionReply(char expected){
	// wait 2 second for a reply
	uint32_t start = millis();
	uint8_t i = 0;
	uint8_t reply[50];
	while(i == 0 && (millis() - start) < 2000){
		delay(1); // yield
		while(Serial.available()){
			reply[i] = Serial.read();
			if(++i == 50) // just for safeness
				break;
		}
	}
	
	// check if character we expect is inside response
	for(uint8_t j = 0; j < i; j++){
		if(reply[j] == expected){
			return true;
		}
	}

	return false;
}

bool UpdateClass::_writeInBlocks(uint8_t* buffer, uint16_t len){
	uint8_t blockSize = 128;
	uint8_t block[blockSize + 5];
	uint32_t blkNum = 1;
	while(len > 0){
		block[0] = 0x01; // SOH
        block[1] = (blkNum & 0xff);
        block[2] = ~(blkNum & 0xff);
        memmove(&block[3], buffer, (len < blockSize) ? len : blockSize);
        if (len < blockSize)
            memset(&block[3] + len, 0, blockSize - len);
        // add crc
		uint16_t crc16;
		crc16 = crc16Calc(&block[3], blockSize);
		block[blockSize + 3] = (crc16 >> 8) & 0xff;
		block[blockSize + 4] = crc16 & 0xff;
		// write block
		Serial.write(block, sizeof(block));
		// wait for ACK
		if(!_checkCompanionReply(0x06)) // error, abort
			return false;
		buffer += blockSize;
        len -= blockSize;
        blkNum++;
	}
	Serial.write(0x04); // EOT
	// wait for ACK
	if(!_checkCompanionReply(0x06)) // error, abort
		return false;
	return true;
}

UpdateClass Update;
