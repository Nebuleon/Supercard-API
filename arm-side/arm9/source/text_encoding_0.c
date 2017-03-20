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
#include <stdio.h>
#include <inttypes.h>

#include "card_protocol.h"
#include "text_encoding_0.h"

void text_encoding_0(uint32_t header_1)
{
	size_t bytes = (header_1 & DATA_BYTE_COUNT_MASK) >> DATA_BYTE_COUNT_BIT;
	char text[508];
	if (bytes > 508) {
		fatal_link_error("Text encoding 0 data is larger\nthan 508 bytes\n\n%zu extra uncompressed bytes", bytes - 508);
	}

	/* We cannot use DMA here, because we have already started reading some of
	 * the reply. To work properly, DMA must be started before the card bus is
	 * accessed. Otherwise, the DMA hardware MAY ignore a word that was queued
	 * already (REG_ROMCTRL & CARD_DATA_READY), and read only the next one! */

	card_read_data((bytes + 3) & ~3, text, false);
	card_ignore_reply();
	fwrite(text, 1, bytes, stdout);
}
