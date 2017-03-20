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

size_t strcspn(const char* haystack, const char* antineedle)
{
	uint8_t antineedle_bitset[32];
	size_t ret = 0;
	memset(antineedle_bitset, 0, sizeof(antineedle_bitset));

	while (*antineedle) {
		uint8_t antineedle_char = *(uint8_t*) antineedle++;
		antineedle_bitset[antineedle_char / 8] |= 1 << (antineedle_char % 8);
	}

	while (*haystack) {
		uint8_t haystack_char = *(uint8_t*) haystack++;
		if (antineedle_bitset[haystack_char / 8] & (1 << (haystack_char % 8))) {
			break;
		}
		ret++;
	}

	return ret;
}
