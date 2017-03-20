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

#include <nds.h>
#include <stdbool.h>
#include <stdint.h>
#include <maxmod9.h>

#include "audio.h"
#include "card_protocol.h"
#include "main.h"

bool audio_started;

DTCM_BSS size_t audio_sample_size_shift;

DTCM_BSS size_t audio_buffer_samples;

DTCM_BSS uint8_t* audio_buffer;

DTCM_BSS size_t audio_read_index;

DTCM_BSS size_t audio_write_index;

DTCM_BSS uint16_t audio_consumed;

void audio_init()
{
	mm_ds_system initdata;

	initdata.mod_count = 0;
	initdata.samp_count = 0;
	initdata.mem_bank = 0;
	initdata.fifo_channel = FIFO_MAXMOD;

	mmInit(&initdata);
}

static mm_word audio_update(mm_word length, mm_addr dest, mm_stream_formats format)
{
	size_t samples = 0;
	int previous_ime = enterCriticalSection();

	if (audio_read_index < audio_write_index) {
		samples = audio_write_index - audio_read_index;
		if (samples > length)
			samples = length;

		memcpy(dest,
		       &audio_buffer[audio_read_index << audio_sample_size_shift],
		       samples << audio_sample_size_shift);

		add_pending_send(PENDING_SEND_AUDIO_CONSUMED);
	} else if (audio_read_index > audio_write_index) {
		size_t samples_a, samples_b;
		samples = audio_buffer_samples - (audio_read_index - audio_write_index);
		if (samples > length)
			samples = length;
		samples_a = audio_buffer_samples - audio_read_index;
		if (samples_a > length)
			samples_a = length;
		samples_b = samples - samples_a;

		memcpy(dest,
		       &audio_buffer[audio_read_index << audio_sample_size_shift],
		       samples_a << audio_sample_size_shift);
		memcpy((uint8_t*) dest + (samples_a << audio_sample_size_shift),
		       audio_buffer,
		       samples_b << audio_sample_size_shift);

		add_pending_send(PENDING_SEND_AUDIO_CONSUMED);
	}

	audio_read_index = add_wrap_fast(audio_read_index, samples, audio_buffer_samples);
	audio_consumed += samples;

	memset((uint8_t*) dest + (samples << audio_sample_size_shift),
	       0,
	       (length - samples) << audio_sample_size_shift);

	leaveCriticalSection(previous_ime);
	return length;
}

void audio_start(uint16_t frequency, size_t buffer_size, bool is_16bit, bool is_stereo)
{
	mm_stream streamdata;

	if (audio_started) {
		audio_stop();
	}

	streamdata.sampling_rate = frequency;
	/* This buffer size calculation is based on the interrupt load that the
	 * ARM9 and ARM7 can sustain, as well as the FIFO load that is required
	 * to send data to the ARM7: one every 2 milliseconds. Any longer would
	 * unnecessarily increase sound latency; any shorter would cause audible
	 * pops due to excessive load. */
	streamdata.buffer_length = frequency / 500;
	/* And don't send too few samples at once, because the DS sound hardware
	 * cannot handle too short bursts. */
	if (streamdata.buffer_length < 16)
		streamdata.buffer_length = 16;
	streamdata.callback = audio_update;
	streamdata.timer = MM_TIMER0;
	streamdata.manual = false;
	streamdata.format = is_stereo
	                  ? (is_16bit ? MM_STREAM_16BIT_STEREO : MM_STREAM_8BIT_STEREO)
	                  : (is_16bit ? MM_STREAM_16BIT_MONO : MM_STREAM_8BIT_MONO);

	audio_sample_size_shift = (is_stereo ? 1 : 0) + (is_16bit ? 1 : 0);

	audio_buffer_samples = buffer_size + 1;
	audio_buffer = malloc(audio_buffer_samples << audio_sample_size_shift);
	audio_read_index = audio_write_index = 0;

	REG_IME = IME_ENABLE;  /* must be enabled for maxmod communications */
	mmStreamOpen(&streamdata);
	REG_IME = IME_DISABLE;
}

void audio_stop()
{
	REG_IME = IME_ENABLE;  /* must be enabled for maxmod communications */
	mmStreamClose();
	REG_IME = IME_DISABLE;

	remove_pending_send(PENDING_SEND_AUDIO_CONSUMED);
	audio_consumed = 0;

	free(audio_buffer);
	audio_buffer = NULL;

	audio_started = false;
}
