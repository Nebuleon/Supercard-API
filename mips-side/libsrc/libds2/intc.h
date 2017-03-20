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

#ifndef __INTC_H__
#define __INTC_H__

#include <stdint.h>

#define NUM_GPIO 192
#define IRQ_MAX (IRQ_GPIO_0 + NUM_GPIO)

// Initialises the interrupt controller.
void _intc_init(void);

// Registers a handler for the given IRQ number, with the given data to be
// supplied to the handler when it needs to be called.
extern int irq_request(unsigned int irq, void (*handler) (unsigned int), unsigned int arg);

// Removes the handler for the given IRQ number.
extern void irq_free(unsigned int irq);

// Sets the master interrupt enable flag.
extern void sti(void);

// Clears the master interrupt enable flag.
extern void cli(void);

// Starts a section of code that has interrupts disabled, but returns the
// contents of Coprocessor 0 Status before the operation. This allows nested
// critical sections to restore to the prior state (with interrupts still
// disabled) properly using DS2_LeaveCriticalSection.
extern uint32_t DS2_EnterCriticalSection(void);

// Ends a section of code that had interrupts disabled, restoring the contents
// of Coprocessor 0 Status to what they were before.
extern void DS2_LeaveCriticalSection(uint32_t val);

#endif //__INTC_H__

