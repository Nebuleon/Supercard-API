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

#include <ds2/pm.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "../intc.h"
#include "audio.h"
#include "audio_encoding_0.h"
#include "globals.h"

size_t DS2_GetFreeAudioSamples(void)
{
	uint32_t section = DS2_EnterCriticalSection();
	size_t snd_read = _ds2_ds.snd_read, snd_write = _ds2_ds.snd_write;
	DS2_LeaveCriticalSection(section);

	if (snd_read > snd_write)
		return snd_read - snd_write - 1;
	else
		return _ds2_ds.snd_samples - (snd_write - snd_read) - 1;
}

void _audio_dequeue(void)
{
	size_t result = _audio_encoding_0(_ds2_ds.snd_send, _ds2_ds.snd_write);

	_ds2_ds.snd_send = _add_wrap_fast(_ds2_ds.snd_send, result, _ds2_ds.snd_samples);

	if (_ds2_ds.snd_send != _ds2_ds.snd_write)
		_add_pending_send(PENDING_SEND_AUDIO);
}

void _audio_consumed(size_t samples)
{
	_ds2_ds.snd_read = _add_wrap_fast(_ds2_ds.snd_read, samples, _ds2_ds.snd_samples);
}

int DS2_SubmitAudio(const void* data, size_t n)
{
	uint32_t section;

	/* Wait until the audio is fully started before submitting samples. */
	DS2_StartAwait();
	while (_ds2_ds.snd_status == AUDIO_STATUS_STARTING)
		DS2_AwaitInterrupt();
	DS2_StopAwait();

	if (_ds2_ds.snd_status != AUDIO_STATUS_STARTED)
		return EFAULT;
	if (n == 0)
		return 0;

	section = DS2_EnterCriticalSection();
	while (n > 0) {
		size_t snd_read = _ds2_ds.snd_read, snd_write = _ds2_ds.snd_write;
		size_t transfer_samples;

		if (snd_write >= snd_read) {
			transfer_samples = _ds2_ds.snd_samples - snd_write;
			if (snd_read == 0)
				transfer_samples--; /* Ensure there's a 1-sample gap */
		} else {
			transfer_samples = snd_read - snd_write - 1;
		}
		if (transfer_samples > n)
			transfer_samples = n;

		if (transfer_samples > 0) {
			memcpy(&_ds2_ds.snd_buffer[snd_write << _ds2_ds.snd_size_shift],
			       data,
			       transfer_samples << _ds2_ds.snd_size_shift);
			n -= transfer_samples;
			data = (void*) ((uint8_t*) data + (transfer_samples << _ds2_ds.snd_size_shift));
			_ds2_ds.snd_write = _add_wrap_fast(_ds2_ds.snd_write, transfer_samples, _ds2_ds.snd_samples);
			_add_pending_send(PENDING_SEND_AUDIO);
		} else {
			DS2_LeaveCriticalSection(section);
			DS2_StartAwait();
			while (_ds2_ds.snd_read == snd_read)
				DS2_AwaitInterrupt();
			DS2_StopAwait();
			section = DS2_EnterCriticalSection();
		}
	}
	DS2_LeaveCriticalSection(section);
	return 0;
}
