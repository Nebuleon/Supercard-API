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

#ifndef VIDEO_ENCODING_1_H
#define VIDEO_ENCODING_1_H

#include <stdint.h>

/*
 * Video encoding 1 is video data sent by the Supercard as references to
 * a palette sent for the buffer.
 *
 * The palette is first sent using a packet of video encoding 1 with the
 * VIDEO_SET_PALETTE bit set. That palette contains up to 252 entries.
 *
 * The frame is then sent using packets of video encoding 1 with the
 * VIDEO_SET_PALETTE bit unset, and each byte refers to a palette entry.
 */

/*
 * Receives and processes a packet using video encoding 1, writing part of the
 * screen to VRAM using references to the palette last sent by set_palette for
 * the buffer.
 *
 * The Main Engine is implicitly used.
 *
 * In:
 *   header_1: The first header word, containing the meaningful byte count.
 *   dest: Pointer to the destination of the video update request, computed
 *     from the second header word.
 *   max_pixels: Number of valid pixels at and after 'dest'.
 */
void video_encoding_1(uint32_t header_1, uint16_t* dest, size_t max_pixels);

/*
 * Receives and processes a packet using video encoding 1, setting the palette
 * to be used by the given buffer.
 *
 * The Main Engine is implicitly used.
 *
 * In:
 *   buffer: The buffer to set the palette for.
 */
void set_palette(uint8_t buffer);

#endif /* !VIDEO_ENCODING_1_H */
