/*
 * This file is part of the DS communication library for the Supercard DSTwo.
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

#ifndef __DS2_DS_AUDIO_H__
#define __DS2_DS_AUDIO_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

extern void _audio_dequeue(void);

extern void _audio_consumed(size_t samples);

static inline size_t _add_wrap_fast(size_t index, size_t increment, size_t buffer_size)
{
	index += increment;
	if (index >= buffer_size)
		index -= buffer_size;
	return index;
}

#endif /* !__DS2_DS_AUDIO_H__ */
