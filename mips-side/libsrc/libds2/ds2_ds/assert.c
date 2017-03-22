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

#include <string.h>
#include <ds2/pm.h>

#include "../intc.h"
#include "card_protocol.h"
#include "globals.h"

__attribute__((noreturn, cold))
void _assert_fail(const char* file, unsigned int line, const char* text)
{
	size_t file_len = strlen(file), text_len = strlen(text);
	_ds2_ds.assert_failure.line = line;

	if (file_len > 255)
		file_len = 255;
	if (file_len + text_len > sizeof(_ds2_ds.assert_failure.data))
		text_len = sizeof(_ds2_ds.assert_failure.data) - file_len;

	_ds2_ds.assert_failure.file_len = file_len;
	_ds2_ds.assert_failure.text_len = text_len;

	memcpy(_ds2_ds.assert_failure.data, file, file_len);
	memcpy(&_ds2_ds.assert_failure.data[file_len], text, text_len);

	uint32_t section = DS2_EnterCriticalSection();
	_add_pending_send(PENDING_SEND_ASSERT);
	DS2_LeaveCriticalSection(section);

	while (1) {
		DS2_AwaitInterrupt();
	}
}

void _send_assert(void)
{
	_send_reply_4(DATA_KIND_MIPS_ASSERT | DATA_ENCODING(0) | DATA_BYTE_COUNT(sizeof(_ds2_ds.assert_failure)) | DATA_END);
	_send_reply(&_ds2_ds.assert_failure, sizeof(_ds2_ds.assert_failure));

	_ds2_ds.pending_sends = 0;
}
