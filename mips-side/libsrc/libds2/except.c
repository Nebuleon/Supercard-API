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

#include <ds2/except.h>

exception_handler _exception_handlers[32];

void* _exception_data[32];

extern void _c_exception_save(void);

void DS2_SetExceptionHandler(unsigned int excode, exception_handler handler, void* data)
{
	if (excode < 32) {
		_exception_handlers[excode] = handler;
		_exception_data[excode] = data;
	}
}

void DS2_SetCExceptionHandler(unsigned int excode, c_exception_handler handler)
{
	if (excode < 32) {
		_exception_handlers[excode] = _c_exception_save;
		_exception_data[excode] = handler;
	}
}
