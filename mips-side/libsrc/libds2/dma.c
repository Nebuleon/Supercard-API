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

#include <stdbool.h>
#include <ds2/pm.h>

#include "jz4740.h"
#include "dma.h"
#include "intc.h"

static bool dmac_started;

static bool dma_ch_in_use[MAX_DMA_NUM];
static bool dma_ch_transfer_size_shift[MAX_DMA_NUM];
static bool dma_ch_triggers_irq[MAX_DMA_NUM];

#define PHYSADDR(addr) ((uintptr_t) (addr) & 0x1FFFFFFF)

/* Using >> on a number of bytes to be transferred will yield the number of
 * transfer units. The index into the array depends on the value of
 * DMAC_DCMD_DS_MASK in REG_DMAC_DCMD. */
static const uint8_t TRANSFER_SIZE_SHIFTS[8] = { 2, 0, 1, 4, 5, 0, 0, 0 };

void _dma_init(void)
{
	int i;

	REG_DMAC_DMACR = 0;  /* Global DMA disable */
	REG_DMAC_DMAIPR = 0;  /* Clear pending DMA interrupts */
	for (i = 0; i < MAX_DMA_NUM; i++) {
		REG_DMAC_DCCSR(i) = 0;  /* Disable DMA channel */
	}
}

int dma_request(void (*irq_handler) (unsigned int), unsigned int arg,
	uint32_t type, uint32_t mode)
{
	int i;

	if (!dmac_started) {
		__cpm_start_dmac();
		__dmac_enable_module();
		dmac_started = true;
	}

	for (i = 0; i < MAX_DMA_NUM; i++) {
		if (!dma_ch_in_use[i]) break;
	}
	if (i == MAX_DMA_NUM)
		return -1;

	dma_ch_in_use[i] = true;
	dma_ch_transfer_size_shift[i] = TRANSFER_SIZE_SHIFTS[((mode & DMAC_DCMD_DS_MASK) >> DMAC_DCMD_DS_BIT)];
	REG_DMAC_DCKE |= DMAC_DCKE_CHN_ON(i);  /* Start this channel's clock */
	REG_DMAC_DCCSR(i) = 0;
	REG_DMAC_DRSR(i) = type;
	REG_DMAC_DCMD(i) = mode;

	if (irq_handler) {
		dma_ch_triggers_irq[i] = true;
		irq_request(IRQ_DMA_0 + i, irq_handler, arg);
	} else {
		dma_ch_triggers_irq[i] = false;
	}

	return 0;
}

void dma_free(int ch)
{
	if (ch < 0 || ch >= MAX_DMA_NUM || !dma_ch_in_use[ch])
		return;

	REG_DMAC_DCCSR(ch) = 0;
	if (dma_ch_triggers_irq[ch])
		__dmac_channel_disable_irq(ch);

	REG_DMAC_DCKE &= ~DMAC_DCKE_CHN_ON(ch);  /* Stop this channel's clock */
	dma_ch_in_use[ch] = false;
	dma_ch_triggers_irq[ch] = false;
}

void dma_start(int ch, const void* src, void* dst, uint32_t count)
{
	if (ch < 0 || ch >= MAX_DMA_NUM)
		return;

	REG_DMAC_DCCSR(ch) = 0;

	REG_DMAC_DSAR(ch) = PHYSADDR(src);
	REG_DMAC_DTAR(ch) = PHYSADDR(dst);
	REG_DMAC_DTCR(ch) = count >> dma_ch_transfer_size_shift[ch];
	REG_DMAC_DCCSR(ch) = DMAC_DCCSR_NDES | DMAC_DCCSR_EN; /* No-descriptor transfer */
}

void dma_join(int ch)
{
	if (ch < 0 || ch >= MAX_DMA_NUM)
		return;

	if (REG_DMAC_DCCSR(ch) & DMAC_DCCSR_EN) {
		while (!__dmac_channel_transmit_end_detected(ch));
	}
}
