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

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "card_protocol.h"
#include "globals.h"
#include "video.h"

size_t _video_encoding_1(const uint16_t* src, uint_fast8_t buffer, uint_fast16_t pixel_offset, size_t pixel_count)
{
	bool end = pixel_count <= 504;
	const uint8_t* rev_palette = _video_main_rev_palettes[buffer];
	size_t i;
	if (!end)
		pixel_count = 504;

	_ds2_ds.vid_header_1 = DATA_KIND_VIDEO | DATA_ENCODING(1)
	                     | DATA_BYTE_COUNT(pixel_count);
	_ds2_ds.vid_header_2 = VIDEO_BUFFER(buffer) | VIDEO_PIXEL_OFFSET(pixel_offset)
	                     | VIDEO_ENGINE_MAIN
	                     | (end ? VIDEO_END_FRAME : 0);

	for (i = 0; i < pixel_count; i += 4) {
		_ds2_ds.vid_next_data.words[i / 4] = rev_palette[src[i] & UINT16_C(0x7FFF)]
			| (rev_palette[src[i + 1] & UINT16_C(0x7FFF)] << 8)
			| (rev_palette[src[i + 2] & UINT16_C(0x7FFF)] << 16)
			| (rev_palette[src[i + 3] & UINT16_C(0x7FFF)] << 24);
	}

	_ds2_ds.vid_next_ptr = &_ds2_ds.vid_next_data;
	_ds2_ds.vid_fixup = false;

	return pixel_count;
}

void _send_palette(uint_fast8_t buffer)
{
	_ds2_ds.vid_header_1 = DATA_KIND_VIDEO | DATA_ENCODING(1);
	_ds2_ds.vid_header_2 = VIDEO_SET_PALETTE | VIDEO_BUFFER(buffer)
	                     | VIDEO_ENGINE_MAIN;
	_ds2_ds.vid_next_ptr = _video_main_palettes[buffer];
	/* Up until now, the palette has been in the native framebuffer format for
	 * the Main Engine. Fixing up the palette to BGR 555 with the high bit set
	 * will allow the Nintendo DS to get the right colors. */
	_ds2_ds.vid_fixup = true;
}
