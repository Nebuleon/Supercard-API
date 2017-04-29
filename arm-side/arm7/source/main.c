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
#include <maxmod7.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "common_ipc.h"
#include "reset.h"

/* The latest reading of the real-time clock. In a global variable to avoid
 * stack usage in fifo_value_handler. */
static uint8_t rtc_data[7];

/* The latest reading of the touchscreen. In a global variable to avoid stack
 * usage in fifo_value_handler. */
static touchPosition touch;

static uint8_t prev_backlights, new_backlights;

extern uint8_t arm7_loader;
extern uint8_t arm7_loader_end;

void fifo_value_handler(uint32_t value, void* userdata)
{
	uint16_t command = value & 0xFFFF;

	switch (command) {
		case IPC_GET_INPUT:
		{
			uint16_t ext_buttons = REG_KEYXY;
			uint16_t reply = ((ext_buttons & 3) ^ 3) << 10; /* X bit 0 -> 10, Y bit 1 -> 11 */
			reply |= (ext_buttons & 0x80) << 6; /* Lid bit 7 -> 13 */
			if (touchPenDown()) {
				reply |= 0x1000;
			}
			fifoSendValue32(FIFO_USER_01, IPC_RPL_INPUT_BUTTONS | RPL_INPUT_BUTTONS(reply));
			if (reply & 0x1000) {
				touchReadXY(&touch);
				fifoSendValue32(FIFO_USER_01, IPC_RPL_INPUT_TOUCH
					| RPL_INPUT_TOUCH_X(touch.px & 0xFF)
					| RPL_INPUT_TOUCH_Y(touch.py & 0xFF));
			}
			break;
		}

		case IPC_GET_RTC:
			rtcGetTimeAndDate(rtc_data);
			fifoSendDatamsg(FIFO_USER_01, 7, rtc_data);
			break;

		case IPC_SET_BACKLIGHT:
			new_backlights = (value & SET_BACKLIGHT_DATA_MASK) >> SET_BACKLIGHT_DATA_BIT;
			break;

		case IPC_START_RESET:
		{
			reset_hardware();

			/* Copy the loader code just before the ARM9's reset code, which
			 * is at 0x027FFDF4 in EWRAM, with stacks starting at 0x0380FF00
			 * in IWRAM and going down. Addresses are high to avoid the code
			 * and stack getting overwritten by most programs.
			 *
			 * There are 4 cases here:
			 *
			 * a) The new program is somewhere in EWRAM, ending at or before
			 *    0x027FFDF4 - loader size;
			 * b) The new program is somewhere in EWRAM, ending after
			 *    0x027FFDF4 - loader size;
			 * c) The new program is somewhere in IWRAM, ending at or before
			 *    0x0380FF00 - loader stack usage;
			 * d) The new program is somewhere in IWRAM, ending after
			 *    0x0380FF00 - loader stack usage.
			 *
			 * 'a' and 'c' are perfectly safe, but in cases 'b' amd 'd', the
			 * loader will have to write to an address it occupies and will
			 * execute any code that is found at that address, before it has
			 * had a chance to set its entrypoint! This is why we load into
			 * the highest possible address.
			 *
			 * NOTE: The start of the loader MUST be ARM code in order for the
			 * lowest bit of its starting address to be unset. Fortunately, we
			 * must set up the stacks, which entails using the MSR instruction
			 * which is only available in ARM mode.
			 */
			ptrdiff_t loader_size = &arm7_loader_end - &arm7_loader;
			loader_size = (loader_size + 3) & ~3;

			uint8_t* entrypoint = (uint8_t*) 0x027FFDF4 - loader_size;
			memcpy(entrypoint, &arm7_loader, loader_size);

			*(volatile uint8_t**) 0x027FFE34 = entrypoint;

			/* Wait for the ARM9 to relinquish these. */
			while ((REG_EXMEMSTAT & (ARM7_OWNS_CARD | ARM7_OWNS_ROM))
			    != (ARM7_OWNS_CARD | ARM7_OWNS_ROM));

			swiSoftReset();
			break;
		}
	}
}

/*
 * This handler sets the state of the Nintendo DS's backlights only once per
 * frame, if required. In addition to avoiding tearing on both screens, this
 * prevents changing the status of the backlights too often which, according
 * to GBATEK, could damage hardware.
 */
void vblank_handler(void)
{
	if (new_backlights != prev_backlights) {
		int section = enterCriticalSection();
		writePowerManagement(PM_CONTROL_REG,
			(readPowerManagement(PM_CONTROL_REG) & ~0xC) | ((new_backlights & 0x3) << 2));
		prev_backlights = new_backlights;
		leaveCriticalSection(section);
	}
}

int main()
{
	/* Clear sound registers. */
	dmaFillWords(0, (void*) 0x04000400, 0x100);

	REG_SOUNDCNT = SOUND_ENABLE | SOUND_VOL(0x7F);
	writePowerManagement(PM_CONTROL_REG,
		(readPowerManagement(PM_CONTROL_REG) & ~PM_SOUND_MUTE) | PM_SOUND_AMP);
	powerOn(POWER_SOUND);

	/* Change the sound bias slowly from the current value to normal (0x200)
	 * with 0x400 unit delays. */
	swiChangeSoundBias(1, 0x400);

	readUserSettings(); /* required for touch calibration data */
	ledBlink(0);

	irqInit();
	/* Start the RTC tracking IRQ. */
	initClockIRQ();
	/* IRQ_NETWORK also encompasses the RTC interrupt. Enable it quickly. */
	irqEnable(IRQ_NETWORK);

	fifoInit();
	mmInstall(FIFO_MAXMOD);
	installSystemFIFO();
	fifoSetValue32Handler(FIFO_USER_01, fifo_value_handler, NULL);

	irqSet(IRQ_VBLANK, vblank_handler);
	irqEnable(IRQ_VBLANK);

	/* The ARM7 will be mostly idle, only waking up for interrupts. */
	while (1) {
		swiIntrWait(1, IRQ_ALL);
	}
	return 0;
}
