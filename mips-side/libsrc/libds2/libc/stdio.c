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
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>

#include "ds2_ds/text.h"

#include "libfat/source/fatdir.h"
#include "libfat/source/fatfile.h"
#include "libfat/source/partition.h"

#include "libfat/include/fat.h"

static FILE_STRUCT _std_files[3] __attribute__((section(".noinit")));

FILE* const stdin = (FILE*) &_std_files[0];
FILE* const stdout = (FILE*) &_std_files[1];
FILE* const stderr = (FILE*) &_std_files[2];

#define IS_STDIN(handle) ((uintptr_t) (handle) == (uintptr_t) &_std_files[0])
#define IS_STDOUT(handle) ((uintptr_t) (handle) == (uintptr_t) &_std_files[1])
#define IS_STDERR(handle) ((uintptr_t) (handle) == (uintptr_t) &_std_files[2])

bool DS2_InitFS(void)
{
	return fatInitDefault();
}

/* All functions that close a file redirect to this function.
 * If the core function changes, please change them in all functions
 * that call this one. */
int close(int fd)
{
	if (IS_STDOUT(fd) || IS_STDERR(fd) || IS_STDIN(fd))
		return -1;
	else {
		int result = _FAT_close(fd);

		free((FILE_STRUCT*) fd);
		return result;
	}
}

/* All functions that get the file pointer redirect to this function.
 * If the core function changes, please change them in all functions
 * that call this one. */
long ftell(FILE* handle)
{
	if (IS_STDIN(handle) || IS_STDOUT(handle) || IS_STDERR(handle))
		return 0;

	return ((FILE_STRUCT*) handle)->currentPosition;
}

/* All functions that read from the file redirect to this function.
 * If the core function changes, please change them in all functions
 * that call this one. */
ssize_t read(int fd, void* dst, size_t n)
{
	/* TODO Grab information from somewhere (only stdin) */
	if (IS_STDOUT(fd) || IS_STDERR(fd) || IS_STDIN(fd))
		return 0;

	return _FAT_read(fd, dst, n);
}

/* All functions that write to the file redirect to this function.
 * If the core function changes, please change them in all functions
 * that call this one. */
ssize_t write(int fd, const void* src, size_t n)
{
	if (IS_STDIN(fd))
		return 0;
	else if (IS_STDOUT(fd) || IS_STDERR(fd)) {
		_text_enqueue(src, n);
		return n;
	}

	return _FAT_write(fd, src, n);
}

int access(const char* path, int mode)
{
	/* TODO Implement actual checks here */
	return 0;
}

int chdir(const char* path)
{
	return _FAT_chdir(path) ? 0 : -1;
}

void clearerr(FILE* handle)
{
	if (!IS_STDIN(handle) && !IS_STDOUT(handle) && !IS_STDERR(handle)) {
		((FILE_STRUCT*) handle)->error = false;
	}
}

int closedir(DIR* dir)
{
	int result = _FAT_dirclose((DIR_ITER*) dir);

	free(dir);
	return result;
}

int fclose(FILE* handle)
{
	return close(fileno(handle));
}

int fdatasync(int fd)
{
	if (IS_STDIN(fd) || IS_STDOUT(fd) || IS_STDERR(fd))
		return 0;

	return _FAT_fsync(fd);
}

FILE* fdopen(int fd, const char* mode)
{
	return (FILE*) fd;
}

int feof(FILE* handle)
{
	return ((FILE_STRUCT*) handle)->currentPosition >= ((FILE_STRUCT*) handle)->filesize
	     ? -1
	     : 0;
}

int ferror(FILE* handle)
{
	return ((FILE_STRUCT*) handle)->error ? -1 : 0;
}

int fflush(FILE* handle)
{
	if (IS_STDIN(handle) || IS_STDOUT(handle) || IS_STDERR(handle))
		return 0;
	else if (handle == NULL) {  /* flush all open files */
		if (_FAT_cache_flush(single_partition->cache))
			return 0;
		else {
			errno = EIO;
			return EOF;
		}
	} else
		return _FAT_fsync(fileno(handle));
}

int fgetc(FILE* handle)
{
	char ch;
	ssize_t result = read(fileno(handle), &ch, 1);

	if (result <= 0)
		return EOF;
	else
		return (int) (unsigned char) ch;
}

int fgetpos(FILE* restrict handle, fpos_t* restrict pos)
{
	*pos = ftell(handle);
	return 0;
}

