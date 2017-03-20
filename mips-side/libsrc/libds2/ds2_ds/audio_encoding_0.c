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

#include <stddef.h>
#include <string.h>

#include "audio.h"
#include "card_protocol.h"
#include "globals.h"

size_t _audio_encoding_0(size_t audio_send_index, size_t audio_write_index)
{
	size_t max_samples = 508 >> _audio_sample_size_shift, samples;

	if (audio_send_index < audio_write_index) {
		samples = audio_write_index - audio_send_index;
		if (samples > max_samples)
			samples = max_samples;

		memcpy(&_global_reply.bytes[4],
		       &_audio_buffer[audio_send_index << _audio_sample_size_shift],
		       samples << _audio_sample_size_shift);
	} else {
		size_t samples_a, samples_b;
		samples = _audio_buffer_samples - (audio_send_index - audio_write_index);
		if (samples > max_samples)
			samples = max_samples;
		samples_a = _audio_buffer_samples - audio_send_index;
		if (samples_a > max_samples)
			samples_a = max_samples;
		samples_b = samples - samples_a;

		memcpy(&_global_reply.bytes[4],
		       &_audio_buffer[audio_send_index << _audio_sample_size_shift],
		       samples_a << _audio_sample_size_shift);
		memcpy(&_global_reply.bytes[4 + (samples_a << _audio_sample_size_shift)],
		       _audio_buffer,
		       samples_b << _audio_sample_size_shift);
	}

	_global_reply.words[0] = DATA_KIND_AUDIO | DATA_ENCODING(0) | DATA_BYTE_COUNT(samples << _audio_sample_size_shift);
	_send_reply(&_global_reply, 512);

	return samples;
}
