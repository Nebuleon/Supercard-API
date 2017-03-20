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

#include <asm/cachectl.h>
#include <assert.h>
#include <ds2/pm.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

/* Allows _Exit to close open files by hooking into single_partition and
 * _FAT_partition_destructor */
#include "partition.h"
#include "../intc.h"
#include "../dma.h"
#include "../ds2_ds/main.h"
#include "../ds2_ds/requests.h"

typedef void (*_atexit_func) (void);

static _atexit_func _atexit_funcs[32];
static size_t _atexit_func_count;

#define RESET_POWER ((0 << 11) | (0 << 12))
#define RESET_MAIN (CPLD_CTR_FPGA_MODE | (0 << 11) | (0 << 12))
#define RESET_TEST ((1 << 11) | (0 << 12))

#define RESET_MAIN_NO_NDS (CPLD_CTR_FPGA_MODE | (1 << 11) | (0 << 12))
#define RESET_PLUGIN ((0 << 11) | (1 << 12))

__attribute__((cold))
void _reset(void)
{
	_intc_init();
	_dma_init();
	cli();

	dcache_invalidate_all();
	icache_invalidate_all();

	REG_CPLD_CTR = CPLD_CTR_FIFO_CLEAR;
	REG_CPLD_CTR = RESET_MAIN_NO_NDS;

	REG_CPLD_FIFO_STATE = CPLD_FIFO_STATE_NDS_CMD_COMPLETE | CPLD_FIFO_STATE_CPU_RTC_CLK_EN;

	__wdt_select_extalclk();
	__wdt_select_clk_div64();
	__wdt_set_data(100);
	__wdt_set_count(0);
	__tcu_start_wdt_clock();
	__wdt_start();
	REG_EMC_DMCR |= EMC_DMCR_RMODE | EMC_DMCR_RFSH;
}

__attribute__((noreturn, cold))
void abort(void)
{
	_Exit(EXIT_FAILURE);
}

int atexit(_atexit_func f)
{
	if (_atexit_func_count >= sizeof(_atexit_funcs) / sizeof(_atexit_funcs[0]))
		return 1;

	_atexit_funcs[_atexit_func_count++] = f;
	return 0;
}

__attribute__((noreturn, cold))
void exit(int status)
{
	for (; _atexit_func_count > 0; _atexit_func_count--) {
		(*_atexit_funcs[_atexit_func_count - 1]) ();
	}

	_Exit(status);
}

__attribute__((noreturn, cold))
void _Exit(int status)
{
	if (single_partition != NULL) {
		_FAT_partition_destructor(single_partition);
	}

	{
		uint32_t section = DS2_EnterCriticalSection();
		_request_nds_reset();
		DS2_LeaveCriticalSection(section);

		/* The reset is awaited here. It will proceed as follows.
		 *
		 * -SC-                           ARM9                           ARM7
		 *      ----[requests.reset]---->
		 * [reset]                              ------[audio_stop]------>
		 *    .                                -[FIFO IPC_START_RESET]->
		 *    .                          [reset]          [swiChangeSoundBias]
		 *    .                             .            [set backlights etc.]
		 *    .                             .                          [reset]
		 *
		 * Next step: ARM9 requests.c: process_requests
		 */
		while (1) {
			DS2_AwaitInterrupt();
		}
	}
}
