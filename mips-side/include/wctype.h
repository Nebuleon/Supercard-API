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

#ifndef __WCTYPE_H__
#define __WCTYPE_H__

/* Also defined in wchar.h. */
#ifndef _WINT_T_
#  define _WINT_T_
typedef __INT32_TYPE__ wint_t;
#endif

/* Also defined in wchar.h. */
#ifndef _WCTYPE_T_
#  define _WCTYPE_T_
typedef unsigned int wctype_t;
#endif

/* Also defined in wchar.h. */
#ifndef WEOF
#  define WEOF -1
#endif

typedef unsigned int wctrans_t;

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

extern wint_t towctrans(wint_t ch, wctrans_t trans);

extern wint_t towlower(wint_t ch);

extern wint_t towupper(wint_t ch);

extern wctrans_t wctrans(const char* spec) __attribute__((const));

extern wctype_t wctype(const char* spec) __attribute__((const));

#endif /* !__WCTYPE_H__ */
