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

#ifndef __SYS_STATVFS_H__
#define __SYS_STATVFS_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <sys/types.h>

struct statvfs {
	unsigned long f_bsize;    /* filesystem block size */
	unsigned long f_frsize;   /* fundamental filesystem block size */
	fsblkcnt_t f_blocks;      /* number of blocks on filesystem in f_frsize units */
	fsblkcnt_t f_bfree;       /* number of free blocks */
	fsblkcnt_t f_bavail;      /* number of blocks available to non-privileged processes */
	fsfilcnt_t f_files;       /* number of file serial numbers */
	fsfilcnt_t f_ffree;       /* number of free file serial numbers */
	fsfilcnt_t f_favail;      /* number of file serial numbers available to non-privileged processes */
	unsigned long f_fsid;     /* filesystem identifier */
	unsigned long f_flag;
	unsigned long f_namemax;  /* maximum file name length */
};

#define ST_RDONLY 1
#define ST_NOSUID 2

extern int statvfs(const char* restrict path, struct statvfs* restrict dst);

#ifdef __cplusplus
}
#endif

#endif
