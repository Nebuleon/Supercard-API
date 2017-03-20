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

#include <stdbool.h>
#include <ctype.h>

int atoi(const char* s)
{
	unsigned int result = 0;
	bool neg = false;
	while (isspace(*s))
		s++;
	if (*s == '+' || *s == '-') {
		neg = *s == '-';
		s++;
	}
	while (*s && (*s >= '0' && *s <= '9')) {
		result = result * 10 + (*s - '0');
		s++;
	}
	return neg ? -result : result;
}
