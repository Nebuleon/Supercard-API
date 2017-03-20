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

#ifndef __WCHAR_H__
#define __WCHAR_H__

#include <stdarg.h>

/* Also defined in stddef.h. */
#ifndef _WCHAR_T_
#  define _WCHAR_T_
/* The largest supported wide character is Unicode U+10FFFF, 21-bit. */
typedef __UINT32_TYPE__ wchar_t;
#endif

#define WCHAR_MIN 0
#define WCHAR_MAX 0x10FFFF

/* Also defined in stddef.h, stdlib.h, string.h, time.h. */
#ifndef NULL
#  define NULL ((void*) 0)
#endif

/* Also defined in stddef.h, stdlib.h, string.h, time.h, sys/types.h.
 * The name _SIZE_T_ is also used by sysdefs.h. */
#ifndef _SIZE_T_
#  define _SIZE_T_
typedef __SIZE_TYPE__ size_t;
#endif

/* Also defined in wctype.h. */
#ifndef _WINT_T_
#  define _WINT_T_
typedef __INT32_TYPE__ wint_t;
#endif

/* Also defined in wctype.h. */
#ifndef _WCTYPE_T_
#  define _WCTYPE_T_
typedef unsigned int wctype_t;
#endif

/* Also defined in wctype.h. */
#ifndef WEOF
#  define WEOF -1
#endif

typedef __UINT32_TYPE__ mbstate_t;

struct tm;

extern wint_t btowc(int ch) __attribute__((const));

extern int iswalnum(wint_t ch) __attribute__((const));

extern int iswalpha(wint_t ch) __attribute__((const));

extern int iswcntrl(wint_t ch) __attribute__((const));

extern int iswdigit(wint_t ch) __attribute__((const));

extern int iswgraph(wint_t ch) __attribute__((const));

extern int iswlower(wint_t ch) __attribute__((const));

extern int iswprint(wint_t ch) __attribute__((const));

extern int iswpunct(wint_t ch) __attribute__((const));

extern int iswspace(wint_t ch) __attribute__((const));

extern int iswupper(wint_t ch) __attribute__((const));

extern int iswxdigit(wint_t ch) __attribute__((const));

extern int iswctype(wint_t ch, wctype_t type) __attribute__((const));

extern int mblen(const char *s, size_t n);

extern size_t mbrlen(const char *restrict s, size_t n, mbstate_t *restrict st);

extern size_t mbrtowc(wchar_t *restrict wc, const char *restrict src, size_t n, mbstate_t *restrict st);

extern int mbsinit(const mbstate_t *st);

extern size_t mbsnrtowcs(wchar_t *restrict wcs, const char **restrict src, size_t n, size_t wn, mbstate_t *restrict st);

extern size_t mbsrtowcs(wchar_t *restrict ws, const char **restrict src, size_t wn, mbstate_t *restrict st);

extern size_t mbstowcs(wchar_t *restrict ws, const char *restrict s, size_t wn);

extern int mbtowc(wchar_t *restrict wc, const char *restrict src, size_t n);

extern wint_t towlower(wint_t ch);

extern wint_t towupper(wint_t ch);

extern size_t wcrtomb(char *restrict s, wchar_t wc, mbstate_t *restrict st);

extern size_t wcsnrtombs(char *restrict dst, const wchar_t **restrict wcs, size_t wn, size_t n, mbstate_t *restrict st);

extern size_t wcsrtombs(char *restrict s, const wchar_t **restrict ws, size_t n, mbstate_t *restrict st);

extern size_t wcstombs(char *restrict s, const wchar_t *restrict ws, size_t n);

extern int wctob(wint_t c);

extern int wctomb(char *s, wchar_t wc);

#endif /* !__WCHAR_H__ */
