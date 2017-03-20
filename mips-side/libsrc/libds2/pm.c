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
#include <stdint.h>

#include "clock.h"

#define LCD_CLK 40000000    //40MHz

#define PLL_LEVELS  16

#define PLL_ELEMENTS 6
/* These are indexes into pll_data[level]. */
#define PLL_M        0  /* The PLL multiplier. 2 is removed from the value of the element. */
#define PLL_N        1  /* The PLL divider. 2 is removed from the value of the element. */
#define PLL_CDIV     2  /* The CPU clock frequency divider. div_data[element value] to convert. */
#define PLL_HDIV     3  /* The system clock frequency divider. div_data[element value] to convert. */
#define PLL_MDIV     4  /* The memory frequency divider. div_data[element value] to convert. */
#define PLL_PDIV     5  /* The peripheral clock frequency divider. div_data[element value] to convert. */

#define DIV_COUNT   10

static const uint8_t pll_data[PLL_LEVELS][PLL_ELEMENTS] = {
	// multipliers and dividers are relative to EXTAL, 24 MHz
	// M,     N,     CDIV, HDIV, MDIV, PDIV          idx, raw, CCLK, HCLK, MCLK, PCLK
	{ 10 - 2, 2 - 2,    1,    1,    1,    1 },  //   [0]  120,   60,   60,   60,   60
	{ 10 - 2, 2 - 2,    0,    1,    1,    1 },  //   [1]  120,  120,   60,   60,   60
	{ 10 - 2, 2 - 2,    0,    0,    0,    0 },  //   [2]  120,  120,  120,  120,  120
	{ 12 - 2, 2 - 2,    0,    1,    1,    1 },  //   [3]  144,  144,   72,   72,   72
	{ 16 - 2, 2 - 2,    0,    1,    1,    1 },  //   [4]  192,  192,   96,   96,   96
	{ 17 - 2, 2 - 2,    0,    0,    1,    1 },  //   [5]  204,  204,  102,  102,  102
	{ 20 - 2, 2 - 2,    0,    1,    1,    1 },  //   [6]  240,  240,  120,  120,  120
	{ 22 - 2, 2 - 2,    0,    2,    2,    2 },  //   [7]  264,  264,   88,   88,   88
	{ 24 - 2, 2 - 2,    0,    2,    2,    2 },  //   [8]  288,  288,   96,   96,   96
	{ 25 - 2, 2 - 2,    0,    2,    2,    2 },  //   [9]  300,  300,  100,  100,  100
	{ 28 - 2, 2 - 2,    0,    2,    2,    2 },  //  [10]  336,  336,  112,  112,  112
	{ 30 - 2, 2 - 2,    0,    2,    2,    2 },  //  [11]  360,  360,  120,  120,  120
	{ 32 - 2, 2 - 2,    0,    2,    2,    2 },  //  [12]  384,  384,  128,  128,  128
	{ 33 - 2, 2 - 2,    0,    2,    2,    2 },  //  [13]  396,  396,  132,  132,  132

	{ 33 - 2, 2 - 2,    0,    2,    2,    2 },  //  [14]  396,  396,  132,  132,  132
	{ 33 - 2, 2 - 2,    0,    2,    2,    2 },  //  [15]  396,  396,  132,  132,  132
};

/* This applies to CDIV, HDIV, MDIV and PDIV. */
static const uint8_t div_data[DIV_COUNT] = {
	1, 2, 3, 4, 6, 8, 12, 16, 24, 32
};

