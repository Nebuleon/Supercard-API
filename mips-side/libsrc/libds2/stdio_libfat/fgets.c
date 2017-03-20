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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "../libfat/source/fatfile.h"

char* fgets(char* restrict dst, int n, FILE* restrict handle)
{
	ssize_t count;
	char* nl_ptr;

	dst[0] = '\0';
	if (n <= 1) return dst;

	count = read(fileno(handle), dst, n - 1);
	if (count <= 0) {
		dst[0] = '\0';
		return NULL;
	}
	else dst[count] = '\0';

	nl_ptr = memchr(dst, 0x0A, count);

	if (nl_ptr) {
		ptrdiff_t nl_index = nl_ptr - dst;

		/* A newline was found! End the string there. */
		*(nl_ptr + 1) = '\0';
		/* Fix up the excess by unreading it. */
		lseek(fileno(handle), -(count - (nl_index + 1)), SEEK_CUR);
	} else if (count == n - 1 && dst[n - 2] == 0x0D) {
		/* Part of a newline is being read. Unread the first part. */
		dst[n - 2] = '\0';
		lseek(fileno(handle), -1, SEEK_CUR);
	}

	return dst;
}
