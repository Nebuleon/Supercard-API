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

void _text_dequeue(void)
{
	_text_encoding_0(_ds2_ds.txt_data, _ds2_ds.txt_size);
	_ds2_ds.txt_size = 0;
}

void _text_enqueue(const char* text, size_t length)
{
	if (text == NULL)
		return;

	while (length > 0) {
		DS2_StartAwait();
		while (_ds2_ds.txt_size != 0)
			DS2_AwaitInterrupt();
		DS2_StopAwait();
		size_t entry_length = length >= 508 ? 508 : length;
		memcpy(_ds2_ds.txt_data, text, entry_length);
		_ds2_ds.txt_size = entry_length;
		text += entry_length;
		length -= entry_length;

		uint32_t section = DS2_EnterCriticalSection();
		_add_pending_send(PENDING_SEND_TEXT);
		DS2_LeaveCriticalSection(section);
	}
}
