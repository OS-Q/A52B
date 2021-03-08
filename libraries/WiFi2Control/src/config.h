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

#ifndef CONFIG_H
#define CONFIG_H
/*
 * Firmware version and build date
 */

#define BUILD_DATE "20190123"
#define FW_VERSION "1.0.0"
#define FW_NAME "wifi2control"


/*
 * Enable/Disable Debug
 */
//#define DEBUG
//#define BAUDRATE_DEBUG 115200

/*
 * Define board hostname
 */
#define DEF_HOSTNAME "briki"

/*
 * Defines main parameters
 */

// briki board configuration parameters
#define BOARDMODEL "Briki"
#define SSIDNAME "MBC-WB"

//#define DEBUG_SERIALE


static IPAddress default_IP(192,168,240,1);  //defaul IP Address

#endif //CONFIG_H