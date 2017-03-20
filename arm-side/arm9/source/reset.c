/*
 * This file is part of the DS communication library for the Supercard DSTwo.
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

#include <nds.h>
#include <stddef.h>

void reset_hardware()
{
	size_t i;

	/* Clear the screen quickly. */
	vramDefault();

	/* Ensure all timers and DMAs are stopped. */
	for (i = 0; i < 4; i++) {
		TIMER_CR(i) = 0;
		DMA_CR(i) = 0;
	}

	REG_IME = IME_DISABLE;
	REG_IE = 0;
	REG_IF = IRQ_ALL;
}
