/*
 * This file is part of the DS communication library for the Supercard DSTwo.
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
#include <ds2/pm.h>

#include "main.h"
#include "globals.h"
#include "../dma.h"
#include "../intc.h"
#include "../jz4740.h"

static void data_interrupt_enable(void)
{
	__gpio_as_irq_rise_edge(fpga_data_port + nds_data_iqe_pinx);
	__gpio_unmask_irq(fpga_data_port + nds_data_iqe_pinx);
}

static void data_interrupt_disable(void)
{
	__gpio_as_disable_irq(fpga_data_port + nds_data_iqe_pinx);
}

static void cmd_interrupt_enable(void)
{
	__gpio_as_irq_fall_edge(fpga_cmd_port + nds_cmd_iqe_pinx);
	__gpio_unmask_irq(fpga_cmd_port + nds_cmd_iqe_pinx);
}

static void cmd_interrupt_disable(void)
{
	__gpio_as_disable_irq(fpga_cmd_port + nds_cmd_iqe_pinx);
}

/* This function is called when the FIFO that sends to the Nintendo DS is
 * empty, and more data is ready to be sent. Depending on the local send
 * queue, nothing may need to be sent, in which case the data empty interrupt
 * gets disabled. */
static void data_interrupt_handler(unsigned int arg)
{
	
}

static void cmd_interrupt_handler(unsigned int arg)
{
	union card_command command;
	int i;

	for (i = 0; i < 4; i++) {
		command.halfwords[i] = REG_CPLD_FIFO_READ_NDSWCMD;
	}

	_ds2_ds.current_protocol(&command);
}

static void fpga_gpio_init(void)
{
	GPIO_ADDR_RECOVER();

	__gpio_disable_pull(fpga_cmd_port + nds_cmd_iqe_pinx);
	data_interrupt_disable();
	__gpio_disable_pull(fpga_data_port + nds_data_iqe_pinx);

	INIT_FPGA_PORT();

	REG_CPLD_CTR = CPLD_CTR_KEY2_INIT_EN;

	SET_ADDR_GROUP(GPIO_ADDR_GROUP2);

	*(volatile uint16_t*) CPLD_BASE = 0x6BF3;
	*(volatile uint16_t*) (CPLD_BASE + CPLD_STEP * 1) = 0xF0C2;
	*(volatile uint16_t*) (CPLD_BASE + CPLD_STEP * 2) = 0x9252;

	SET_ADDR_DEFT();
	REG_CPLD_CTR = 0;

	usleep(1);
}

static void fpga_init(void)
{
	RESET_FPGA_FIFO();
}

static void init_variables(void)
{
	size_t i;

	_ds2_ds.snd_started = false;

	_ds2_ds.txt_size = 0;

	_ds2_ds.vid_formats[0] = DS2_PIXEL_FORMAT_BGR555;
	_ds2_ds.vid_formats[1] = DS2_PIXEL_FORMAT_BGR555;
	_ds2_ds.vid_main_displayed = 0;
	_ds2_ds.vid_main_current = 0;
	_ds2_ds.vid_swap = false;
	_ds2_ds.vid_backlights = DS_SCREEN_BOTH;
	_ds2_ds.vid_last_was_flip = false;
	for (i = 0; i < MAIN_BUFFER_COUNT; i++) {
		_ds2_ds.vid_main_busy[i] = 0;
	}
	_ds2_ds.vid_sub_busy = 0;
	_ds2_ds.vid_queue_count = 0;
	_ds2_ds.vblank_count = 0;

	_ds2_ds.link_status = LINK_STATUS_NONE;
	_ds2_ds.pending_recvs = PENDING_RECV_ALL;
	_ds2_ds.pending_sends = 0;
	_ds2_ds.current_protocol = _link_establishment_protocol;

	memset(&_ds2_ds.in_presses, 0, sizeof(_ds2_ds.in_presses));
	memset(&_ds2_ds.in_releases, 0, sizeof(_ds2_ds.in_releases));

	memset(&_ds2_ds.requests, 0, sizeof(_ds2_ds.requests));
}

int _ds2_ds_init(void)
{
	fpga_gpio_init();
	fpga_init();

	if (irq_request(NDS_CMD_IRQ, cmd_interrupt_handler, 0) < 0) {
		dgprintf("Failed to register the Nintendo DS command interrupt\n");
		goto cmd_irq_failed;
	}

	if (irq_request(NDS_DATA_IRQ, data_interrupt_handler, 0) < 0) {
		dgprintf("Failed to register the Nintendo DS data interrupt\n");
		goto data_irq_failed;
	}

	if ((_ds2_ds.dma_channel = dma_request(NULL, 0, DMAC_DRSR_RS_AUTO,
	     DMAC_DCMD_SAI | DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_16 | DMAC_DCMD_DS_32BIT)) < 0) {
		dgprintf("Failed to reserve a DMA channel for writes to the Nintendo DS FIFO\n");
		goto dma_failed;
	}

	init_variables();

	/* MIPS-ARM9 SYNC AWAIT 2: We are waiting for the Nintendo DS to assert
	 * its card command line. This will indicate that it, too, is ready... and
	 * that we must read its command and reply to it. */
	cmd_interrupt_enable();

	/* MIPS-ARM9 SYNC PROVIDE 1: We begin to assert the card line for the DS.
	 * The fact that we're trying this signals that our initialisation is
	 * done, and that our version of the link state is as follows:
	 *
	 * - The DS's video mode is assumed to be graphical
	 * - Main Screen buffers 0, 1 and 2, and Sub Screen buffer 0 are opaque
	 *   black
	 * - Among the Main Screen buffers, number 0 is displayed
	 * - None of the buffers are being transferred
	 * - Audio is stopped
	 * - Screens are not swapped (i.e. the Main Screen is on the bottom)
	 * - Both screen backlights are on
	 * - No reading is available for the Nintendo DS's buttons
	 * - No reading is available for the Nintendo DS's real-time clock
	 */
	DS2_StartAwait();
	while (_ds2_ds.link_status == LINK_STATUS_NONE) {
		/* With our interrupts temporarily disabled (because we don't want the
		 * card line to be stuck high when replies get sent later on)... */
		uint32_t section = DS2_EnterCriticalSection();
		/* Poke the DS by setting the card line to high... */
		REG_CPLD_FIFO_STATE = CPLD_FIFO_STATE_NDS_IQE_OUT;
		/* ... then setting it back to low. The interrupt is edge-triggered,
		 * so this much was all that is really needed. */
		REG_CPLD_FIFO_STATE = 0;
		DS2_LeaveCriticalSection(section);
		/* Wait 1 millisecond for the DS to give us an interrupt back. If it
		 * hasn't, the link status will still be LINK_STATUS_NONE, since our
		 * card command interrupt handler won't have run. This can happen if
		 * the DS, not being ready for interrupts yet, simply drops them. */
		usleep(1000);
	}
	DS2_StopAwait();

	DS2_StartAwait();
	while (_ds2_ds.link_status != LINK_STATUS_ESTABLISHED)
		DS2_AwaitInterrupt();
	DS2_StopAwait();

	return 0;

dma_failed:
	irq_free(NDS_DATA_IRQ);

data_irq_failed:
	irq_free(NDS_CMD_IRQ);

cmd_irq_failed:
	return -1;
}
