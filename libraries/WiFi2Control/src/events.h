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


/*
 * define these functions inside sketch to be able to receive related events
 */

#ifndef EVENTS_H
#define EVENTS_H

extern bool WiFi_disconnected()          __attribute__((weak)); /**< ESP32 station disconnected from AP */
extern bool WiFi_connected()             __attribute__((weak)); /**< ESP32 station connected to AP */
extern bool WiFi_ready()                 __attribute__((weak)); /**< ESP32 WiFi ready */
extern bool WiFi_scanDone()              __attribute__((weak)); /**< ESP32 finish scanning AP */
extern bool WiFi_startedSTA()            __attribute__((weak)); /**< ESP32 station start */
extern bool WiFi_stoppedSTA()            __attribute__((weak)); /**< ESP32 station stop */
extern bool WiFi_authModeChanged()       __attribute__((weak)); /**< the auth mode of AP connected by ESP32 station changed */
extern bool WiFi_gotIP()                 __attribute__((weak)); /**< ESP32 station got IP from connected AP */
extern bool WiFi_lostIP()                __attribute__((weak)); /**< ESP32 station lost IP and the IP is reset to 0 */
extern bool WiFi_wpsEnrolleeSuccess()    __attribute__((weak)); /**< ESP32 station wps succeeds in enrollee mode */
extern bool WiFi_wpsEnrolleeFailed()     __attribute__((weak)); /**< ESP32 station wps fails in enrollee mode */
extern bool WiFi_wpsEnrolleeTimeout()    __attribute__((weak)); /**< ESP32 station wps timeout in enrollee mode */
extern bool WiFi_wpsEnrolleePinCode()    __attribute__((weak)); /**< ESP32 station wps pin code in enrollee mode */
extern bool WiFi_startedAP()             __attribute__((weak)); /**< ESP32 soft-AP start */
extern bool WiFi_stoppedAP()             __attribute__((weak)); /**< ESP32 soft-AP stop */
extern bool WiFi_STAconnectedToAP()      __attribute__((weak)); /**< a station connected to ESP32 soft-AP */
extern bool WiFi_STAdisconnectedFromAP() __attribute__((weak)); /**< a station disconnected from ESP32 soft-AP */
extern bool WiFi_APassignedIP2STA()      __attribute__((weak)); /**< ESP32 soft-AP assign an IP to a connected station */
extern bool WiFi_APreceivedProbe()       __attribute__((weak)); /**< Receive probe request packet in soft-AP interface */
extern bool WiFi_gotIPv6()               __attribute__((weak)); /**< ESP32 station or ap or ethernet interface v6IP addr is preferred */

#endif //EVENTS_H