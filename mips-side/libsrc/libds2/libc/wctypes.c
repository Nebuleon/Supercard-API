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

wctype_t wctype(const char* spec)
{
	if (strcmp(spec, "alnum") == 0)
		return (wctype_t) 1;
	if (strcmp(spec, "alpha") == 0)
		return (wctype_t) 2;
	if (strcmp(spec, "blank") == 0)
		return (wctype_t) 3;
	if (strcmp(spec, "cntrl") == 0)
		return (wctype_t) 4;
	if (strcmp(spec, "digit") == 0)
		return (wctype_t) 5;
	if (strcmp(spec, "graph") == 0)
		return (wctype_t) 6;
	if (strcmp(spec, "lower") == 0)
		return (wctype_t) 7;
	if (strcmp(spec, "print") == 0)
		return (wctype_t) 8;
	if (strcmp(spec, "punct") == 0)
		return (wctype_t) 9;
	if (strcmp(spec, "space") == 0)
		return (wctype_t) 10;
	if (strcmp(spec, "upper") == 0)
		return (wctype_t) 11;
	if (strcmp(spec, "xdigit") == 0)
		return (wctype_t) 12;
	return 0;
}

int iswctype(wint_t ch, wctype_t type)
{
	switch (type) {
		case (wctype_t)  1: return iswalnum(ch);
		case (wctype_t)  2: return iswalpha(ch);
		case (wctype_t)  3: return iswspace(ch);
		case (wctype_t)  4: return iswcntrl(ch);
		case (wctype_t)  5: return iswdigit(ch);
		case (wctype_t)  6: return iswgraph(ch);
		case (wctype_t)  7: return iswlower(ch);
		case (wctype_t)  8: return iswprint(ch);
		case (wctype_t)  9: return iswpunct(ch);
		case (wctype_t) 10: return iswspace(ch);
		case (wctype_t) 11: return iswupper(ch);
		case (wctype_t) 12: return iswxdigit(ch);
		default:            return 0;
	}
}
