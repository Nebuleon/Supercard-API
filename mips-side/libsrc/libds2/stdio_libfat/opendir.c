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

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "../libfat/source/fatdir.h"

DIR* opendir(const char* path)
{
	DIR_ITER* iter = malloc(sizeof(DIR_ITER));

	if (iter == NULL) {
		errno = EMFILE; /* Too many open files (exhausted memory) */
		goto iter_error;
	}

	iter->dirStruct = malloc(sizeof(DIR_STATE_STRUCT));
	if (iter->dirStruct == NULL) {
		errno = EMFILE; /* Too many open files (exhausted memory) */
		goto state_struct_error;
	}

	iter->d = malloc(sizeof(struct dirent) + FILENAME_MAX);
	if (iter->d == NULL) {
		errno = EMFILE; /* Too many open files (exhausted memory) */
		goto dirent_error;
	}

	if (_FAT_diropen(iter, path) == NULL) {
		goto open_error;
	}

	return iter;

open_error:
	free(iter->dirStruct);
dirent_error:
	free(iter->d);
state_struct_error:
	free(iter);
iter_error:
	return NULL;
}
