/*
 * linux/arch/mips/jz4740/common/pm.c
 * 
 * JZ4740 Power Management Routines
 * 
 * Copyright (C) 2006 Ingenic Semiconductor Inc.
 * Author: <jlwei@ingenic.cn>
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 */

#include "jz4740.h"
#include "pm.h"
#include <ds2/pm.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#define NOMINAL_CPU_FREQ UINT32_C(360000000)
#define NOMINAL_RAM_FREQ UINT32_C(120000000)

#define DIV_COUNT   10

static uint32_t cpu_hz __attribute__((section(".noinit")));

/* This applies to CDIV, HDIV, MDIV and PDIV. */
static const uint8_t div_data[DIV_COUNT] = {
	1, 2, 3, 4, 6, 8, 12, 16, 24, 32
};

static void _sdram_convert(uint32_t pll_hz)
{
	const uint32_t ns = UINT32_C(1000000000) / pll_hz;
	uint32_t delay;

	delay = (SDRAM_TREF / ns) / 64 + 1;
	if (delay > 0xFF) delay = 0xFF;

	REG_EMC_RTCOR = delay;
	REG_EMC_RTCNT = delay - 1;  /* Refresh SDRAM right now */
}

int _clock_convert(uint32_t* restrict cpu_hz, uint32_t* restrict mem_hz)
{
	const uint32_t ext_hz = EXTAL_CLK;
	/* A smaller N is better. (We simply use the minimum allowed value.) */
	const uint32_t pll_n = 2;
	const uint32_t min_pll_hz = (100000000 + ext_hz / pll_n - 1)
		/ (ext_hz / pll_n) * (ext_hz / pll_n);
	const uint32_t max_pll_hz = 500000000 / (ext_hz / pll_n) * (ext_hz / pll_n);

	uint32_t hz = *cpu_hz, pll_m = 2, cdiv = 0, mdiv = 0;
	uint32_t cpccr, cppcr;

	/* If the requested CPU frequency would leave the PLL clock below 100 MHz,
	 * increase the PLL clock (and require a CPU clock divider later). If this
	 * is not done, the CPU clock can only be 108, 54, 36, 27... MHz. */
	if (*cpu_hz < min_pll_hz) {
		size_t pmul = 0;
		while (pmul < DIV_COUNT && min_pll_hz / div_data[pmul] > *cpu_hz)
			pmul++;
		if (pmul >= DIV_COUNT)
			return EINVAL;
		hz = *cpu_hz * div_data[pmul];
	}

	/* Restriction: The PLL, after multiplication by M and division by N, must
	 * output between 100 MHz and 500 MHz. The Output Divider does not matter.
	 * (The Output Divider is not currently implemented.) */
	if (hz < min_pll_hz)
		hz = min_pll_hz;
	else if (hz > max_pll_hz)
		hz = max_pll_hz;

	/* Restriction: M must be between 2 and 513 inclusive. Met implicitly by
	 * the requirement for the PLL to be between 100 MHz and 500 MHz, with N
	 * = 2. */
	pll_m = hz / (ext_hz / pll_n);
	hz = ext_hz * pll_m / pll_n;

	while (cdiv < DIV_COUNT && hz / div_data[cdiv] > *cpu_hz)
		cdiv++;
	while (mdiv < DIV_COUNT && hz / div_data[mdiv] > *mem_hz)
		mdiv++;

	/* Restrictions: CCLK must be an integral multiple of HCLK; HCLK is either
	 * equal to MCLK or twice MCLK. (Currently, HCLK == MCLK.) */
	if (cdiv >= DIV_COUNT || mdiv >= DIV_COUNT)
		return EINVAL;
	while (mdiv < DIV_COUNT && div_data[mdiv] % div_data[cdiv] != 0)
		mdiv++;

	/* Restriction: The frequency ratio of CCLK and HCLK cannot be 24 or 32.
	 * (Currently, HCLK == MCLK.) */
	if (mdiv >= DIV_COUNT)
		return EINVAL;
	if (div_data[cdiv] / div_data[mdiv] >= 24)
		return EINVAL;

	/* Empirical evidence suggests that a transition from 360 MHz to any clock
	 * speed above 100 MHz for the CPU with a memory divider of 1 is unstable.
	 * Set the dividers to 2 in that case. */
	if (div_data[mdiv] == 1 && hz / div_data[cdiv] >= 100000000) {
		hz = *cpu_hz < *mem_hz ? *cpu_hz : *mem_hz;
		pll_m = (hz * 2) / (ext_hz / pll_n);
		hz = ext_hz * pll_m / pll_n;
		cdiv++;
		mdiv++;
	}

	*cpu_hz = hz / div_data[cdiv];
	*mem_hz = hz / div_data[mdiv];

	_sdram_convert(hz / div_data[mdiv]);

	cpccr = REG_CPM_CPCCR;
	cppcr = REG_CPM_CPPCR;

	cppcr &= ~(CPM_CPPCR_PLLM_MASK | CPM_CPPCR_PLLN_MASK);
	cppcr |= ((pll_m - 2) << CPM_CPPCR_PLLM_BIT)
	       | ((pll_n - 2) << CPM_CPPCR_PLLN_BIT);

	cpccr &= ~(CPM_CPCCR_CDIV_MASK | CPM_CPCCR_HDIV_MASK | CPM_CPCCR_PDIV_MASK
	         | CPM_CPCCR_MDIV_MASK | CPM_CPCCR_LDIV_MASK);
	cpccr |= (cdiv << CPM_CPCCR_CDIV_BIT)
	       | (mdiv << CPM_CPCCR_HDIV_BIT)
	       | (mdiv << CPM_CPCCR_MDIV_BIT)
	       | (mdiv << CPM_CPCCR_PDIV_BIT)
	       | (31 << CPM_CPCCR_LDIV_BIT) /* LCD device clock frequency <= 150 MHz */;

	REG_CPM_CPCCR = cpccr;
	REG_CPM_CPPCR = cppcr;

	while (!(REG_CPM_CPPCR & CPM_CPPCR_PLLS)); /* Wait for the PLL to stabilise */

	_detect_clock();
	return 0;
}

