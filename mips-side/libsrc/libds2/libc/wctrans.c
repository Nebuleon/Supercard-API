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

#include <wctype.h>
#include <string.h>

wctrans_t wctrans(const char* spec)
{
	if (strcmp(spec, "tolower") == 0)
		return (wctrans_t) 1;
	if (strcmp(spec, "toupper") == 0)
		return (wctrans_t) 2;
	return 0;
}

wint_t towctrans(wint_t ch, wctrans_t trans)
{
	switch (trans) {
		case (wctrans_t) 1: return towlower(ch);
		case (wctrans_t) 2: return towupper(ch);
		default:            return ch;
	}
}
