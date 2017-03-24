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

#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* true if audio is started; false if it is stopped. */
extern bool audio_started;

/* true if the Supercard requires notifications after audio has been started
 * or stopped, in order to avoid race conditions involving submission of new
 * audio data and audio_consumed. */
extern bool audio_status_required;

/* The base-2 logarithm of the length in bytes of a sample. Can be used with
 * << to know how many bytes are taken up by a certain number of samples, or
 * >> to know how many samples fit in a certain number of bytes. */
extern DTCM_BSS size_t audio_sample_size_shift;

/* The number of samples in 'audio_buffer'. This value is the size requested
 * on the wire plus 1, to allow the code to distinguish an empty ring buffer
 * from a full one. */
extern DTCM_BSS size_t audio_buffer_samples;

/* The samples that have been received from the Supercard and are to be sent
 * to the ARM7. There are 'audio_buffer_samples' samples here, each
 * 'audio_sample_size' bytes.
 *
 * Note: Only the pointer is in the DTCM.
 */
extern DTCM_BSS uint8_t* audio_buffer;

/* The index of the sample we will next read from in 'audio_buffer'.
 * This is the head of the ring buffer. */
extern DTCM_BSS size_t audio_read_index;

/* The index of the sample we will next write to in 'audio_buffer'.
 * This is the tail of the ring buffer. */
extern DTCM_BSS size_t audio_write_index;

/* The number of samples of audio consumed since the last time we sent a
 * notification to the Supercard telling it we consumed some of the audio
 * from the head of the buffer. */
extern DTCM_BSS uint16_t audio_consumed;

/* Initialises the audio system. */
extern void audio_init(void);

/* Starts a stream of PCM data. From the call to this function until the next
 * call to audio_stop, samples will be sent to the audio hardware via the ARM7
 * processor.
 *
 * In:
 *   frequency: The sampling rate in Hertz.
 *   buffer_size: The maximum number of samples to be kept by the ARM9 before
 *     they are sent to the audio hardware.
 *   is_16bit:
 *   - true: Data is signed little-endian 16-bit PCM.
 *   - false: Data is unsigned 8-bit PCM.
 *   is_stereo:
 *   - true: Each sample is 2 values (16-bit or 8-bit) next to each other,
 *     first the one for the left channel, then the one for the right channel.
 *   - false: Each sample is 1 value (16-bit or 8-bit).
 */
extern void audio_start(uint16_t frequency, size_t buffer_size, bool is_16bit, bool is_stereo);

/* Stops a stream of PCM data. */
extern void audio_stop(void);

static inline size_t add_wrap_fast(size_t index, size_t increment, size_t buffer_size)
{
	index += increment;
	if (index >= buffer_size)
		index -= buffer_size;
	return index;
}

#endif /* !AUDIO_H */
