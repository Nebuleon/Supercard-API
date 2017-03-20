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

#include <strings.h>

int strncasecmp(const char* s1, const char* s2, size_t n)
{
	while (n--) {
		unsigned char c1 = (unsigned char) *s1++;
		unsigned char c2 = (unsigned char) *s2++;
		if ((c1 | 32) >= 'a' && (c1 | 32) <= 'z')
			c1 |= 32;
		if ((c2 | 32) >= 'a' && (c2 | 32) <= 'z')
			c2 |= 32;
		if (c1 != c2)
			return (int) c1 - (int) c2;
	}
	return 0;
}
