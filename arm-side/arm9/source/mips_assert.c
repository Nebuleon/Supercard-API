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
#include <inttypes.h>
#include <stdio.h>

#include "audio.h"
#include "card_protocol.h"
#include "common_ipc.h"
#include "video.h"

void process_assert()
{
	struct card_reply_mips_assert failure;
	char* file;
	char* text;

	REG_IME = IME_ENABLE;
	card_read_data(sizeof(failure), &failure, true);
	REG_IME = IME_DISABLE;

	file = malloc(failure.file_len + 1);
	text = malloc(failure.text_len + 1);
	memcpy(file, failure.data, failure.file_len);
	file[failure.file_len] = '\0';
	memcpy(text, &failure.data[failure.file_len], failure.text_len);
	text[failure.text_len] = '\0';

	set_sub_text();
	consoleClear();
	iprintf("- Supercard assertion failure -\n\nFile: %s\nLine: %" PRIu32 "\n\n%s",
		file, failure.line, text);

	link_status = LINK_STATUS_ERROR;
}
