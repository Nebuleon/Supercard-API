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

#ifndef __CTYPE_H__
#define __CTYPE_H__

extern int isalnum(int c) __attribute__((const));

extern int isalpha(int c) __attribute__((const));

extern int islower(int c) __attribute__((const));

extern int isupper(int c) __attribute__((const));

extern int isdigit(int c) __attribute__((const));

extern int isxdigit(int c) __attribute__((const));

extern int iscntrl(int c) __attribute__((const));

extern int isgraph(int c) __attribute__((const));

extern int isspace(int c) __attribute__((const));

extern int isblank(int c) __attribute__((const));

extern int isprint(int c) __attribute__((const));

extern int ispunct(int c) __attribute__((const));

extern int tolower(int c) __attribute__((const));

extern int toupper(int c) __attribute__((const));

#endif /* !__CTYPE_H__ */
