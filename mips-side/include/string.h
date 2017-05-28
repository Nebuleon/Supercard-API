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

#ifndef __STRING_H__
#define __STRING_H__

/* Also defined in stddef.h, stdlib.h, time.h, wchar.h. */
#ifndef NULL
#  define NULL ((void*) 0)
#endif

/* Also defined in stddef.h, stdlib.h, time.h, wchar.h, sys/types.h.
 * The name _SIZE_T_ is also used by sysdefs.h. */
#ifndef _SIZE_T_
#  define _SIZE_T_
typedef __SIZE_TYPE__ size_t;
#endif

extern void* memchr(const void* src, int ch, size_t n) __attribute__((nonnull));

extern int memcmp(const void* a, const void* b, size_t n) __attribute__((nonnull));

extern void* memcpy(void* restrict dst, const void* restrict src, size_t n) __attribute__((nonnull, returns_nonnull));

extern void* memmove(void* dst, const void* src, size_t n) __attribute__((nonnull, returns_nonnull));

extern void* memset(void* dst, int value, size_t n) __attribute__((nonnull, returns_nonnull));

extern char* strcat(char* restrict dst, const char* restrict src) __attribute__((nonnull, returns_nonnull));

extern char* strchr(const char* s, int ch) __attribute__((nonnull, pure));

extern int strcmp(const char *a, const char *b) __attribute__((nonnull));

extern char* strcpy(char* restrict dst, const char* restrict src) __attribute__((nonnull, returns_nonnull));

extern size_t strcspn(const char* haystack, const char* antineedle) __attribute__((nonnull, pure));

extern char* strdup(const char* s) __attribute__((nonnull));

extern char* strerror(int errnum) __attribute__((returns_nonnull));

extern size_t strlen(const char* src) __attribute__((nonnull, pure));

extern char* strncat(char* restrict dst, const char* restrict src, size_t n) __attribute__((nonnull, returns_nonnull));

extern int strncmp(const char *s1, const char *s2, size_t n) __attribute__((nonnull, pure));

extern char* strncpy(char* restrict dst, const char* restrict src, size_t n) __attribute__((nonnull, returns_nonnull));

/* This function is an extension. */
extern size_t strnlen(const char* s, size_t n) __attribute__((nonnull, pure));

extern char* strpbrk(const char* haystack, const char* needle) __attribute__((nonnull, pure));

extern char* strrchr(const char* s, int ch) __attribute__((nonnull, pure));

extern size_t strspn(const char* haystack, const char* needle) __attribute__((nonnull, pure));

extern char* strstr(const char* haystack, const char* needle) __attribute__((nonnull, pure));

extern char* strtok(char* restrict haystack, const char* restrict needle) __attribute__((nonnull));

#endif /* !__STRING_H__ */
