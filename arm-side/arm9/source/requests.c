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

#include <nds.h>
#include <stdint.h>

#include "audio.h"
#include "card_protocol.h"
#include "common_ipc.h"
#include "requests.h"
#include "reset.h"
#include "video.h"

extern uint32_t arm9_reset_code[3];

void process_requests()
{
	struct card_reply_requests requests;

	REG_IME = IME_ENABLE;
	card_read_data(sizeof(requests), &requests, true);
	REG_IME = IME_DISABLE;

	if (requests.stop_audio || requests.reset) {
		audio_stop();
	}
	if (requests.start_audio) {
		audio_start(requests.audio_freq, requests.buffer_size,
			requests.is_16bit ? true : false,
			requests.is_stereo ? true : false);
	}
	if (requests.change_swap) {
		set_pending_swap(requests.swap_screens);
	}
	REG_IME = IME_ENABLE;
	if (requests.reset) {
		fifoSendValue32(FIFO_USER_01, IPC_START_RESET);
		/* Next step: ARM7 main.c: fifo_value_handler */
		reset_hardware();

		uint8_t* entrypoint = (uint8_t*) 0x027FFDF4;

		/* We will put ourselves in a loop after swiSoftReset.
		 * That loop, at 0x027FFDF8, will read the address at 0x027FFDFC.
		 * It is initially 0x027FFDF8. When the ARM7 processor is ready to
		 * send us somewhere, it will write a new address there. */
		memcpy(entrypoint, arm9_reset_code, sizeof(arm9_reset_code));
		*(volatile uint8_t**) 0x027FFE24 = entrypoint + 4;

		/* Make the ARM7 processor own all of these. Because of this line,
		 * the ARM9 processor will be halted if the ARM7 is also accessing
		 * RAM. */
		REG_EXMEMCNT = ARM7_MAIN_RAM_PRIORITY | ARM7_OWNS_ROM | ARM7_OWNS_CARD;

		IC_InvalidateAll();
		DC_FlushAll();
		DC_InvalidateAll();
		swiSoftReset();
	} else if (requests.change_backlight) {
		fifoSendValue32(FIFO_USER_01, IPC_SET_BACKLIGHT
			| SET_BACKLIGHT_DATA(requests.screen_backlights));
	}
}
