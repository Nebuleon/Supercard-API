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

#include "../libfat/source/fatdir.h"

struct dirent* readdir(DIR* dir)
{
	int result = _FAT_dirnext((DIR_ITER*) dir, ((DIR_ITER*) dir)->d->d_name, NULL);

	if (result == 0)
		return ((DIR_ITER*) dir)->d;
	else
		return NULL;
}
