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
#include <stdint.h>
#include <inttypes.h>

#include "audio.h"
#include "audio_encoding_0.h"
#include "card_protocol.h"

void audio_encoding_0(uint32_t header_1)
{
	size_t bytes = (header_1 & DATA_BYTE_COUNT_MASK) >> DATA_BYTE_COUNT_BIT;
	union card_reply_512 reply_512;
	size_t samples, src_sample = 0;
	if ((bytes & ((1 << audio_sample_size_shift) - 1)) != 0) {
		fatal_link_error("Audio encoding 0 data is not\na whole number of samples\n\nSize received: %zu\nSample size: %zu", bytes, 1 << audio_sample_size_shift);
	}
	samples = bytes >> audio_sample_size_shift;

	REG_IME = IME_ENABLE;
	card_read_data((bytes + 3) & ~3, &reply_512, false);
	card_ignore_reply();
	REG_IME = IME_DISABLE;

	while (samples > 0) {
		size_t transfer_samples = audio_buffer_samples - audio_write_index;
		if (transfer_samples > samples)
			transfer_samples = samples;
		if ((audio_read_index == 0 && audio_write_index + transfer_samples == audio_buffer_samples)
		 || (audio_read_index != 0 && audio_write_index < audio_read_index
		  && audio_write_index + transfer_samples >= audio_read_index)) {
			fatal_link_error("Supercard sent enough audio to\ncause a buffer overrun that\nbehaves like an underrun");
		}

		memcpy(&audio_buffer[audio_write_index << audio_sample_size_shift],
		       &reply_512.bytes[src_sample << audio_sample_size_shift],
		       transfer_samples << audio_sample_size_shift);
		src_sample += transfer_samples;
		audio_write_index = add_wrap_fast(audio_write_index, transfer_samples, audio_buffer_samples);
		samples -= transfer_samples;
	}
}
