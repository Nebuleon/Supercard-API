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

#ifndef __UNISTD_H__
#define __UNISTD_H__

#include <sys/types.h>

#define F_OK 1
#define R_OK 2
#define W_OK 4
#define X_OK 8

/* Also defined in stdio.h. */
#if !defined SEEK_SET
#  define SEEK_SET 1
#endif
#if !defined SEEK_CUR
#  define SEEK_CUR 2
#endif
#if !defined SEEK_END
#  define SEEK_END 3
#endif

extern int access(const char* path, int mode);

extern int chdir(const char* path);

extern int close(int fd);

extern int fdatasync(int fd);

extern int fsync(int fd);

extern int ftruncate(int fd, off_t new_length);

extern off_t lseek(int fd, off_t offset, int whence);

extern ssize_t read(int fd, void* dst, size_t n);

extern int rmdir(const char* path);

extern int unlink(const char* file);

/*
 * Causes the CPU to delay the given number of microseconds. (= 0.000001 s)
 *
 * The CPU may delay for a bit longer if interrupts need to be handled.
 *
 * Input:
 *   usec: The number of microseconds to wait.
 */
extern void usleep(unsigned int usec);

extern ssize_t write(int fd, const void* src, size_t n);

#endif /* !__UNISTD_H__ */

