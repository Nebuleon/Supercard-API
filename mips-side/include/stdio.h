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

#ifndef __STDIO_H__
#define __STDIO_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <stdarg.h>

#define BUFSIZ 512 /* This is the size of a microSD sector */

/* Keep this in sync with the value of MAX_FILENAME_LENGTH in
 * libfat/source/directory.h. That's a private header, so don't include it
 * here. */
#define FILENAME_MAX 768
#define FOPEN_MAX 16

#define _IOFBF 1 /* I dunno */
#define _IOLBF 2 /* I dunno */
#define _IONBF 3 /* I dunno */

/* Also defined in unistd.h. */
#if !defined SEEK_SET
#  define SEEK_SET 1
#endif
#if !defined SEEK_CUR
#  define SEEK_CUR 2
#endif
#if !defined SEEK_END
#  define SEEK_END 3
#endif

#define EOF -1

struct _FILE_STRUCT;

typedef struct _FILE_STRUCT FILE;

typedef long fpos_t;

extern FILE* const stdin;
extern FILE* const stdout;
extern FILE* const stderr;

/*
 * Prints (outputs with formatting) a variable argument list into a newly
 * allocated buffer.
 *
 * In:
 *   format: The format string, which decides when each member of '...' gets
 *     output, and how, exactly as in any other printf family function.
 *   ...: A variable argument list.
 * Out:
 *   ptr: Updated to point to the newly-allocated buffer on success. Undefined
 *     on failure.
 * Returns:
 *   On success, the number of bytes printed into *ptr.
 *   On failure, -1.
 */
extern int asprintf(char **ptr, const char *format, ...);

extern void clearerr(FILE*);

extern int fclose(FILE*);

extern FILE* fdopen(int fd, const char* mode);

extern int feof(FILE*);

extern int ferror(FILE*);

extern int fflush(FILE*);

extern int fgetc(FILE*);

extern int fgetpos(FILE* restrict handle, fpos_t* restrict pos);

extern char* fgets(char* restrict dst, int n, FILE* restrict handle);

#define fileno(file) ((int) (file))

extern FILE* fopen(const char* restrict name, const char* restrict symbolic_mode);

extern int fprintf(FILE* restrict handle, const char* restrict format, ...) __attribute__((format (printf, 2, 3)));

extern int fputc(int ch, FILE*);

extern int fputs(const char* restrict s, FILE* restrict handle);

extern size_t fread(void* restrict dst, size_t size, size_t n, FILE* restrict handle);

// TODO Implement this function
// extern int fscanf(FILE* restrict handle, const char* restrict format, ...);

extern int fseek(FILE*, long, int whence);

extern int fsetpos(FILE*, const fpos_t*);

extern long ftell(FILE*);

extern size_t fwrite(const void* restrict src, size_t size, size_t n, FILE* restrict handle);

#define getc fgetc

extern void perror(const char* s);

extern int printf(const char* restrict format, ...);

#define putc fputc

extern int puts(const char* s);

extern int remove(const char* path);

extern int rename(const char* oldName, const char* newName);

extern void rewind(FILE*);

// Not implemented
// extern int scanf(const char* restrict format, ...);

extern int snprintf(char* restrict dst, size_t n, const char* restrict format, ...) __attribute__((format (printf, 3, 4)));

extern int sprintf(char* restrict dst, const char* restrict format, ...) __attribute__((format (printf, 2, 3)));

extern int sscanf(const char* restrict s, const char* restrict format, ...) __attribute__((format (scanf, 2, 3)));

// TODO Implement this function
// extern int ungetc(int ch, FILE*);

extern int vfprintf(FILE* restrict handle, const char* restrict format, va_list ap) __attribute__((format (printf, 2, 0)));

/*
 * Prints (outputs with formatting) from a captured variable argument list
 * into a newly allocated buffer.
 *
 * In:
 *   format: The format string, which decides when each member of 'ap' gets
 *     output, and how, exactly as in any other printf family function.
 *   ap: A variable argument list captured using va_start.
 * Out:
 *   ptr: Updated to point to the newly-allocated buffer on success. Undefined
 *     on failure.
 * Returns:
 *   On success, the number of bytes printed into *ptr.
 *   On failure, -1.
 */
extern int vasprintf(char **ptr, const char *format, va_list ap) __attribute__((format (printf, 2, 0)));

// Not implemented
// extern int vprintf(const char* restrict format, va_list ap) __attribute__((format (printf, 1, 0)));

extern int vsnprintf(char* restrict dst, size_t n, const char* restrict format, va_list ap) __attribute__((format (printf, 3, 0)));

// TODO Implement this function
// extern int vsprintf(char* dst, const char* format, va_list ap);

#ifdef __cplusplus
}
#endif

#endif /* !__STDIO_H__ */
