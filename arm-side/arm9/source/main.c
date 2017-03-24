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
#include <nds.h>
#include <stddef.h>

#include "audio.h"
#include "card_protocol.h"
#include "common_ipc.h"
#include "main.h"
#include "structs.h"
#include "video.h"

/* Bitmask of things we need to send to the Supercard.
 * volatile because it is modified by the card line interrupt handler and FIFO
 * handler, and is then tested in a loop. */
static volatile DTCM_DATA uint32_t pending_sends;

static struct DS_RTC rtc;

static struct DS_InputState input;

void add_pending_send(uint32_t mask)
{
	pending_sends |= mask;
}

void remove_pending_send(uint32_t mask)
{
	pending_sends &= ~mask;
}

static uint32_t take_pending_send()
{
	uint32_t sends = pending_sends;
	uint32_t result = sends & (~sends + 1); /* Get the lowest set bit */
	pending_sends = sends & ~result; /* Remove it from the bitfield */
	return result;
}

void init_regs()
{
	REG_EXMEMCNT = ARM7_MAIN_RAM_PRIORITY;
}

void card_line_handler()
{
	if (link_status == LINK_STATUS_NONE) {
		/* MIPS-ARM9 SYNC PROVIDE 2: The interrupt logic has deasserted the
		 * card line. Our part is done. */
		link_establishment_protocol();
	} else if (link_status == LINK_STATUS_ESTABLISHED) {
		/* When the link is established, this is a way to get us to issue the
		 * send queue command. */
		int previous_ime = enterCriticalSection();
		add_pending_send(PENDING_SEND_QUEUE);
		leaveCriticalSection(previous_ime);
	}
}

void fifo_value_handler(uint32_t value, void* userdata)
{
	uint16_t reply_type = value & 0xFFFF;
	int section = enterCriticalSection();

	switch (reply_type) {
	case IPC_RPL_INPUT_BUTTONS:
	{
		uint16_t reply_data = (value & RPL_INPUT_BUTTONS_MASK) >> RPL_INPUT_BUTTONS_BIT;
		input.buttons = ((REG_KEYINPUT & 0x3FF) ^ 0x3FF)
		              | reply_data;
		if (!(reply_data & KEY_TOUCH)) {
			/* Don't expect touch data; send this immediately */
			input.touch_x = 0;
			input.touch_y = 0;
			add_pending_send(PENDING_SEND_INPUT);
		}
		break;
	}

	case IPC_RPL_INPUT_TOUCH:
		input.touch_x = (value & RPL_INPUT_TOUCH_X_MASK) >> RPL_INPUT_TOUCH_X_BIT;
		input.touch_y = (value & RPL_INPUT_TOUCH_Y_MASK) >> RPL_INPUT_TOUCH_Y_BIT;
		add_pending_send(PENDING_SEND_INPUT);
		break;
	}

	leaveCriticalSection(section);
}

void fifo_datamsg_handler(int bytes, void* userdata)
{
	if (bytes == 7) {
		int section = enterCriticalSection();
		fifoGetDatamsg(FIFO_USER_01, 7, (uint8_t*) &rtc);
		add_pending_send(PENDING_SEND_RTC);
		leaveCriticalSection(section);
	}
}

void vblank_handler()
{
	int previous_ime = enterCriticalSection();
	apply_pending_flip();
	apply_pending_swap();

	add_pending_send(PENDING_SEND_VBLANK);
	leaveCriticalSection(previous_ime);

	if (link_status == LINK_STATUS_ESTABLISHED) {
		fifoSendValue32(FIFO_USER_01, IPC_GET_INPUT);
		fifoSendValue32(FIFO_USER_01, IPC_GET_RTC);
	}
	vblank_count++;
}

