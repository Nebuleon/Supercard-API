/*
 * This file is part of the MIPS packer for the Supercard DSTwo.
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

#ifndef FILTER_H
#define FILTER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/*
 * Filters the specified buffer, containing 'n' 32-bit words, into streams.
 * Returns the number of words that were filtered. The remainder is data.
 */
extern uint32_t make_streams(const uint8_t* buf, uint32_t n);

/*
 * Gets the size of the streams made by the last invocation of make_streams in
 * bytes. This includes the header, indicating the offsets of each stream, but
 * not any remaining data.
 */
extern uint32_t get_filtered_size(void);

/*
 * Writes the streams made by the last invocation of make_streams to the given
 * file, with a header indicating the offsets of each stream, and aligning all
 * streams to 4 bytes.
 *
 * Returns:
 *   true if all writes succeeded.
 *   false if a write failed.
 */
extern bool write_streams(FILE* outfile);

/* Releases resources currently being used by streams. */
extern void free_streams(void);

#endif /* !FILTER_H */
