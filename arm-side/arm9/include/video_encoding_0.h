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

#ifndef VIDEO_ENCODING_0_H
#define VIDEO_ENCODING_0_H

#include <stdint.h>

/*
 * Video encoding 0 is simply uncompressed video, sent by the Supercard as
 * RGB 555 with the upper bit set on each 16-bit quantity by the FPGA.
 *
 * In:
 *   header_1: The first header word, containing the meaningful byte count.
 *   dest: Pointer to the destination of the video update request, computed
 *     from the second header word.
 *   max_pixels: Number of valid pixels at and after 'dest'.
 */
void video_encoding_0(uint32_t header_1, uint16_t* dest, size_t max_pixels);

#endif /* !VIDEO_ENCODING_0_H */
