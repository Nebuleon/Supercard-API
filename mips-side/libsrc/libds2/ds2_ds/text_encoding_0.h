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

#ifndef __DS2_DS_TEXT_ENCODING_0_H__
#define __DS2_DS_TEXT_ENCODING_0_H__

#include <stddef.h>

/*
 * Text encoding 0 is simply uncompressed text sent to the Nintendo DS.
 *
 * That text is queued up by writes to stdout and stderr.
 *
 * In:
 *   text: A pointer to the first character to be sent.
 *   length: The number of characters to be sent. Regardless of the value of
 *     this argument, 508 bytes are valid at and after *src. The actual length
 *     is sent in the header.
 */
extern void _text_encoding_0(const char* text, size_t length);

#endif /* !__DS2_DS_TEXT_ENCODING_0_H__ */