int DS2_SetClockSpeed(uint32_t* restrict cpu_hz, uint32_t* restrict mem_hz)
{
	uint32_t cur_cpu_hz, cur_mem_hz;
	DS2_GetClockSpeed(&cur_cpu_hz, &cur_mem_hz);

	/* Transitions between clock speeds are smoother if they go through the
	 * nominal frequencies first. */
	if (cur_cpu_hz != NOMINAL_CPU_FREQ || cur_mem_hz != NOMINAL_RAM_FREQ) {
		int result;
		cur_cpu_hz = NOMINAL_CPU_FREQ;
		cur_mem_hz = NOMINAL_RAM_FREQ;
		if ((result = _clock_convert(&cur_cpu_hz, &cur_mem_hz)) != 0) {
			*cpu_hz = cur_cpu_hz;
			*mem_hz = cur_mem_hz;
			return result;
		}
	}

	/* If the requests are for the nominal frequencies, they'd be wasteful
	 * now. Otherwise, run them. */
	if (*cpu_hz != NOMINAL_CPU_FREQ || *mem_hz != NOMINAL_RAM_FREQ) {
		return _clock_convert(cpu_hz, mem_hz);
	}
	return 0;
}

int DS2_LowClockSpeed(void)
{
	uint32_t cpu_hz = UINT32_C(60000000), mem_hz = UINT32_C(60000000);
	return DS2_SetClockSpeed(&cpu_hz, &mem_hz);
}

int DS2_NominalClockSpeed(void)
{
	uint32_t cpu_hz = UINT32_C(360000000), mem_hz = UINT32_C(120000000);
	return DS2_SetClockSpeed(&cpu_hz, &mem_hz);
}

int DS2_HighClockSpeed(void)
{
	uint32_t cpu_hz = UINT32_C(396000000), mem_hz = UINT32_C(132000000);
	return DS2_SetClockSpeed(&cpu_hz, &mem_hz);
}

int DS2_GetClockSpeed(uint32_t* restrict cpu_hz, uint32_t* restrict mem_hz)
{
	static const uint8_t out_div_shifts[4] = { 0, 1, 1, 2 };

	uint32_t cppcr = REG_CPM_CPPCR, cpccr = REG_CPM_CPCCR;
	uint32_t hz;

	if ((cppcr & CPM_CPPCR_PLLEN) && !(cppcr & CPM_CPPCR_PLLBP)) {
		uint32_t pll_m = (cppcr & CPM_CPPCR_PLLM_MASK) >> CPM_CPPCR_PLLM_BIT;
		uint32_t pll_n = (cppcr & CPM_CPPCR_PLLN_MASK) >> CPM_CPPCR_PLLN_BIT;
		uint32_t pll_no = (cppcr & CPM_CPPCR_PLLOD_MASK) >> CPM_CPPCR_PLLOD_BIT;
		hz = (EXTAL_CLK >> out_div_shifts[pll_no]) / (pll_n + 2) * (pll_m + 2);
	} else
		hz = EXTAL_CLK;

	*cpu_hz = hz / div_data[(cpccr & CPM_CPCCR_CDIV_MASK) >> CPM_CPCCR_CDIV_BIT];
	*mem_hz = hz / div_data[(cpccr & CPM_CPCCR_MDIV_MASK) >> CPM_CPCCR_MDIV_BIT];
	return 0;
}

void _detect_clock(void)
{
	uint32_t mem_hz;
	DS2_GetClockSpeed(&cpu_hz, &mem_hz);
}

int usleep(useconds_t usec)
{
	if (usec >= 1000000) {
		errno = EINVAL;
		return -1;
	}

	uint32_t i = usec * (cpu_hz / 2000000);

	__asm__ __volatile__ (
		"\t.set push\n"
		"\t.set noreorder\n"
		"1:\n\t"
		"bne\t%0, $0, 1b\n\t"
		"addiu\t%0, %0, -1\n\t"
		".set pop\n"
		: "=r" (i)
		: "0" (i)
	);
	return 0;
}

unsigned int sleep(unsigned int seconds)
{
	for (; seconds > 0; seconds--)
		usleep(999999);
	return 0;
}
