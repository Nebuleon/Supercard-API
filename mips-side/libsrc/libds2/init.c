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

#include <stdint.h>
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
#include "pm.h"
#include "sys_time.h"

typedef void (*pfunc) (void);

extern pfunc __CTOR_LIST__[];
extern pfunc __CTOR_END__[];

extern uint8_t _tlb_refill_vector, _tlb_refill_vector_end;
extern uint8_t _cache_error_vector, _cache_error_vector_end;
extern uint8_t _general_exception_vector, _general_exception_vector_end;
extern uint8_t _interrupt_vector, _interrupt_vector_end;

static void run_constructors(void)
{
	pfunc *p;

	for (p = &__CTOR_END__[-1]; p >= __CTOR_LIST__; p--) {
		(*p) ();
	}
}

void _ds2_init(void)
{
	/* Copy the exception handlers to their proper locations. */
	memcpy((void*) A_K0BASE, &_tlb_refill_vector,
		&_tlb_refill_vector_end - &_tlb_refill_vector);
	dcache_writeback_range((void*) A_K0BASE,
		&_tlb_refill_vector_end - &_tlb_refill_vector);
	icache_invalidate_range((void*) A_K0BASE,
		&_tlb_refill_vector_end - &_tlb_refill_vector);

	memcpy((void*) (A_K0BASE + 0x100), &_cache_error_vector,
		&_cache_error_vector_end - &_cache_error_vector);
	dcache_writeback_range((void*) (A_K0BASE + 0x100),
		&_cache_error_vector_end - &_cache_error_vector);
	icache_invalidate_range((void*) (A_K0BASE + 0x100),
		&_cache_error_vector_end - &_cache_error_vector);

	memcpy((void*) (A_K0BASE + 0x180), &_general_exception_vector,
		&_general_exception_vector_end - &_general_exception_vector);
	dcache_writeback_range((void*) (A_K0BASE + 0x180),
		&_general_exception_vector_end - &_general_exception_vector);
	icache_invalidate_range((void*) (A_K0BASE + 0x180),
		&_general_exception_vector_end - &_general_exception_vector);

	memcpy((void*) (A_K0BASE + 0x200), &_interrupt_vector,
		&_interrupt_vector_end - &_interrupt_vector);
	dcache_writeback_range((void*) (A_K0BASE + 0x200),
		&_interrupt_vector_end - &_interrupt_vector);
	icache_invalidate_range((void*) (A_K0BASE + 0x200),
		&_interrupt_vector_end - &_interrupt_vector);

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
