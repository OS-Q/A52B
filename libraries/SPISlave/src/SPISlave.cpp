#include<SPISlave.h>

spi_slave_transaction_t m;

void transIntr(spi_slave_transaction_t * trans){
	if(SPISlave._handshake != -1){ // lower handshake pin to signal that our buffer is empty - we are not ready to receive a new transaction
		WRITE_PERI_REG(GPIO_OUT_W1TC_REG, (1<<SPISlave._handshake));
	}

	uint8_t transferredBytes = (trans->trans_len / 8);

	bool received = false;
	
	// copy the received bytes in rx_buffer array
	if(SPISlave._output_buffer != NULL){
		if(((uint8_t*)trans->rx_buffer)[0] != 0){
			// overwrite buffer only if we have received valid data
			memcpy(SPISlave._output_buffer, trans->rx_buffer, transferredBytes);
			*(SPISlave._output_len) = transferredBytes;
			received = true;
			// invalidate data
			((uint8_t*)trans->rx_buffer)[0] = 0;
		}
	}

	if(SPISlave.callback != NULL)
		SPISlave.callback(transferredBytes, received);
}


void setupIntr(spi_slave_transaction_t * trans){
	if(SPISlave._handshake != -1){ // raise handshake pin to signal that our buffer is full - we are ready to receive a new transaction
		WRITE_PERI_REG(GPIO_OUT_W1TS_REG, (1<<SPISlave._handshake));
	}

	if(SPISlave.dataSet != NULL)
		SPISlave.dataSet();
}

SPISlaveClass::SPISlaveClass(){}


void SPISlaveClass::begin(uint8_t mosi, uint8_t miso, uint8_t sclk, uint8_t cs, int8_t handshake){
	// check on SPI pins
	#ifdef BRIKI_MBC_WB_ESP
		if((sclk > PINS_COUNT || g_APinDescription[sclk].ulPort == PIO_NOT_A_PORT) || (miso > PINS_COUNT || g_APinDescription[miso].ulPort == PIO_NOT_A_PORT) || (mosi > PINS_COUNT || g_APinDescription[mosi].ulPort == PIO_NOT_A_PORT) || (cs > PINS_COUNT || g_APinDescription[cs].ulPort == PIO_NOT_A_PORT)) {
			return;
		}
		sclk = g_APinDescription[sclk].ulPin;
		miso = g_APinDescription[miso].ulPin;
		mosi = g_APinDescription[mosi].ulPin;
		cs = g_APinDescription[cs].ulPin;
		if(handshake != -1)
			handshake = g_APinDescription[handshake].ulPin;
	#endif
	_handshake = handshake;

	//Configuration for the SPI bus
    spi_bus_config_t buscfg={
        mosi_io_num : (gpio_num_t)mosi,
        miso_io_num : (gpio_num_t)miso,
        sclk_io_num : (gpio_num_t)sclk
    };

    //Configuration for the SPI slave interface
    spi_slave_interface_config_t slvcfg={
        spics_io_num : (gpio_num_t)cs,
        flags : 0,
        queue_size : 1,
        mode : 0,
        post_setup_cb : setupIntr,
        post_trans_cb : transIntr
    };

	//Configuration for the handshake line
    if(_handshake != -1){
        gpio_config_t io_conf;
	    io_conf.pin_bit_mask= (1<< _handshake);
	    io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.intr_type = GPIO_INTR_DISABLE;

        //Configure handshake line as output
        gpio_config(&io_conf);
	}

    //Enable pull-ups on SPI lines so we don't detect rogue pulses when no master is connected.
    gpio_set_pull_mode((gpio_num_t)mosi, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode((gpio_num_t)sclk, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode((gpio_num_t)cs, GPIO_PULLUP_ONLY);

    //Initialize SPI slave interface
    esp_err_t ret = spi_slave_initialize(HSPI_HOST, &buscfg, &slvcfg, 1);
    assert(ret==ESP_OK);
    memset(_rx_buffer, 0, 128);
    memset(_tx_buffer, 0, 128);
	
	// set a transaction in the queue to be able to receive messages from master
	t.length=128*8;
    t.rx_buffer=_rx_buffer;
	t.tx_buffer=_tx_buffer;
	
	spi_slave_queue_trans(HSPI_HOST, &t, 0);
}

void SPISlaveClass::setRxBuffer(uint8_t* buffer, uint8_t* recvLen){
	if(buffer != NULL && recvLen != NULL){
		_output_buffer = buffer;
		_output_len = recvLen;
	}
}

void SPISlaveClass::setOnDataTrasnferred(exter_intr function){
	callback = function;
}

void SPISlaveClass::setOnDataSet(void (*function)(void)){
	dataSet = function;
}

esp_err_t SPISlaveClass::setData(uint8_t * tx_buffer, uint8_t len, uint32_t timeout){
	// 128 is the maximum len for a single SPI transmission
	if(len > 127)
		len = 127;

	t.length=((len)*8);
    t.rx_buffer=_rx_buffer;

	t.tx_buffer=tx_buffer;

	// spi_slave_queue_trans is a blocking function. If timeout is not set it will wait forever until master starts a communication to make room for this message in the queue
	TickType_t waitTime = portMAX_DELAY;
	if(timeout != 0xFFFFFFFF)
		waitTime = timeout / portTICK_PERIOD_MS;
	
    return spi_slave_queue_trans(HSPI_HOST, &t, waitTime);
}

esp_err_t SPISlaveClass::enableRx(){
	// enable reception of next packet by setting another transaction in the queue
	
	m.length=128*8;
    m.rx_buffer=SPISlave._rx_buffer;
	m.tx_buffer=SPISlave._tx_buffer;
	
	// timeout 0 helps if another transaction was already being set outside
	return spi_slave_queue_trans(HSPI_HOST, &m, 0);
}

SPISlaveClass SPISlave;