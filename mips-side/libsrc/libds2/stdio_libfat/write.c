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
#include <unistd.h>
#include <sys/types.h>

#include "../libfat/source/fatfile.h"
#include "../ds2_ds/text.h"

/* All functions that write to the file redirect to this function.
 * If the core function changes, please change them in all functions
 * that call this one. */
ssize_t write(int fd, const void* src, size_t n)
{
	if (fd == fileno(stdin))
		return 0;
	else if (fd == fileno(stdout) || fd == fileno(stderr)) {
		_text_enqueue(src, n);
		return n;
	}

	return _FAT_write(fd, src, n);
}
