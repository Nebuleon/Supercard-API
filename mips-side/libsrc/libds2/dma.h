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

#ifndef __DMA_H__
#define __DMA_H__

#include <stdint.h>

// Initialises the DMA subsystem.
void _dma_init(void);

/*
 * Reserves a DMA channel for operations of the given 'mode' and 'type'.
 * Also starts its clock.
 *
 * In:
 *   irq_handler: If not NULL, the DMA is set up to trigger interrupts when
 *     it's done with a transfer, and this handler will be called with the
 *     value given in 'arg' as its sole argument.
 *   arg: For the handler.
 *   type: Value to be written to REG_DMAC_DRSR corresponding to the channel
 *     that gets reserved. Refer to the macros starting with DMAC_DRSR_ in
 *     jz4740.h.
 *   mode: Value to be written to REG_DMAC_DCMD corresponding to the channel
 *     that gets reserved. Refer to the macros starting with DMAC_DCMD_ in
 *     jz4740.h.
 * Returns:
 *   The number of the DMA channel that was reserved, or -1 if none is
 *   available.
 */
extern int dma_request(void (*irq_handler) (unsigned int), unsigned int arg,
	uint32_t type, uint32_t mode);

/*
 * Starts a DMA transfer on a reserved channel. Undefined behavior will
 * result if the channel was not reserved with a call to dma_request: its
 * clock will not be running.
 *
 * In:
 *   ch: The DMA channel to be used.
 *   src: The source address of the transfer.
 *   dst: The destination address of the transfer.
 *   count: The number of bytes to transfer. This number will be adjusted
 *     downward to the previous full transfer unit, whose size was selected
 *     in REG_DMAC_DCMD ('type' parameter of dma_request).
 * Input assumptions:
 *   RAM holds the most recent version of data from 'src' to 'src + count'.
 *   Use dcache_writeback_range from asm/cachectl.h to ensure that the data
 *   cache is written back to RAM, if applicable.
 */
extern void dma_start(int ch, const void* src, void* dst, uint32_t count);

/*
 * Ends a DMA channel reservation. Also stops its clock. Passing a channel
 * that is not in use does nothing, and is not an error.
 *
 * In:
 *   ch: The DMA channel number.
 */
extern void dma_free(int ch);

/*
 * Waits until the last transfer started on the given DMA channel is done.
 * Undefined behavior will result if the channel was not reserved with a call
 * to dma_request: its clock will not be running.
 *
 * In:
 *   ch: The DMA channel number.
 */
extern void dma_join(int ch);

#endif //__DMA_H__
