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

#ifndef __STDLIB_H__
#define __STDLIB_H__

/* Also defined in stddef.h, string.h, time.h, wchar.h. */
#ifndef NULL
#  define NULL ((void*) 0)
#endif

/* Also defined in stddef.h, string.h, time.h, wchar.h, sys/types.h.
 * The name _SIZE_T_ is also used by sysdefs.h. */
#ifndef _SIZE_T_
#  define _SIZE_T_
typedef __SIZE_TYPE__ size_t;
#endif

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

#define RAND_MAX 32767

/* The maximum number of bytes required in UTF-8, the only supported encoding,
 * to encode a 'wchar_t'. */
#define MB_CUR_MAX 4

typedef struct _div_t {
	int quot;
	int rem;
} div_t;

typedef struct _ldiv_t {
	long quot;
	long rem;
} ldiv_t;

typedef struct _lldiv_t {
	long long quot;
	long long rem;
} lldiv_t;

extern void* malloc(size_t n) __attribute__((malloc, alloc_size(1), assume_aligned(8)));

/* Requests a memory allocation of 'n' bytes, whose first byte is aligned to
 * 'alignment' bytes. More formally, as many trailing zeroes in the requested
 * 'alignment' value will also be trailing zeroes in the return value if it
 * is converted to uintptr_t.
 *
 * The returned allocation may be passed to 'free'. It may also be passed to
 * 'realloc', but reallocations that grow the object are not guaranteed to be
 * as aligned as was requested to 'memalign'.
 *
 * In:
 *   alignment: The requested alignment. This must be a power of 2 greater
 *     than or equal to sizeof(void*). If it's less than sizeof(void*), it
 *     is silently set to sizeof(void*), unless it is also not a power of 2.
 *   n: The number of bytes to be allocated.
 * Returns:
 *   A pointer to the first byte, which will be aligned as requested, on
 *   success.
 *   NULL if:
 *   - The alignment value is not a power of 2.
 *   - There was insufficient memory to satisfy the request.
 */
extern void* memalign(size_t alignment, size_t n) __attribute__((malloc, alloc_align(1), alloc_size(2)));

extern void* calloc(size_t count, size_t size) __attribute__((malloc, alloc_size(1, 2), assume_aligned(4)));

extern void* realloc(void* data, size_t n) __attribute__((alloc_size(2), assume_aligned(8)));

extern void free(void* data);

extern int abs(int n) __attribute__((const));

extern long labs(long n) __attribute__((const));

extern long long llabs(long long n) __attribute__((const));

extern div_t div(int numer, int denom) __attribute__((const));

extern ldiv_t ldiv(long numer, long denom) __attribute__((const));

extern lldiv_t lldiv(long long numer, long long denom) __attribute__((const));

// TODO Implement this function
// extern double atof(const char* ns) __attribute__((pure));

extern int atoi(const char* ns) __attribute__((pure));

extern long atol(const char* ns) __attribute__((pure));

extern long long atoll(const char* ns) __attribute__((pure));

// TODO Implement this function
// extern double strtod(const char* restrict ns, char** restrict end);

// TODO Implement this function
// extern float strtof(const char* restrict ns, char** restrict end);

// TODO Implement this function
// extern long double strtold(const char* restrict ns, char** restrict end);

extern long strtol(const char* restrict ns, char** restrict end, int base);

extern long long strtoll(const char* restrict ns, char** restrict end, int base);

extern unsigned long strtoul(const char* restrict ns, char** restrict end, int base);

extern unsigned long long strtoull(const char* restrict ns, char** restrict end, int base);

extern int rand(void);

extern void srand(unsigned int seed);

extern void abort(void) __attribute__((noreturn, cold));

extern int atexit(void (*f) (void));

extern void exit(int status) __attribute__((noreturn, cold));

extern void _Exit(int status) __attribute__((noreturn, cold));

extern void* bsearch(const void* key, const void* a, size_t n, size_t s,
	int (*cmp) (const void*, const void*));

extern void qsort(void* a, size_t n, size_t s,
	int (*cmp) (const void*, const void*));

extern char* getenv(const char* var);

#endif /* !__STDLIB_H__ */
