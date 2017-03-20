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

#ifndef __ASSERT_H__
#define __ASSERT_H__

#undef assert

#ifdef NDEBUG
#  define assert(cond) ((void) 0)
#else
extern void _assert_fail(const char* file, unsigned int line, const char* text) __attribute__((noreturn, cold));

#  define assert(cond) \
	do { \
		if (!(cond)) _assert_fail(__FILE__, __LINE__, #cond); \
	} while (0)
#endif

#endif /* !__ASSERT_H__ */
