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

#ifndef __STDDEF_H__
#define __STDDEF_H__

/* Also defined in stdlib.h, string.h, time.h, wchar.h. */
#ifndef NULL
#  define NULL ((void*) 0)
#endif

#define offsetof(type, member) __builtin_offsetof(type, member)

typedef __PTRDIFF_TYPE__ ptrdiff_t;

/* Also defined in stdlib.h, string.h, time.h, wchar.h, sys/types.h.
 * The name _SIZE_T_ is also used by sysdefs.h. */
#ifndef _SIZE_T_
#  define _SIZE_T_
typedef __SIZE_TYPE__ size_t;
#endif

/* Also defined in wchar.h. */
#ifndef _WCHAR_T_
#  define _WCHAR_T_
/* The largest supported wide character type has up to U+10FFFF. */
typedef __UINT32_TYPE__ wchar_t;
#endif

#endif /* !__STDDEF_H__ */
