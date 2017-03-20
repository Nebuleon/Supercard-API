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

#include <stdint.h>

#define CONTROL 0x01
#define SPACE   0x02
#define PUNCT   0x04
#define UPPER   0x08
#define LOWER   0x10
#define DIGIT   0x20
#define XDIGIT  0x40
#define BLANK   0x80

static const uint8_t lut[0x80] = {
	/* 0x00 NUL */ CONTROL,
	/* 0x01 SOH */ CONTROL,
	/* 0x02 STX */ CONTROL,
	/* 0x03 ETX */ CONTROL,
	/* 0x04 EOT */ CONTROL,
	/* 0x05 ENQ */ CONTROL,
	/* 0x06 ACK */ CONTROL,
	/* 0x07 BEL */ CONTROL,
	/* 0x08 BS  */ CONTROL,
	/* 0x09 HT  */ CONTROL | SPACE | BLANK,
	/* 0x0A LF  */ CONTROL | SPACE,
	/* 0x0B VT  */ CONTROL | SPACE,
	/* 0x0C FF  */ CONTROL | SPACE,
	/* 0x0D CR  */ CONTROL | SPACE,
	/* 0x0E SO  */ CONTROL,
	/* 0x0F SI  */ CONTROL,

	/* 0x10 DLE */ CONTROL,
	/* 0x11 DC1 */ CONTROL,
	/* 0x12 DC2 */ CONTROL,
	/* 0x13 DC3 */ CONTROL,
	/* 0x14 DC4 */ CONTROL,
	/* 0x15 NAK */ CONTROL,
	/* 0x16 SYN */ CONTROL,
	/* 0x17 ETB */ CONTROL,
	/* 0x18 CAN */ CONTROL,
	/* 0x19 EM  */ CONTROL,
	/* 0x1A SUB */ CONTROL,
	/* 0x1B ESC */ CONTROL,
	/* 0x1C FS  */ CONTROL,
	/* 0x1D GS  */ CONTROL,
	/* 0x1E RS  */ CONTROL,
	/* 0x1F US  */ CONTROL,

	/* 0x20 ' ' */ SPACE | BLANK,
	/* 0x21 '!' */ PUNCT,
	/* 0x22 '"' */ PUNCT,
	/* 0x23 '#' */ PUNCT,
	/* 0x24 '$' */ PUNCT,
	/* 0x25 '%' */ PUNCT,
	/* 0x26 '&' */ PUNCT,
	/* 0x27 ''' */ PUNCT,
	/* 0x28 '(' */ PUNCT,
	/* 0x29 ')' */ PUNCT,
	/* 0x2A '*' */ PUNCT,
	/* 0x2B '+' */ PUNCT,
	/* 0x2C ',' */ PUNCT,
	/* 0x2D '-' */ PUNCT,
	/* 0x2E '.' */ PUNCT,
	/* 0x2F '/' */ PUNCT,

	/* 0x30 '0' */ DIGIT | XDIGIT,
	/* 0x31 '1' */ DIGIT | XDIGIT,
	/* 0x32 '2' */ DIGIT | XDIGIT,
	/* 0x33 '3' */ DIGIT | XDIGIT,
	/* 0x34 '4' */ DIGIT | XDIGIT,
	/* 0x35 '5' */ DIGIT | XDIGIT,
	/* 0x36 '6' */ DIGIT | XDIGIT,
	/* 0x37 '7' */ DIGIT | XDIGIT,
	/* 0x38 '8' */ DIGIT | XDIGIT,
	/* 0x39 '9' */ DIGIT | XDIGIT,
	/* 0x3A ':' */ PUNCT,
	/* 0x3B ';' */ PUNCT,
	/* 0x3C '<' */ PUNCT,
	/* 0x3D '=' */ PUNCT,
	/* 0x3E '>' */ PUNCT,
	/* 0x3F '?' */ PUNCT,

	/* 0x40 '@' */ PUNCT,
	/* 0x41 'A' */ UPPER | XDIGIT,
	/* 0x42 'B' */ UPPER | XDIGIT,
	/* 0x43 'C' */ UPPER | XDIGIT,
	/* 0x44 'D' */ UPPER | XDIGIT,
	/* 0x45 'E' */ UPPER | XDIGIT,
	/* 0x46 'F' */ UPPER | XDIGIT,
	/* 0x47 'G' */ UPPER,
	/* 0x48 'H' */ UPPER,
	/* 0x49 'I' */ UPPER,
	/* 0x4A 'J' */ UPPER,
	/* 0x4B 'K' */ UPPER,
	/* 0x4C 'L' */ UPPER,
	/* 0x4D 'M' */ UPPER,
	/* 0x4E 'N' */ UPPER,
	/* 0x4F 'O' */ UPPER,

	/* 0x50 'P' */ UPPER,
	/* 0x51 'Q' */ UPPER,
	/* 0x52 'R' */ UPPER,
	/* 0x53 'S' */ UPPER,
	/* 0x54 'T' */ UPPER,
	/* 0x55 'U' */ UPPER,
	/* 0x56 'V' */ UPPER,
	/* 0x57 'W' */ UPPER,
	/* 0x58 'X' */ UPPER,
	/* 0x59 'Y' */ UPPER,
	/* 0x5A 'Z' */ UPPER,
	/* 0x5B '[' */ PUNCT,
	/* 0x5C '\' */ PUNCT,
	/* 0x5D ']' */ PUNCT,
	/* 0x5E '^' */ PUNCT,
	/* 0x5F '_' */ PUNCT,

	/* 0x60 '`' */ PUNCT,
	/* 0x61 'a' */ LOWER | XDIGIT,
	/* 0x62 'b' */ LOWER | XDIGIT,
	/* 0x63 'c' */ LOWER | XDIGIT,
	/* 0x64 'd' */ LOWER | XDIGIT,
	/* 0x65 'e' */ LOWER | XDIGIT,
	/* 0x66 'f' */ LOWER | XDIGIT,
	/* 0x67 'g' */ LOWER,
	/* 0x68 'h' */ LOWER,
	/* 0x69 'i' */ LOWER,
	/* 0x6A 'j' */ LOWER,
	/* 0x6B 'k' */ LOWER,
	/* 0x6C 'l' */ LOWER,
	/* 0x6D 'm' */ LOWER,
	/* 0x6E 'n' */ LOWER,
	/* 0x6F 'o' */ LOWER,

	/* 0x70 'p' */ LOWER,
	/* 0x71 'q' */ LOWER,
	/* 0x72 'r' */ LOWER,
	/* 0x73 's' */ LOWER,
	/* 0x74 't' */ LOWER,
	/* 0x75 'u' */ LOWER,
	/* 0x76 'v' */ LOWER,
	/* 0x77 'w' */ LOWER,
	/* 0x78 'x' */ LOWER,
	/* 0x79 'y' */ LOWER,
	/* 0x7A 'z' */ LOWER,
	/* 0x7B '{' */ PUNCT,
	/* 0x7C '|' */ PUNCT,
	/* 0x7D '}' */ PUNCT,
	/* 0x7E '~' */ PUNCT,
	/* 0x7F DEL */ CONTROL,
};

