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

#include <mipsregs.h>
#include "bsp.h"
#include "jz4740.h"
#include "intc.h"

static struct {
	void (*handler)(unsigned int);
	unsigned int arg;
} irq_table[IRQ_MAX];

static void default_handler(unsigned int arg)
{
	/* Yep. TODO: Add a way to notify the developer about this. */
	// Received interrupt 'arg' without a registered handler...
	while (1);
}

static uint32_t dma_irq_mask = 0;
static uint32_t gpio_irq_mask[6];

static void irq_enable(unsigned int irq)
{
	unsigned int t;
	if ((irq >= IRQ_GPIO_0) && (irq <= IRQ_GPIO_0 + NUM_GPIO)) {
		__gpio_unmask_irq(irq - IRQ_GPIO_0);
		t = (irq - IRQ_GPIO_0) >> 5;
		gpio_irq_mask[t] |= (1 << ((irq - IRQ_GPIO_0) & 0x1f));
		__intc_unmask_irq(IRQ_GPIO0 - t);
	} else if ((irq >= IRQ_DMA_0) && (irq <= IRQ_DMA_0 + MAX_DMA_NUM)) {
		__dmac_channel_enable_irq(irq - IRQ_DMA_0);
		dma_irq_mask |= (1 << (irq - IRQ_DMA_0));
		__intc_unmask_irq(IRQ_DMAC);
	} else if (irq < 32)
		__intc_unmask_irq(irq);
}

static void irq_disable(unsigned int irq)
{
	unsigned int t;
	if ((irq >= IRQ_GPIO_0) && (irq <= IRQ_GPIO_0 + NUM_GPIO)) {
		__gpio_mask_irq(irq - IRQ_GPIO_0);
		t = (irq - IRQ_GPIO_0) >> 5;
		gpio_irq_mask[t] &= ~(1 << ((irq - IRQ_GPIO_0) & 0x1f));
		if (!gpio_irq_mask[t])
			__intc_mask_irq(IRQ_GPIO0 - t);
	} else if ((irq >= IRQ_DMA_0) && (irq <= IRQ_DMA_0 + MAX_DMA_NUM)) {
		__dmac_channel_disable_irq(irq - IRQ_DMA_0);
		dma_irq_mask &= ~(1 << (irq - IRQ_DMA_0));
		if (!dma_irq_mask)
			__intc_mask_irq(IRQ_DMAC);
	} else if (irq < 32)
		__intc_mask_irq(irq);
}

static void irq_ack(unsigned int irq)
{
	if ((irq >= IRQ_GPIO_0) && (irq <= IRQ_GPIO_0 + NUM_GPIO)) {
		__intc_ack_irq(IRQ_GPIO0 - ((irq - IRQ_GPIO_0)>>5));
		__gpio_ack_irq(irq - IRQ_GPIO_0);
	} else if ((irq >= IRQ_DMA_0) && (irq <= IRQ_DMA_0 + MAX_DMA_NUM)) {
		__intc_ack_irq(IRQ_DMAC);
	} else if (irq < 32) {
		__intc_ack_irq(irq);
		if (irq == IRQ_TCU2)
			__tcu_clear_full_match_flag(4);
		else if(irq == IRQ_TCU1)
			__tcu_clear_full_match_flag(5);
	}
}

static uint32_t ipl;  /* Interrupt Pending List */

static int intc_irq(void)
{
	int irq;
	
	ipl |= REG_INTC_IPR;

	if (ipl == 0)
		return -1;

	irq = _fhs32(ipl);

	ipl &= ~(1 << irq);

	/* Map this to the flat IRQ numbers (0 .. IRQ_MAX - 1) */
	switch (irq) {
	case IRQ_GPIO0:
		irq = __gpio_group_irq(0) + IRQ_GPIO_0;
		break;
	case IRQ_GPIO1:
		irq = __gpio_group_irq(1) + IRQ_GPIO_0 + 32;
		break;
	case IRQ_GPIO2:
		irq = __gpio_group_irq(2) + IRQ_GPIO_0 + 64;
		break;
	case IRQ_GPIO3:
		irq = __gpio_group_irq(3) + IRQ_GPIO_0 + 96;
		break;
	case IRQ_GPIO4:
		irq = __gpio_group_irq(4) + IRQ_GPIO_0 + 128;
		break;
	case IRQ_GPIO5:
		irq = __gpio_group_irq(5) + IRQ_GPIO_0 + 160;
		break;
	case IRQ_DMAC:
		irq = __dmac_get_irq() + IRQ_DMA_0;
		break;
	}

	return irq;
}

void _intc_init(void)
{
	unsigned int i;
	ipl = 0;
	for (i = 0; i < IRQ_MAX; i++) {
		irq_disable(i);
		irq_table[i].handler = default_handler;
		irq_table[i].arg = i;
	}
}

int irq_request(unsigned int irq, void (*handler) (unsigned int), unsigned int arg)
{
	if (irq >= IRQ_MAX)
		return -1;

	irq_table[irq].handler = handler;
	irq_table[irq].arg = arg;
	irq_enable(irq);

	return 0;
}

void irq_free(unsigned int irq)
{
	irq_disable(irq);
	irq_table[irq].handler = default_handler;
	irq_table[irq].arg = irq;
}

void _irq_handler()
{
	int irq = intc_irq();

	if (irq < 0)
		return;

	irq_ack(irq);

	irq_table[irq].handler(irq_table[irq].arg);
}

void cli(void)
{
	uint32_t t = read_c0_status();
	t &= ~1;
	write_c0_status(t);
}

void sti(void)
{
	uint32_t t = read_c0_status();
	t |= 1;
	write_c0_status(t);
}

uint32_t DS2_EnterCriticalSection(void)
{
	uint32_t t = read_c0_status();
	write_c0_status(t & ~1);
	return t;
}

void DS2_LeaveCriticalSection(uint32_t val)
{
	write_c0_status(val);
}

