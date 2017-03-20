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

/* All functions that read from the file redirect to this function.
 * If the core function changes, please change them in all functions
 * that call this one. */
ssize_t read(int fd, void* dst, size_t n)
{
	/* TODO Grab information from somewhere (only stdin) */
	if (fd == fileno(stdout) || fd == fileno(stderr) || fd == fileno(stdin))
		return 0;

	return _FAT_read(fd, dst, n);
}
