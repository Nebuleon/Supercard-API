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

#ifndef __DS2_DS_VIDEO_ENCODING_1_H__
#define __DS2_DS_VIDEO_ENCODING_1_H__

#include <ds2/ds.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Video encoding 1 sends video data as references to a palette sent for
 * the buffer.
 *
 * The palette is first sent using a packet of video encoding 1 with the
 * VIDEO_SET_PALETTE bit set. That palette contains up to 252 entries.
 *
 * The frame is then sent using packets of video encoding 1 with the
 * VIDEO_SET_PALETTE bit unset, and each byte refers to a palette entry.
 */

/*
 * Sends some pixels to the Nintendo DS as references to the palette in use
 * by the given buffer.
 *
 * The Main Engine is implicitly used.
 *
 * In:
 *   src: A pointer to the first pixel to be sent. This is guaranteed to be
 *     aligned to 4 bytes.
 *   buffer: The buffer number to which the pixels are destined. Sent in the
 *     header.
 *   pixel_offset: The pixel offset within the screen (or screen buffer) to
 *     which the first pixel is destined. Sent in the header.
 *   pixel_count: The number of valid pixels at and after *src. Some of these
 *     pixels are used for the reply, and the number is sent in the header.
 *     This is guaranteed to be a multiple of 2.
 * Returns:
 *   The number of pixels sent in the reply.
 */
extern size_t _video_encoding_1(const uint16_t* src, uint_fast8_t buffer, uint_fast16_t pixel_offset, size_t pixel_count);

/*
 * Sends the given palette to the Nintendo DS for the next frame.
 *
 * The Main Engine is implicitly used.
 *
 * In:
 *   buffer: The buffer to set the palette for. Sent in the header.
 */
extern void _send_palette(uint_fast8_t buffer);

#endif /* !__DS2_DS_VIDEO_ENCODING_1_H__ */
