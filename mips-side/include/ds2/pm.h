/*
 * This file is part of the C standard library for the Supercard DSTwo.
 *
 * Copyright 2017 Nebuleon Fumika <nebuleon.fumika@gmail.com>
 *
 * It is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * It is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with it.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __DS2_PM_H__
#define __DS2_PM_H__

/*
 * Requests the lowest stable clock speed for the Supercard DSTwo's internal
 * CPU. Currently, this is 360 MHz.
 *
 * Returns:
 *   0 on success.
 *   -1 if the request failed.
 */
extern int DS2_LowClockSpeed(void);

/*
 * Requests the nominal clock speed for the Supercard DSTwo's internal CPU.
 * This is 360 MHz.
 *
 * Returns:
 *   0 on success.
 *   -1 if the request failed.
 */
extern int DS2_NominalClockSpeed(void);

/*
 * Requests the highest stable clock speed for the Supercard DSTwo's internal
 * CPU. Currently, this is 360 MHz.
 *
 * Returns:
 *   0 on success.
 *   -1 if the request failed.
 */
extern int DS2_HighClockSpeed(void);

/*
 * Enters an idle state, waiting for the next interrupt.
 *
 * This can be used to wait for a condition that can only be updated by the
 * successful completion of an interrupt, like so:
 *
 *   while (condition involving a volatile read)
 *     DS2_AwaitInterrupt();
 *
 * To prevent a situation where an interrupt occurs between reading the value
 * of the condition and waiting for an interrupt waits for the next one, code
 * that requires low latency should wrap the loop inside DS2_StartAwait() and
 * DS2_StopAwait().
 */
#define DS2_AwaitInterrupt() \
	do { \
		__asm__ __volatile__ ( \
			".set mips32\n\t" \
			"wait\n\t" \
			".set mips0\n\t" \
		); \
	} while (0)

/*
 * Starts a block of code that awaits interrupts, of the form
 *
 *   while (condition involving a volatile read)
 *     DS2_AwaitInterrupt();
 *
 * Such code can be interrupted right after the read of the condition and
 * before awaiting an interrupt, which ends up awaiting the next interrupt
 * instead. With agreement from the interrupt handler, such code can be
 * restarted just before the 'while' statement, if a call to this function
 * is placed before the 'while' statement.
 */
extern void DS2_StartAwait(void) __attribute__((returns_twice));

/* Ends a block of code that awaits interrupts, started by DS2_StartAwait. */
extern void DS2_StopAwait(void);

#endif /* !__DS2_PM_H__ */
