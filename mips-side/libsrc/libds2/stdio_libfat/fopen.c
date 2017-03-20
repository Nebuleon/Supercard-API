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

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../libfat/source/fatfile.h"

struct _symbolic_mode_mapping {
	const char* symbolic;
	int mode;
};

static const struct _symbolic_mode_mapping _symbolic_mode_mappings[] = {
	{ "r",   O_RDONLY },
	{ "w",   O_WRONLY | O_TRUNC | O_CREAT },
	{ "a",   O_WRONLY | O_APPEND | O_CREAT },
	{ "rb",  O_RDONLY },
	{ "wb",  O_WRONLY | O_TRUNC | O_CREAT },
	{ "ab",  O_WRONLY | O_APPEND | O_CREAT },
	{ "r+",  O_RDWR },
	{ "w+",  O_RDWR | O_TRUNC | O_CREAT },
	{ "a+",  O_RDWR | O_APPEND | O_CREAT },
	{ "r+b", O_RDWR },
	{ "rb+", O_RDWR },
	{ "w+b", O_RDWR | O_TRUNC | O_CREAT },
	{ "wb+", O_RDWR | O_TRUNC | O_CREAT },
	{ "a+b", O_RDWR | O_APPEND | O_CREAT },
	{ "ab+", O_RDWR | O_APPEND | O_CREAT }
};

#define SYMBOLIC_MODE_MAPPING_COUNT (sizeof(_symbolic_mode_mappings) / sizeof(_symbolic_mode_mappings[0]))

FILE* fopen(const char* restrict file, const char* restrict symbolic_mode)
{
	FILE_STRUCT* handle = malloc(sizeof(FILE_STRUCT));
	size_t i;
	int mode;

	if (handle == NULL) {
		errno = EMFILE; /* Too many open files (exhausted memory) */
		return NULL;
	}

	for (i = 0; i < SYMBOLIC_MODE_MAPPING_COUNT; i++) {
		if (strcmp(symbolic_mode, _symbolic_mode_mappings[i].symbolic) == 0) {
			break;
		}
	}

	if (i >= SYMBOLIC_MODE_MAPPING_COUNT) {
		errno = EINVAL;
		goto error;
	}

	mode = _symbolic_mode_mappings[i].mode;
	if (_FAT_open(handle, file, mode, 0) == -1) {
		goto error;
	}

	return (FILE*) handle;

error:
	free(handle);
	return NULL;
}
