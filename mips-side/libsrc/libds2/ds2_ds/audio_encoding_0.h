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

#ifndef __DS2_DS_AUDIO_ENCODING_0_H__
#define __DS2_DS_AUDIO_ENCODING_0_H__

#include <stddef.h>

/*
 * Audio encoding 0 is simply uncompressed audio.
 *
 * In:
 *   audio_send_index: The index of the first sample in _audio_buffer to send.
 *   audio_write_index: One past the index of the last sample in _audio_buffer
 *     to send.
 * Returns:
 *   The number of samples sent.
 */
extern size_t _audio_encoding_0(size_t audio_send_index, size_t audio_write_index);

#endif /* !__DS2_DS_AUDIO_ENCODING_0_H__ */