int isalnum(int c)
{
	if (c >= 0 && c <= 0x7F)
		return lut[c] & (UPPER | LOWER | DIGIT);
	else
		return 0;
}

int isalpha(int c)
{
	if (c >= 0 && c <= 0x7F)
		return lut[c] & (UPPER | LOWER);
	else
		return 0;
}

int islower(int c)
{
	if (c >= 0 && c <= 0x7F)
		return lut[c] & LOWER;
	else
		return 0;
}

int isupper(int c)
{
	if (c >= 0 && c <= 0x7F)
		return lut[c] & UPPER;
	else
		return 0;
}

int isdigit(int c)
{
	if (c >= 0 && c <= 0x7F)
		return lut[c] & DIGIT;
	else
		return 0;
}

int isxdigit(int c)
{
	if (c >= 0 && c <= 0x7F)
		return lut[c] & XDIGIT;
	else
		return 0;
}

int iscntrl(int c)
{
	if (c >= 0 && c <= 0x7F)
		return lut[c] & CONTROL;
	else
		return 0;
}

int isgraph(int c)
{
	if (c >= 0 && c <= 0x7F)
		return lut[c] & (UPPER | LOWER | DIGIT | PUNCT);
	else
		return 0;
}

int isspace(int c)
{
	if (c >= 0 && c <= 0x7F)
		return lut[c] & SPACE;
	else
		return 0;
}

int isblank(int c)
{
	if (c >= 0 && c <= 0x7F)
		return lut[c] & BLANK;
	else
		return 0;
}

int isprint(int c)
{
	if (c >= 0 && c <= 0x7F)
		return !(lut[c] & CONTROL);
	else
		return 0;
}

int ispunct(int c)
{
	if (c >= 0 && c <= 0x7F)
		return lut[c] & PUNCT;
	else
		return 0;
}

int tolower(int c)
{
	if (c >= 'A' && c <= 'Z')
		return c + 0x20;
	else
		return c;
}

int toupper(int c)
{
	if (c >= 'a' && c <= 'z')
		return c - 0x20;
	else
		return c;
}
