#ifndef SPISlave_Class
#define SPISlave_Class

#include "Arduino.h"
#include "driver/spi_slave.h"

typedef void (*exter_intr)(uint8_t, bool);

class SPISlaveClass
{	
  public:
	SPISlaveClass();
	
	void begin(uint8_t mosi, uint8_t miso, uint8_t sclk, uint8_t cs, int8_t handshake = -1);
	void setRxBuffer(uint8_t* buffer, uint8_t* recvLen);
	void setOnDataTrasnferred(exter_intr function);
	void setOnDataSet(void (*function)(void));
	esp_err_t setData(uint8_t * tx_buffer, uint8_t len, uint32_t timeout = 0xFFFFFFFF);
	esp_err_t enableRx();
	
  private:
	uint8_t _rx_buffer[128] = {0};
	uint8_t _tx_buffer[128] = {0};
	uint8_t *_output_buffer = NULL;
	uint8_t *_output_len = NULL;
	int8_t _handshake = -1;
	exter_intr callback;
	void (*dataSet)(void) = NULL;
	friend void transIntr(spi_slave_transaction_t * trans);
	friend void setupIntr(spi_slave_transaction_t * trans);
	spi_slave_transaction_t t;
};

extern SPISlaveClass SPISlave;
#endif