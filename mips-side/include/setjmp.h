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

#ifndef __SETJMP_H__
#define __SETJMP_H__

struct _setjmp_buffer {
	unsigned int regs[12];
};

typedef struct _setjmp_buffer[1] jmp_buf;

extern int setjmp(jmp_buf env) __attribute__((returns_twice));

extern int longjmp(jmp_buf env, int value) __attribute__((noreturn));

#endif /* !__SETJMP_H__ */
