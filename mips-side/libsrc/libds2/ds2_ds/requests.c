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
#include <ds2/pm.h>

#include "../intc.h"
#include "../dma.h"
#include "audio.h"
#include "globals.h"
#include "video.h"

extern void _reset(void) __attribute__((cold));

void DS2_SetScreenSwap(bool swap)
{
	uint32_t section = DS2_EnterCriticalSection();

	_ds2_ds.requests.change_swap = 1;
	_ds2_ds.vid_swap = swap ? 1 : 0;
	_ds2_ds.requests.swap_screens = _ds2_ds.vid_swap;

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

		_ds2_ds.requests.change_backlight = 1;
		_ds2_ds.vid_backlights = _ds2_ds.requests.screen_backlights = screens;

		_add_pending_send(PENDING_SEND_REQUESTS);
		DS2_LeaveCriticalSection(section);
	}
	return 0;
}

int DS2_StartAudio(uint16_t frequency, uint16_t buffer_size, bool is_16bit, bool is_stereo)
{
	/* Wait for a previous start request to be done, if any. */
	DS2_StartAwait();
	while (_ds2_ds.snd_status == AUDIO_STATUS_STARTING)
		DS2_AwaitInterrupt();
	DS2_StopAwait();
	
	if (_ds2_ds.snd_status == AUDIO_STATUS_STARTED) {
		DS2_StopAudio();

		/* Wait until the audio is fully stopped before starting it again. */
		DS2_StartAwait();
		while (_ds2_ds.snd_status == AUDIO_STATUS_STOPPING)
			DS2_AwaitInterrupt();
		DS2_StopAwait();
	}

	{
		uint32_t section = DS2_EnterCriticalSection();

		_ds2_ds.requests.is_16bit = is_16bit ? 1 : 0;
		_ds2_ds.requests.is_stereo = is_stereo ? 1 : 0;
		_ds2_ds.requests.buffer_size = buffer_size;

		_ds2_ds.snd_size_shift = _ds2_ds.requests.is_16bit + _ds2_ds.requests.is_stereo;

		_ds2_ds.snd_samples = _ds2_ds.requests.buffer_size + 1;
		_ds2_ds.snd_buffer = malloc(_ds2_ds.snd_samples << _ds2_ds.snd_size_shift);
		if (_ds2_ds.snd_buffer == NULL) {
			DS2_LeaveCriticalSection(section);
			return ENOMEM;
		}

		_ds2_ds.requests.audio_freq = frequency;
		_ds2_ds.requests.start_audio = 1;

		_ds2_ds.snd_freq = _ds2_ds.requests.audio_freq;
		_ds2_ds.snd_16bit = _ds2_ds.requests.is_16bit;
		_ds2_ds.snd_stereo = _ds2_ds.requests.is_stereo;
		_ds2_ds.snd_read = _ds2_ds.snd_send = _ds2_ds.snd_write = 0;
		_ds2_ds.snd_status = AUDIO_STATUS_STARTING;

		_add_pending_send(PENDING_SEND_REQUESTS);
		DS2_LeaveCriticalSection(section);
	}
	return 0;
}

void DS2_StopAudio(void)
{
	/* Wait until the audio is fully started before stopping it. */
	DS2_StartAwait();
	while (_ds2_ds.snd_status == AUDIO_STATUS_STARTING)
		DS2_AwaitInterrupt();
	DS2_StopAwait();

	if (_ds2_ds.snd_status == AUDIO_STATUS_STARTED) {
		uint32_t section = DS2_EnterCriticalSection();

		_ds2_ds.requests.stop_audio = 1;

		_ds2_ds.snd_status = AUDIO_STATUS_STOPPING;
		free(_ds2_ds.snd_buffer);
		_ds2_ds.snd_buffer = NULL;
		_remove_pending_send(PENDING_SEND_AUDIO);

		_add_pending_send(PENDING_SEND_REQUESTS);
		DS2_LeaveCriticalSection(section);
	}
}

void _request_nds_reset(void)
{
	uint32_t section = DS2_EnterCriticalSection();

	_ds2_ds.requests.reset = 1;

	_add_pending_send(PENDING_SEND_REQUESTS);
	DS2_LeaveCriticalSection(section);
}

void _send_requests(void)
{
	/* All requests issued up to this point are sent, '_ds2_ds.requests' is
	 * cleared, and requests are no longer pending. Any further requests will
	 * go to a new packet. */
	_ds2_ds.temp.words[0] = DATA_KIND_REQUESTS | DATA_ENCODING(0) | DATA_BYTE_COUNT(sizeof(_ds2_ds.requests));
	memcpy(&_ds2_ds.temp.words[1], &_ds2_ds.requests, sizeof(_ds2_ds.requests));

	if (_ds2_ds.requests.reset) {
		/* We're about to send the reset request. Wait for it to be fully
		 * sent, then initiate our reset immediately. */
		_send_reply(&_ds2_ds.temp, sizeof(_ds2_ds.temp));
		dma_join(_ds2_ds.dma_channel);
		_reset();
	} else {
		memset(&_ds2_ds.requests, 0, sizeof(_ds2_ds.requests));

		_send_reply(&_ds2_ds.temp, sizeof(_ds2_ds.temp));
	}
}
