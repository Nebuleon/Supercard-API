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

#ifndef __SYS_STAT_H__
#define __SYS_STAT_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <time.h>
#include <sys/types.h>

typedef unsigned int mode_t;

struct stat {
	dev_t st_dev;         /* device identifier */
	ino_t st_ino;         /* inode number */
	mode_t st_mode;       /* protection */
	nlink_t st_nlink;     /* number of hard links to the file */
	uid_t st_uid;         /* owner user ID */
	gid_t st_gid;         /* owner group ID */
	dev_t st_rdev;        /* device identifier for device files */
	off_t st_size;        /* total size, in bytes */
	time_t st_atime;      /* time of last access */
	time_t st_mtime;      /* time of last write */
	time_t st_ctime;      /* time of creation */
	blksize_t st_blksize; /* size of blocks on the filesystem */
	blkcnt_t st_blocks;   /* number of blocks allocated for the file */
};

#define S_IFDIR     1
#define S_IFREG     2
#define S_IRUSR     4
#define S_IRGRP     8
#define S_IROTH    16
#define S_IWUSR    32
#define S_IWGRP    64
#define S_IWOTH   128
#define S_IXUSR   256
#define S_IXGRP   512
#define S_IXOTH  1024
#define S_IRWXU  (S_IRUSR | S_IWUSR | S_IXUSR)
#define S_IRWXG  (S_IRGRP | S_IWGRP | S_IXGRP)
#define S_IRWXO  (S_IROTH | S_IWOTH | S_IXOTH)

#define S_ISREG(m) ((m) & S_IFREG)
#define S_ISDIR(m) ((m) & S_IFDIR)

extern int fstat(int fd, struct stat *buf);

extern int mkdir(const char* path, mode_t mode);

extern int stat(const char* restrict path, struct stat* restrict buf);

#define lstat stat /* FAT does not support symbolic links */

#ifdef __cplusplus
}
#endif

#endif /* !__SYS_STAT_H__ */
