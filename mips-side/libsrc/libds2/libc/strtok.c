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

static char* saved_string;

char* strtok(char* restrict haystack, const char* restrict needle)
{
	char* ret = NULL;

	uint8_t needle_bitset[32];
	memset(needle_bitset, 0, sizeof(needle_bitset));

	while (*needle) {
		uint8_t needle_char = *(uint8_t*) needle++;
		needle_bitset[needle_char / 8] |= 1 << (needle_char % 8);
	}

	if (!haystack)
		haystack = saved_string;

	/* First search for a non-needle character. */
	while (*haystack) {
		uint8_t haystack_char = *(uint8_t*) haystack;
		if (!(needle_bitset[haystack_char / 8] & (1 << (haystack_char % 8)))) {
			ret = haystack; /* beginning of token */
			break;
		}
		haystack++;
	}

	/* (If there was no non-needle character, 'ret' is still NULL and the
	 * loop below will not be entered, so the start of the token is NULL
	 * and the end of the string will go into saved_string.) */

	/* Then search for a needle character and replace it with a zero byte. */
	while (*haystack) {
		uint8_t haystack_char = *(uint8_t*) haystack;
		if (needle_bitset[haystack_char / 8] & (1 << (haystack_char % 8))) {
			*haystack = '\0';
			saved_string = haystack + 1;
			return ret;
		}
		haystack++;
	}

	/* If no such byte is found, the current token extends to the end of the
	 * haystack string (and its existing zero byte), and NULL shall be
	 * returned on subsequent calls. */
	saved_string = haystack;
	return ret;
}
