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

#include <string.h>

#include <asm/cachectl.h>
#include <archdefs.h>
#include <mipsregs.h>
#include "jz4740.h"

#include <ds2/pm.h>
#include "clock.h"
#include "dma.h"
#include "intc.h"
#include "gpio.h"
#include "sys_time.h"

#define EXCEPTION_HANDLER_LEN 0x10

typedef void (*pfunc) (void);

extern pfunc __CTOR_LIST__[];
extern pfunc __CTOR_END__[];

extern void _exception_jump(void);

static void run_constructors(void)
{
	pfunc *p;

	for (p = &__CTOR_END__[-1]; p >= __CTOR_LIST__; p--) {
		(*p) ();
	}
}

void _ds2_init(void)
{
	/* Copy the exception handler at the proper locations. Only the first few
	 * instructions are copied. */
	memcpy((void*) A_K0BASE, (void*) _exception_jump, EXCEPTION_HANDLER_LEN);
	dcache_writeback_range((void*) A_K0BASE, EXCEPTION_HANDLER_LEN);
	icache_invalidate_range((void*) A_K0BASE, EXCEPTION_HANDLER_LEN);

	memcpy((void*) (A_K0BASE + 0x180), (void*) _exception_jump, EXCEPTION_HANDLER_LEN);
	dcache_writeback_range((void*) (A_K0BASE + 0x180), EXCEPTION_HANDLER_LEN);
	icache_invalidate_range((void*) (A_K0BASE + 0x180), EXCEPTION_HANDLER_LEN);

	memcpy((void*) (A_K0BASE + 0x200), (void*) _exception_jump, EXCEPTION_HANDLER_LEN);
	dcache_writeback_range((void*) (A_K0BASE + 0x200), EXCEPTION_HANDLER_LEN);
	icache_invalidate_range((void*) (A_K0BASE + 0x200), EXCEPTION_HANDLER_LEN);

	_clock_init();
	/* DS2_*ClockSpeed() or _detect_clock() must be called here to
	 * provide the timing for usleep(), used in later initialisation. */
	_detect_clock();
	_sys_timer_init();
	_dma_init();
	_intc_init();
	_gpio_init();

	/* In 0x10000401:
	 * Bits 31..28 = 0001: Enable access to Coprocessor 0 in user mode
	 * Bit 27 = 0: Disable reduced power mode
	 * Bit 25 = 0: Disable endian reversal in user mode
	 * Bit 22 = 0: Non-bootstrap exception vector locations (8000_xxxx)
	 * Bit 21 = 0: Resetting TLB Multiple Matches bit
	 * Bit 20 = 0: Not in Soft Reset exception
	 * Bit 19 = 0: Not in NMI exception
	 * Bits 15..8 = 00000100: Interrupts: Hardware 0 enabled, others disabled
	 * Bit 4 = 0: Operating in kernel mode
	 * Bit 2 = 0: Not in error level
	 * Bit 1 = 0: Not in exception level
	 * Bit 0 = 1: Interrupts enabled */
	write_c0_status(0x10000401);

	run_constructors();
}
