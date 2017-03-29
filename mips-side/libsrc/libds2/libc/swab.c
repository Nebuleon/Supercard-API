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

#include <stdint.h>
#include <unistd.h>

void swab(const void *from, void *to, ssize_t n)
{
	const uint8_t* from_8 = from;
	uint8_t* to_8 = to;

	while (n >= 2) {
		to_8[0] = from_8[1];
		to_8[1] = from_8[0];
		from_8 += 2;
		to_8 += 2;
		n -= 2;
	}

	if (n == 1)
		*to_8 = *from_8;
}