int main()
{
	init_regs();
	powerOn(POWER_ALL_2D);

	fifoSendValue32(FIFO_USER_01, IPC_SET_BACKLIGHT | SET_BACKLIGHT_DATA(0x3));
	while (!(REG_DISPSTAT & DISP_IN_VBLANK));
	powerOff(POWER_SWAP_LCDS);

	video_init();
	audio_init();

	REG_AUXSPICNT = 0;

	irqSet(IRQ_CARD_LINE, card_line_handler);
	irqEnable(IRQ_CARD_LINE);

	/* MIPS-ARM9 SYNC AWAIT 1: We are waiting for the Supercard to assert the
	 * card line interrupt. The fact that we're waiting for this interrupt
	 * signals that our initialisation is done, and that our version of the
	 * link state is as follows:
	 *
	 * - The video mode is graphical
	 * - Main Screen buffers 0, 1 and 2, and Sub Screen buffer 0 are opaque
	 *   black
	 * - Among the Main Screen buffers, number 0 is displayed
	 * - Audio is stopped
	 * - Screens are not swapped (i.e. the Main Screen is on the bottom)
	 * - Both screen backlights are on
	 * - The Supercard does not have a reading for the buttons
	 * - The Supercard does not have a reading for the real-time clock
	 */
	while (link_status != LINK_STATUS_ESTABLISHED) {
		swiIntrWait(0, IRQ_ALL);
	}

	irqSet(IRQ_VBLANK, vblank_handler);
	irqEnable(IRQ_VBLANK);

	fifoSetValue32Handler(FIFO_USER_01, fifo_value_handler, NULL);
	fifoSetDatamsgHandler(FIFO_USER_01, fifo_datamsg_handler, NULL);

	fifoSendValue32(FIFO_USER_01, IPC_GET_INPUT);
	fifoSendValue32(FIFO_USER_01, IPC_GET_RTC);

	while (link_status == LINK_STATUS_ESTABLISHED) {
		REG_IME = IME_DISABLE;
		uint32_t pending_send = take_pending_send();

		switch (pending_send) {
		case PENDING_SEND_VBLANK:
			REG_IME = IME_ENABLE;
			card_send_command_byte(CARD_COMMAND_VBLANK_BYTE, 4);
			card_ignore_reply();
			break;

		case PENDING_SEND_VIDEO_DISPLAYED:
		{
			union card_command command;
			command.video_displayed.index = video_main_current;
			REG_IME = IME_ENABLE;
			command.video_displayed.byte = CARD_COMMAND_VIDEO_DISPLAYED_BYTE;
			memset(command.video_displayed.zero, 0, sizeof(command.video_displayed.zero));
			card_send_command(&command, 4);
			card_ignore_reply();
			break;
		}

		case PENDING_SEND_AUDIO_CONSUMED:
		{
			union card_command command;
			command.audio_consumed.count = audio_consumed;
			audio_consumed = 0;
			REG_IME = IME_ENABLE;
			command.audio_consumed.byte = CARD_COMMAND_AUDIO_CONSUMED_BYTE;
			memset(command.audio_consumed.zero, 0, sizeof(command.audio_consumed.zero));
			card_send_command(&command, 4);
			card_ignore_reply();
			break;
		}

		case PENDING_SEND_AUDIO_STATUS:
		{
			union card_command command;
			command.audio_status.status = audio_started;
			REG_IME = IME_ENABLE;
			command.audio_status.byte = CARD_COMMAND_AUDIO_STATUS_BYTE;
			memset(command.audio_status.zero, 0, sizeof(command.audio_status.zero));
			card_send_command(&command, 4);
			card_ignore_reply();
			break;
		}

		case PENDING_SEND_INPUT:
		{
			union card_command command;
			command.input.data = input;
			REG_IME = IME_ENABLE;
			command.input.byte = CARD_COMMAND_INPUT_BYTE;
			memset(command.input.zero, 0, sizeof(command.input.zero));
			card_send_command(&command, 4);
			card_ignore_reply();
			break;
		}

		case PENDING_SEND_RTC:
		{
			union card_command command;
			command.rtc.data = rtc;
			REG_IME = IME_ENABLE;
			command.rtc.byte = CARD_COMMAND_RTC_BYTE;
			card_send_command(&command, 4);
			card_ignore_reply();
			break;
		}

		case PENDING_SEND_QUEUE:
			process_send_queue();
			break;

		default:
			REG_IME = IME_ENABLE;
			swiIntrWait(0, IRQ_ALL);
			break;
		}
	}

	irqDisable(IRQ_VBLANK | IRQ_CARD_LINE);
	fifoSendValue32(FIFO_USER_01, IPC_SET_BACKLIGHT | SET_BACKLIGHT_DATA(0x3));
	audio_stop();

	irqDisable(IRQ_ALL);
	while (1) {
		swiIntrWait(1, 0);
	}
}
