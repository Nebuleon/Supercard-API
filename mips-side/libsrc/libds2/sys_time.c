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

#include "jz4740.h"

void _sys_timer_init(void)
{
	REG_TCU_OSTCSR = 0;
	__tcu_stop_ost_counter();

	/* Using EXTAL/1024 as the clock
	 * 24 MHz / 1024 = 23437.5 Hz (period = 42.66... microseconds) */
	REG_TCU_OSTCSR = TCU_TCSR_PRESCALE1024 | TCU_TCSR_EXT_EN | TCU_TCSR_PWM_SD /* PWM shutdown */
		| (1 << 15) /* keep increasing the counter when Compare reaches Count */;
	REG_TCU_OSTDR = -1; /* Compare (Data) is all-ones */
	REG_TCU_OSTCR = 0; /* Count is 0 */
	__tcu_mask_ost_match_irq();
	__tcu_clear_ost_match_flag();

	__tcu_start_ost_counter();
}
