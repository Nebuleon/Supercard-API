/*
 * This file is part of the C standard library for the Supercard DSTwo.
 *
 * Copyright 2006 Ingenic Semiconductor Inc.
 *                author: Seeger Chin <seeger.chin@gmail.com>
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

#include "bsp.h"
#include "jz4740.h"

void _clock_init(void)
{
	REG_CPM_CPCCR = CPM_CPCCR_CLKOEN | CPM_CPCCR_PCS | CPM_CPCCR_CE
	              | (2 << CPM_CPCCR_MDIV_BIT)
	              | (2 << CPM_CPCCR_PDIV_BIT)
	              | (2 << CPM_CPCCR_HDIV_BIT)
	              | (0 << CPM_CPCCR_CDIV_BIT)
	              /* The LCD divider must leave the LCD clock under 150 MHz */
	              | (31 << CPM_CPCCR_LDIV_BIT);
	REG_CPM_CPPCR = ((30 - 2) << CPM_CPPCR_PLLM_BIT) /* Clock multiplier 30 */
	              | ((2 - 2) << CPM_CPPCR_PLLN_BIT) /* Clock divider 2 */
	              | CPM_CPPCR_PLLEN
	              | (17 << CPM_CPPCR_PLLST_BIT) /* 17/32768 s to stabilise */;
	while (!(REG_CPM_CPPCR & CPM_CPPCR_PLLS)); /* Wait for the PLL to stabilise */
}
