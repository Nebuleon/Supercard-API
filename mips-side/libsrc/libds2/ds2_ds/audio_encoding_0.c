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

#include "card_protocol.h"
#include "globals.h"

size_t _audio_encoding_0(size_t snd_send, size_t snd_write)
{
	size_t max_samples = 508 >> _ds2_ds.snd_size_shift, samples;

	if (snd_send < snd_write) {
		samples = snd_write - snd_send;
		if (samples > max_samples)
			samples = max_samples;

		memcpy(&_ds2_ds.temp.bytes[4],
		       &_ds2_ds.snd_buffer[snd_send << _ds2_ds.snd_size_shift],
		       samples << _ds2_ds.snd_size_shift);
	} else {
		size_t samples_a, samples_b;
		samples = _ds2_ds.snd_samples - (snd_send - snd_write);
		if (samples > max_samples)
			samples = max_samples;
		samples_a = _ds2_ds.snd_samples - snd_send;
		if (samples_a > max_samples)
			samples_a = max_samples;
		samples_b = samples - samples_a;

		memcpy(&_ds2_ds.temp.bytes[4],
		       &_ds2_ds.snd_buffer[snd_send << _ds2_ds.snd_size_shift],
		       samples_a << _ds2_ds.snd_size_shift);
		memcpy(&_ds2_ds.temp.bytes[4 + (samples_a << _ds2_ds.snd_size_shift)],
		       _ds2_ds.snd_buffer,
		       samples_b << _ds2_ds.snd_size_shift);
	}

	_ds2_ds.temp.words[0] = DATA_KIND_AUDIO | DATA_ENCODING(0) | DATA_BYTE_COUNT(samples << _ds2_ds.snd_size_shift);
	_send_reply(&_ds2_ds.temp, sizeof(_ds2_ds.temp));

	return samples;
}
