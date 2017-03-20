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
#include <ctype.h>

int iswalnum(wint_t ch)
{
	return iswalpha(ch) || iswdigit(ch);
}

int iswalpha(wint_t ch)
{
	return iswlower(ch) || iswupper(ch);
}

int iswcntrl(wint_t ch)
{
	/* TODO Provide for other control characters in the Unicode standard */
	return iscntrl(ch);
}

int iswdigit(wint_t ch)
{
	/* TODO Provide for other digits in the Unicode standard */
	return isdigit(ch);
}

int iswgraph(wint_t ch)
{
	/* TODO Provide for other graphing characters in the Unicode standard */
	return isgraph(ch);
}

int iswlower(wint_t ch)
{
	/* TODO Provide for other lowercase letters in the Unicode standard */
	return islower(ch);
}

int iswprint(wint_t ch)
{
	/* TODO Provide for other printable characters in the Unicode standard */
	return isprint(ch);
}

int iswspace(wint_t ch)
{
	/* TODO Provide for other spacing characters in the Unicode standard */
	return isspace(ch);
}

int iswupper(wint_t ch)
{
	/* TODO Provide for other uppercase letters in the Unicode standard */
	return isupper(ch);
}

int iswxdigit(wint_t ch)
{
	return isxdigit(ch);
}

wint_t towlower(wint_t ch)
{
	/* TODO Provide for other letters having case in the Unicode standard */
	return tolower(ch);
}

wint_t towupper(wint_t ch)
{
	/* TODO Provide for other letters having case in the Unicode standard */
	return toupper(ch);
}
