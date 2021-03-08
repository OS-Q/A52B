/*
  Meteca SA.  All right reserved.
  created by Chiara Ruggeri
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

#pragma once

#include <stdint.h>
#include <stdbool.h>

/*
 * retrieve the number of timers still available for use
 *
 * @return the number of available timers.
 */
uint8_t availableTimers();

/*
 * create a timer with a specified interval. Call the function passed as parameter when timeout elapses.
 * This function just allocate timer's resources. Timer must be started with startTimer function.
 *
 * @param n_micros: timer period - in microseconds.
 * @param callback: a callback function called every time the number of microseconds specified by period elapses.
 *
 * @return the timer ID associated or -1 if no timer is available.
 */
int8_t createTimer(uint32_t n_micros, void (*callback)());

/*
 * Start the timer created with createTimer function.
 *
 * @param timerId: the ID of the timer returned by createTimer function.
 */
void startTimer(uint8_t timerId);

/*
 * stop a timer. A timer stopped with this function can be started again with startTimer function.
 *
 * @param timerId: the ID of the timer to be stopped.
 */
void stopTimer(uint8_t timerId);

/*
 * stop a timer and free its resources. Once destroyed timerId will no longer refer to this timer.
 *
 * @param timerId: the ID of the timer to destroy.
 */
void destroyTimer(uint8_t timerId);

/*
 * mark a specific timer as busy, in this way it will not be considered when a new timer is started with startTimer function.
 *
 * @param timerId: the ID of the timer that will be marked as busy.
 */
void allocateTimer(uint8_t timerId);

/*
 * check if a specific timer is already in use or not.
 *
 * @param timerId: the ID of the timer to be checked.
 *
 * @return true if the timer is already in use, false otherwise.
 */
bool isTimerUsed(uint8_t timerId);