char* fgets(char* restrict dst, int n, FILE* restrict handle)
{
	ssize_t count;
	char* nl_ptr;

	dst[0] = '\0';
	if (n <= 1) return dst;

	count = read(fileno(handle), dst, n - 1);
	if (count <= 0) {
		dst[0] = '\0';
		return NULL;
	}
	else dst[count] = '\0';

	nl_ptr = memchr(dst, 0x0A, count);

	if (nl_ptr) {
		ptrdiff_t nl_index = nl_ptr - dst;

		/* A newline was found! End the string there. */
		*(nl_ptr + 1) = '\0';
		/* Fix up the excess by unreading it. */
		lseek(fileno(handle), -(count - (nl_index + 1)), SEEK_CUR);
	} else if (count == n - 1 && dst[n - 2] == 0x0D) {
		/* Part of a newline is being read. Unread the first part. */
		dst[n - 2] = '\0';
		lseek(fileno(handle), -1, SEEK_CUR);
	}

	return dst;
}

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

int fputc(int ch, FILE* handle)
{
	char ch_c = ch;
	ssize_t result = write(fileno(handle), &ch_c, 1);

	if (result <= 0)
		return EOF;
	else
		return (int) ch_c;
}

int fputs(const char* restrict s, FILE* restrict handle)
{
	size_t len = strlen(s);
	ssize_t result = write(fileno(handle), s, len);

	if (result != len)
		return EOF;
	else
		return 1;
}

size_t fread(void* restrict dst, size_t size, size_t n, FILE* restrict handle)
{
	ssize_t result;

	result = read(fileno(handle), dst, n * size);
	if (result == -1)
		return 0;
	else
		return result / size;
}

int fseek(FILE* handle, long offset, int whence)
{
	return lseek(fileno(handle), offset, whence) != (off_t) -1 ? 0 : -1;
}

int fsetpos(FILE* handle, const fpos_t* pos)
{
	return lseek(fileno(handle), *pos, SEEK_SET);
}

int fstat(int fd, struct stat* dst)
{
	return _FAT_fstat(fd, dst);
}

int fsync(int fd)
{
	if (IS_STDIN(fd) || IS_STDOUT(fd) || IS_STDERR(fd))
		return 0;

	return _FAT_fsync(fd);
}

int ftruncate(int fd, off_t new_length)
{
	if (IS_STDIN(fd) || IS_STDOUT(fd) || IS_STDERR(fd))
		return 0;

	return _FAT_ftruncate(fd, new_length);
}

size_t fwrite(const void* restrict src, size_t size, size_t n, FILE* restrict handle)
{
	ssize_t result;

	result = write(fileno(handle), src, n * size);
	if (result == -1)
		return 0;
	else
		return result / size;
}

/* All functions that seek into the file redirect to this function.
 * If the core function changes, please change them in all functions
 * that call this one. */
off_t lseek(int fd, off_t offset, int whence)
{
	if (IS_STDOUT(fd) || IS_STDERR(fd) || IS_STDIN(fd))
		return -1;

	return _FAT_seek(fd, offset, whence);
}

int mkdir(const char* path, mode_t mode)
{
	return _FAT_mkdir(path, mode);
}

int open(const char* file, int flags, mode_t mode)
{
	FILE_STRUCT* handle = malloc(sizeof(FILE_STRUCT));

	if (handle == NULL) {
		errno = EMFILE; /* Too many open files (exhausted memory) */
		return -1;
	}

	if (_FAT_open(handle, file, flags, mode) == -1) {
		goto error;
	}

	return fileno(handle);

error:
	free(handle);
	return -1;
}

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

int puts(const char* s)
{
	fputs(s, stdout);
	fputc('\n', stdout);
	return 1;
}

struct dirent* readdir(DIR* dir)
{
	int result = _FAT_dirnext((DIR_ITER*) dir, ((DIR_ITER*) dir)->d->d_name, NULL);

	if (result == 0)
		return ((DIR_ITER*) dir)->d;
	else
		return NULL;
}

int remove(const char* file)
{
	return _FAT_unlink(file);
}

int rename(const char* oldName, const char* newName)
{
	return _FAT_rename(oldName, newName);
}

void rewind(FILE* handle)
{
	lseek(fileno(handle), 0, SEEK_SET);
}

void rewinddir(DIR* dir)
{
	_FAT_dirreset((DIR_ITER*) dir);
}

int rmdir(const char* path)
{
	return _FAT_unlink(path);
}

int stat(const char* restrict path, struct stat* restrict dst)
{
	return _FAT_stat(path, dst);
}

int statvfs(const char* restrict path, struct statvfs* restrict dst)
{
	return _FAT_statvfs(path, dst);
}

int unlink(const char* file)
{
	return _FAT_unlink(file);
}
