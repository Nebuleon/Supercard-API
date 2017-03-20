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

#include <stddef.h>
#include <stdint.h>
#include <string.h>

char* strpbrk(const char* haystack, const char* needle)
{
	uint8_t needle_bitset[32];
	memset(needle_bitset, 0, sizeof(needle_bitset));

	while (*needle) {
		uint8_t needle_char = *(uint8_t*) needle++;
		needle_bitset[needle_char / 8] |= 1 << (needle_char % 8);
	}

	while (*haystack) {
		uint8_t haystack_char = *(uint8_t*) haystack;
		if (needle_bitset[haystack_char / 8] & (1 << (haystack_char % 8))) {
			return (char*) haystack; /* specified to return non-cost char* */
		}
		haystack++;
	}

	return NULL;
}
