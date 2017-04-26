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

size_t _video_encoding_0(const uint16_t* src, enum DS_Engine engine, uint_fast8_t buffer, uint_fast16_t pixel_offset, size_t pixel_count)
{
	bool end = pixel_count <= 252;
	if (!end)
		pixel_count = 252;

	_ds2_ds.vid_header_1 = DATA_KIND_VIDEO | DATA_ENCODING(0)
	                     | DATA_BYTE_COUNT(pixel_count * sizeof(uint16_t));
	_ds2_ds.vid_header_2 = VIDEO_BUFFER(buffer) | VIDEO_PIXEL_OFFSET(pixel_offset)
	                  | (engine == DS_ENGINE_MAIN ? VIDEO_ENGINE_MAIN : VIDEO_ENGINE_SUB)
	                  | (end ? VIDEO_END_FRAME : 0);

	if (!end) {
		_ds2_ds.vid_next_ptr = src;
	} else {
		/* There may not be 504 valid bytes after the final pixels of the
		 * screen; the ones that follow may be past the end of RAM, so we
		 * move them to vid_next_data. */
		memcpy(&_ds2_ds.vid_next_data, src, pixel_count * sizeof(uint16_t));
		_ds2_ds.vid_next_ptr = &_ds2_ds.vid_next_data;
	}
	_ds2_ds.vid_fixup = true;

	return pixel_count;
}
