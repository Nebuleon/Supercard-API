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

#ifndef __DS2_DS_VIDEO_ENCODING_0_H__
#define __DS2_DS_VIDEO_ENCODING_0_H__

#include <ds2/ds.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Video encoding 0 is simply uncompressed video sent to the Nintendo DS as
 * RGB 555 with the upper bit set on each 16-bit quantity.
 *
 * In:
 *   src: A pointer to the first pixel to be sent. This is guaranteed to be
 *     aligned to 4 bytes.
 *   engine: The Nintendo DS engine to which the pixels are destined.
 *     This is used to get the proper pixel format (BGR 555 or RGB 555) and
 *     sent in the header.
 *   buffer: If engine == DS_ENGINE_MAIN, the buffer number to which the
 *     pixels are destined. Sent in the header.
 *   pixel_offset: The pixel offset within the screen (or screen buffer) to
 *     which the first pixel is destined. Sent in the header.
 *   pixel_count: The number of valid pixels at and after *src. Some of these
 *     pixels are used for the reply, and the number is sent in the header.
 *     This is guaranteed to be a multiple of 2.
 * Returns:
 *   The number of pixels sent in the reply.
 */
extern size_t _video_encoding_0(const uint16_t* src, enum DS_Engine engine, uint_fast8_t buffer, uint_fast16_t pixel_offset, size_t pixel_count);

#endif /* !__DS2_DS_VIDEO_ENCODING_0_H__ */
