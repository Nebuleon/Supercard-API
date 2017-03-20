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

#include <ctype.h>
#include <errno.h>
#include <limits.h>

unsigned long strtoul(const char* restrict ns, char** restrict end, int base)
{
	unsigned long result = 0, limit_10s, limit_1s;

	while (isspace(*ns))
		ns++;

	if ((base == 0 || base == 16)
	 && *ns == '0' && (*(ns + 1) == 'x' || *(ns + 1) == 'X')) {
		base = 16;
		ns += 2;
	} else if (base == 0 && *ns == '0') {
		base = 8;
		ns++;
	}

	limit_10s = ULONG_MAX / base;
	limit_1s = ULONG_MAX % base;

	while (*ns) {
		char c = *ns;
		unsigned int digit;

		if (c >= '0' && c <= '9') {
			digit = c - '0';
		} else if ((c | 32) >= 'a' && (c | 32) <= 'z') {
			digit = (c | 32) - 'a' + 10;
		} else {
			break;
		}

		if (digit >= base) {
			break;
		} else if (result > limit_10s || (result == limit_10s && digit >= limit_1s)) {
			errno = ERANGE;
			if (end)
				*end = (char*) ns; /* specified to write non-const char* */
			return ULONG_MAX;
		} else {
			result = result * base + digit;
		}

		ns++;
	}

	if (end)
		*end = (char*) ns; /* specified to write non-const char* */
	return result;
}
