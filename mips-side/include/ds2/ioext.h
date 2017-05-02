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

#ifndef __DS2_IOEXT_H__
#define __DS2_IOEXT_H__

#include <dirent.h>
#include <sys/stat.h>

/*
 * Returns a pointer to the current directory entry information structure for
 * the given 'DIR *' iteration descriptor, and, if the pointer is not NULL,
 * fills the given 'struct stat *' with information about the same file (or
 * directory) that is returned.
 *
 * Once that is done, the function advances the directory iterator in a manner
 * identical to readdir.
 *
 * In:
 *   argument 1, DIR *: Directory iterator returned by opendir and not yet
 *     closed by closedir.
 * Out:
 *   argument 2, struct stat *: Structure to be filled with information about
 *     the current file.
 * Returns:
 *   NULL, errno changed: Failure.
 *   NULL, errno unchanged: A previous call to readdir or readdir_stat found
 *     the end of the directory.
 *   non-NULL: A pointer to a transient structure that describes the current
 *     directory entry, which is defined only until the next call to readdir,
 *     readdir_stat or closedir.
 * Portability notes:
 *   This is an extension introduced by libfat and the Supercard DSTwo's C
 *   standard library for efficiency. Code should use readdir_stat only in
 *   [#ifdef SCDS2]/[#endif] directives, and use readdir and stat separately
 *   otherwise.
 */
extern struct dirent* readdir_stat(DIR*, struct stat*);

#endif /* !__DS2_IOEXT_H__ */
