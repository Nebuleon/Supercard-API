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

#include <ds2/pm.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "../intc.h"
#include "globals.h"
#include "text_encoding_0.h"

/* Text waiting to be sent to the Nintendo DS.
 * Aligned to 32 bytes so as to affect one fewer cache line than if it were
 * not. */
static char _text_data[508] __attribute__((aligned (32)));

/* The number of meaningful bytes at the start of _text_data.
 * volatile because it's modified by _text_dequeue as part of the card command
 * interrupt handler, and it's then tested in a loop to see if more text can
 * be submitted. */
static volatile size_t _text_size;

void _text_dequeue(void)
{
	_text_encoding_0(_text_data, _text_size);
	_text_size = 0;
}

void _text_enqueue(const char* text, size_t length)
{
	if (text == NULL)
		return;

	while (length > 0) {
		DS2_StartAwait();
		while (_text_size != 0)
			DS2_AwaitInterrupt();
		DS2_StopAwait();
		size_t entry_length = length >= 508 ? 508 : length;
		memcpy(_text_data, text, entry_length);
		_text_size = entry_length;
		text += entry_length;
		length -= entry_length;

		uint32_t section = DS2_EnterCriticalSection();
		_add_pending_send(PENDING_SEND_TEXT);
		DS2_LeaveCriticalSection(section);
	}
}