static void _sdram_convert(uint32_t pll_hz)
{
	uint32_t ns, dmcr, rtcsr, tmp, div, divm;

	dmcr = REG_EMC_DMCR;

	dmcr &= ~(EMC_DMCR_TRAS_MASK | EMC_DMCR_RCD_MASK | EMC_DMCR_TPC_MASK |
	          EMC_DMCR_TRWL_MASK | EMC_DMCR_TRC_MASK);

	ns = UINT32_C(1000000000) / pll_hz;
	/* RAS# active time */
	tmp = SDRAM_TRAS / ns;
	if (tmp < 4) tmp = 4;
	if (tmp > 11) tmp = 11;
	dmcr |= ((tmp - 4) << EMC_DMCR_TRAS_BIT);

	/* RAS# to CAS# delay */
	tmp = SDRAM_RCD / ns;
	if (tmp > 3) tmp = 3;
	dmcr |= (tmp << EMC_DMCR_RCD_BIT);

	/* RAS# precharge time */
	tmp = SDRAM_TPC / ns;
	if (tmp > 7) tmp = 7;
	dmcr |= (tmp << EMC_DMCR_TPC_BIT);

	/* Write latency */
	tmp = SDRAM_TRWL / ns;
	if (tmp > 3) tmp = 3;
	dmcr |= (tmp << EMC_DMCR_TRWL_BIT);

	/* RAS cycle time */
	tmp = (SDRAM_TRAS + SDRAM_TPC) / ns;
	if (tmp > 14) tmp = 14;
	dmcr |= (((tmp + 1) >> 1) << EMC_DMCR_TRC_BIT);

	REG_EMC_DMCR = dmcr;

	rtcsr = REG_EMC_RTCSR;

	/* Set the new refresh timing */
	tmp = SDRAM_TREF / ns;
	div = (tmp + 254) / 255;
	if (div <= 4) div = 1;       // in units of CKO/4
	else if (div <= 16) div = 2; // in units of CKO/16
	else div = 3;                // in units of CKO/64

	rtcsr &= ~EMC_RTCSR_CKS_MASK;
	rtcsr |= div << EMC_RTCSR_CKS_BIT;

	REG_EMC_RTCSR = rtcsr;

	divm = 4 << (2 * div);

	tmp = tmp / divm + 1;

	REG_EMC_RTCOR = tmp;
	REG_EMC_RTCNT = tmp - 1;  /* Refresh SDRAM right now */
}

/* convert pll while program is running */
static int _pm_pllconvert(size_t level)
{
	uint32_t freq_b;
	uint32_t cpccr, cppcr;

	if (level >= PLL_LEVELS) return -1;

	freq_b = (pll_data[level][PLL_M] + 2) * EXTAL_CLK
	       / (pll_data[level][PLL_N] + 2);

	_sdram_convert(freq_b / div_data[pll_data[level][PLL_MDIV]]);

	cpccr = REG_CPM_CPCCR;
	cppcr = REG_CPM_CPPCR;

	cppcr &= ~(CPM_CPPCR_PLLM_MASK | CPM_CPPCR_PLLN_MASK);
	cppcr |= (pll_data[level][PLL_M] << CPM_CPPCR_PLLM_BIT)
	       | (pll_data[level][PLL_N] << CPM_CPPCR_PLLN_BIT);

	cpccr &= ~(CPM_CPCCR_CDIV_MASK | CPM_CPCCR_HDIV_MASK | CPM_CPCCR_PDIV_MASK
	         | CPM_CPCCR_MDIV_MASK | CPM_CPCCR_LDIV_MASK);
	cpccr |= (pll_data[level][PLL_CDIV] << CPM_CPCCR_CDIV_BIT)
	       | (pll_data[level][PLL_HDIV] << CPM_CPCCR_HDIV_BIT)
	       | (pll_data[level][PLL_MDIV] << CPM_CPCCR_MDIV_BIT)
	       | (pll_data[level][PLL_PDIV] << CPM_CPCCR_PDIV_BIT)
	       | (31 << CPM_CPCCR_LDIV_BIT) /* LCD device clock frequency <= 150 MHz */;

	REG_CPM_CPCCR = cpccr;
	REG_CPM_CPPCR = cppcr;

	while (!(REG_CPM_CPPCR & CPM_CPPCR_PLLS)); /* Wait for the PLL to stabilise */

	_detect_clock();
	return 0;
}

int DS2_LowClockSpeed(void)
{
	return _pm_pllconvert(11);
}

int DS2_NominalClockSpeed(void)
{
	return _pm_pllconvert(11);
}

int DS2_HighClockSpeed(void)
{
	return _pm_pllconvert(11);
}
