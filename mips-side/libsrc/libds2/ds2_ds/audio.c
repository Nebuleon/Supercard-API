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

bool _audio_started;

uint16_t _audio_frequency;

bool _audio_is_16bit;

bool _audio_is_stereo;

size_t _audio_sample_size_shift;

uint8_t* _audio_buffer;

size_t _audio_buffer_samples;

volatile size_t _audio_read_index;

size_t _audio_send_index;

volatile size_t _audio_write_index;

size_t DS2_GetFreeAudioSamples(void)
{
	uint32_t section = DS2_EnterCriticalSection();
	size_t read_index_copy = _audio_read_index, write_index_copy = _audio_write_index;
	DS2_LeaveCriticalSection(section);

	if (read_index_copy > write_index_copy)
		return read_index_copy - write_index_copy - 1;
	else
		return _audio_buffer_samples - (write_index_copy - read_index_copy) - 1;
}

void _audio_dequeue(void)
{
	size_t result = _audio_encoding_0(_audio_send_index, _audio_write_index);

	_audio_send_index = _add_wrap_fast(_audio_send_index, result, _audio_buffer_samples);

	if (_audio_send_index != _audio_write_index)
		_add_pending_send(PENDING_SEND_AUDIO);
}

void _audio_consumed(size_t samples)
{
	/* Ignore any samples that are being marked by the DS as consumed when we
	 * haven't sent that many. This is to correctly handle the case where the
	 * sound was stopped via DS2_StopAudio, the request hasn't been sent yet,
	 * and an audio_consumed packet was sent by the Nintendo DS while we have
	 * no audio to mark as consumed anymore. */
	size_t send_index_copy = _audio_send_index, read_index_copy = _audio_read_index;
	size_t sent;

	if (send_index_copy > read_index_copy)
		sent = send_index_copy - read_index_copy;
	else
		sent = _audio_buffer_samples - (read_index_copy - send_index_copy);
	if (samples > sent)
		samples = sent;

	_audio_read_index = _add_wrap_fast(_audio_read_index, samples, _audio_buffer_samples);
}

int DS2_SubmitAudio(const void* data, size_t n)
{
	uint32_t section;

	if (!_audio_started)
		return EFAULT;
	if (n == 0)
		return 0;

	section = DS2_EnterCriticalSection();
	while (n > 0) {
		size_t read_index_copy = _audio_read_index, write_index_copy = _audio_write_index;
		size_t transfer_samples;

		if (write_index_copy >= read_index_copy) {
			transfer_samples = _audio_buffer_samples - write_index_copy;
			if (read_index_copy == 0)
				transfer_samples--; /* Ensure there's a 1-sample gap */
		} else {
			transfer_samples = read_index_copy - write_index_copy - 1;
		}
		if (transfer_samples > n)
			transfer_samples = n;

		if (transfer_samples > 0) {
			memcpy(&_audio_buffer[write_index_copy << _audio_sample_size_shift],
			       data,
			       transfer_samples << _audio_sample_size_shift);
			n -= transfer_samples;
			data = (void*) ((uint8_t*) data + (transfer_samples << _audio_sample_size_shift));
			_audio_write_index = _add_wrap_fast(_audio_write_index, transfer_samples, _audio_buffer_samples);
			_add_pending_send(PENDING_SEND_AUDIO);
		} else {
			DS2_LeaveCriticalSection(section);
			DS2_StartAwait();
			while (_audio_read_index == read_index_copy)
				DS2_AwaitInterrupt();
			DS2_StopAwait();
			section = DS2_EnterCriticalSection();
		}
	}
	DS2_LeaveCriticalSection(section);
	return 0;
}
