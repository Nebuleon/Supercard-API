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

#ifndef __DIRENT_H__
#define __DIRENT_H__

/* Also defined in sys/types.h. */
#ifndef _INO_T_
#  define _INO_T_
typedef unsigned int ino_t;
#endif

struct dirent {
	char d_name[1];
};

struct _DIR_ITER;

typedef struct _DIR_ITER DIR;

extern int closedir(DIR*);

extern DIR* opendir(const char* path);

extern struct dirent* readdir(DIR*);

extern void rewinddir(DIR*);

#endif /* !__DIRENT_H__ */
