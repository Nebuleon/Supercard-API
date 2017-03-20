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

/* true if audio is playing; false if it's stopped. */
extern bool _audio_started;

/* The audio sampling frequency (samples per second). */
extern uint16_t _audio_frequency;

/* true if the audio is signed little-endian 16-bit PCM;
 * false if it's unsigned 8-bit PCM. */
extern bool _audio_is_16bit;

/* true if the audio is stereo, interleaved in this way:
 * [left right] [left right] ...
 * false if the audio is mono. */
extern bool _audio_is_stereo;

/* The base-2 logarithm of the length in bytes of a sample. Can be used with
 * << to know how many bytes are taken up by a certain number of samples, or
 * >> to know how many samples fit in a certain number of bytes. */
extern size_t _audio_sample_size_shift;

/* The audio output buffer. It is dynamically allocated, as its length in
 * samples is controlled by the application. It is a ring buffer defined by
 * audio_buffer_size, audio_read_index and audio_write_index (all in samples,
 * not bytes).
 *
 * Its format is 8-bit or 16-bit according to the value of 'audio_is_16bit',
 * as well as mono or stereo according to the value of 'audio_is_stereo'. */
extern uint8_t* _audio_buffer;

/* Number of samples in the audio buffer. To allow us to distinguish an empty
 * buffer from a full one, an empty one will have audio_read_index ==
 * audio_write_index, but a full one won't; it will instead have a 1-sample
 * gap. */
extern size_t _audio_buffer_samples;

/* Index of the first sample that has not been yet reported as played by the
 * Nintendo DS.
 * volatile because it can be modified by the card command interrupt handler,
 * and it's then tested in a loop to see if more data can be submitted.
 * Reading both 'audio_write_index' and 'audio_read_index' at once must be done
 * in a critical section or with interrupts disabled to prevent reads from
 * getting interrupted. */
extern volatile size_t _audio_read_index;

/* Index of the first sample that has not been sent to the Nintendo DS
 * altogether.
 * Must be accessed in a critical section or with interrupts disabled. */
extern size_t _audio_send_index;

/* Index of the first sample that may be written by the Supercard.
 * volatile because it's used along with 'audio_read_index' to determine the
 * number of free or used samples in the buffer.
 * Reading both 'audio_write_index' and 'audio_read_index' at once must be done
 * in a critical section or with interrupts disabled to prevent reads from
 * getting interrupted. */
extern volatile size_t _audio_write_index;

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
