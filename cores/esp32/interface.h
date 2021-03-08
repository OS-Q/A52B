/*
  Meteca SA.  All right reserved.
  created by Dario Trimarchi and Chiara Ruggeri 2017-2018
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

#ifndef INTERFACE_H
#define INTERFACE_H

#include "mbc_config.h"
#ifdef COMMUNICATION_LIB_ENABLED

#if defined(COMM_CH_UART)
    #include "uart/uart_drv.h"
	class Interface : public UartDrv {};
#elif defined(COMM_CH_SPI)
    #include "spi/spi_drv.h"
    class Interface : public SpiDrv {};
#else
	#error "Please define a valid interface"
#endif

extern Interface interface;

#endif // COMMUNICATION_LIB_ENABLED

#endif // INTERFACE_H
