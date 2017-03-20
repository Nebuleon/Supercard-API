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

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../intc.h"
#include "../dma.h"
#include "audio.h"
#include "globals.h"
#include "video.h"

static struct card_reply_requests requests;

extern void _reset(void) __attribute__((cold));

void DS2_SetScreenSwap(bool swap)
{
	uint32_t section = DS2_EnterCriticalSection();

	requests.change_swap = 1;
	_video_swap = swap ? 1 : 0;
	requests.swap_screens = _video_swap;

	_add_pending_send(PENDING_SEND_REQUESTS);
	DS2_LeaveCriticalSection(section);
}

int DS2_SetScreenBacklights(enum DS_Screen screens)
{
	if ((screens & ~DS_SCREEN_BOTH) != 0) {
		return EINVAL;
	}

	{
		uint32_t section = DS2_EnterCriticalSection();

		requests.change_backlight = 1;
		_video_backlights = requests.screen_backlights = screens;

		_add_pending_send(PENDING_SEND_REQUESTS);
		DS2_LeaveCriticalSection(section);
	}
	return 0;
}

int DS2_StartAudio(uint16_t frequency, uint16_t buffer_size, bool is_16bit, bool is_stereo)
{
	if (_audio_started) {
		DS2_StopAudio();
	}

	{
		uint32_t section = DS2_EnterCriticalSection();

		requests.is_16bit = is_16bit ? 1 : 0;
		requests.is_stereo = is_stereo ? 1 : 0;
		requests.buffer_size = buffer_size;

		_audio_sample_size_shift = requests.is_16bit + requests.is_stereo;

		_audio_buffer_samples = requests.buffer_size + 1;
		_audio_buffer = malloc(_audio_buffer_samples << _audio_sample_size_shift);
		if (_audio_buffer == NULL) {
			DS2_LeaveCriticalSection(section);
			return ENOMEM;
		}

		requests.audio_freq = frequency;
		requests.start_audio = 1;

		_audio_frequency = requests.audio_freq;
		_audio_is_16bit = requests.is_16bit;
		_audio_is_stereo = requests.is_stereo;
		_audio_read_index = _audio_send_index = _audio_write_index = 0;
		_audio_started = true;

		_add_pending_send(PENDING_SEND_REQUESTS);
		DS2_LeaveCriticalSection(section);
	}
	return 0;
}

void DS2_StopAudio(void)
{
	if (!_audio_started) {
		uint32_t section = DS2_EnterCriticalSection();

		requests.stop_audio = 1;

		_audio_started = false;
		_audio_frequency = 0;
		_audio_is_16bit = false;
		_audio_is_stereo = false;
		_audio_sample_size_shift = 0;
		free(_audio_buffer);
		_audio_buffer = NULL;
		_audio_buffer_samples = 0;
		_audio_read_index = _audio_send_index = _audio_write_index = 0;
		_remove_pending_send(PENDING_SEND_AUDIO);

		_add_pending_send(PENDING_SEND_REQUESTS);
		DS2_LeaveCriticalSection(section);
	}
}

void _request_nds_reset(void)
{
	uint32_t section = DS2_EnterCriticalSection();

	requests.reset = 1;

	_add_pending_send(PENDING_SEND_REQUESTS);
	DS2_LeaveCriticalSection(section);
}

void _send_requests(void)
{
	/* All requests issued up to this point are sent, 'requests' is cleared,
	 * and requests are no longer pending. Any further requests will go to a
	 * new packet. */
	_global_reply.words[0] = DATA_KIND_REQUESTS | DATA_ENCODING(0) | DATA_BYTE_COUNT(sizeof(requests));
	memcpy(&_global_reply.words[1], &requests, sizeof(requests));

	if (requests.reset) {
		/* We're about to send the reset request. Wait for it to be fully
		 * sent, then initiate our reset immediately. */
		_send_reply(&_global_reply, 512);
		dma_join(_ds2ds_dma_channel);
		_reset();
	} else {
		memset(&requests, 0, sizeof(requests));

		_send_reply(&_global_reply, 512);
	}
}
