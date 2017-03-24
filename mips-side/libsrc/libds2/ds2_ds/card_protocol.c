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

#include <asm/cachectl.h>
#include <string.h>

#include "assert.h"
#include "audio.h"
#include "card_protocol.h"
#include "except.h"
#include "input.h"
#include "main.h"
#include "globals.h"
#include "requests.h"
#include "text.h"
#include "video.h"
#include "../dma.h"

#define MIPS_VIDEO_ENCODINGS 1
#define MIPS_AUDIO_ENCODINGS 1

void _add_pending_send(uint32_t mask)
{
	uint32_t sends_copy = _ds2_ds.pending_sends;
	if (sends_copy == 0 && mask != 0) {
		/* We had nothing to send, but now we have something to send. Let the
		 * Nintendo DS know; it will then send us commands to get the data. */
		REG_CPLD_FIFO_STATE = CPLD_FIFO_STATE_NDS_IQE_OUT;
		/* And immediately set the card line to low, because the interrupt is
		 * edge-triggered on the ARM side. This prepares us for the next time
		 * the queue transitions from empty to busy, too. */
		REG_CPLD_FIFO_STATE = 0;
		_ds2_ds.pending_sends = mask | PENDING_SEND_END;
	} else {
		_ds2_ds.pending_sends = sends_copy | mask;
	}
}

void _remove_pending_send(uint32_t mask)
{
	_ds2_ds.pending_sends &= ~mask;
}

/* Must be protected by a critical section or be run with interrupts
 * disabled. */
static uint32_t _take_pending_send(void)
{
	uint32_t sends = _ds2_ds.pending_sends;
	uint32_t result = sends & (~sends + 1); /* Get the lowest set bit */
	_ds2_ds.pending_sends = sends & ~result; /* Remove it from the bitfield */
	return result;
}

void _start_reply(void)
{
	REG_CPLD_CTR = CPLD_CTR_FPGA_MODE | CPLD_CTR_FIFO_CLEAR;
	REG_CPLD_CTR = CPLD_CTR_FPGA_MODE;
}

void _send_reply_4(uint32_t reply)
{
	union card_reply_4 reply_4;
	reply_4.word = reply;

	REG_CPLD_FIFO_WRITE_NDSRDATA = reply_4.halfwords[0];
	REG_CPLD_FIFO_WRITE_NDSRDATA = reply_4.halfwords[1];
}

void _send_reply(const void* reply, size_t reply_len)
{
	dcache_writeback_range(reply, reply_len);
	dma_start(_ds2_ds.dma_channel, reply, (void*) CPLD_FIFO_WRITE_NDSRDATA_BASE, reply_len);
}

void _send_video_reply(const void* reply, size_t reply_len, enum DS_Engine engine)
{
	enum DS2_PixelFormat format = _ds2_ds.vid_formats[engine - 1];
	REG_CPLD_CTR = CPLD_CTR_FPGA_MODE | CPLD_CTR_FIX_VIDEO_EN
	             | (format == DS2_PIXEL_FORMAT_RGB555 ? CPLD_CTR_FIX_VIDEO_RGB_EN : 0);

	_send_reply(reply, reply_len);
}

static void _send_end(void)
{
	_ds2_ds.temp.words[0] = DATA_KIND_NONE | DATA_BYTE_COUNT(0) | DATA_END;
	_send_reply(&_ds2_ds.temp, sizeof(_ds2_ds.temp));
}

void _link_establishment_protocol(const union card_command* command)
{
	if (command->bytes[0] == CARD_COMMAND_HELLO_BYTE) {
		struct card_reply_hello* reply = (struct card_reply_hello*) _ds2_ds.temp.bytes;
		size_t i;

		reply->hello_value = SUPERCARD_HELLO_VALUE;
		reply->video_encodings_supported = MIPS_VIDEO_ENCODINGS;
		reply->audio_encodings_supported = MIPS_AUDIO_ENCODINGS;
		reply->extensions.audio_status = 1;
		memset(&reply->reserved, 0, sizeof(reply->reserved));
		for (i = 0; i < sizeof(reply->end_sync); i++) {
			reply->end_sync[i] = i;
		}

		_start_reply();
		_send_reply(reply, 512);

		_ds2_ds.vid_encodings_supported = MIPS_VIDEO_ENCODINGS;
		if (command->hello.video_encodings_supported < _ds2_ds.vid_encodings_supported)
			_ds2_ds.vid_encodings_supported = command->hello.video_encodings_supported;

		_ds2_ds.snd_encodings_supported = MIPS_AUDIO_ENCODINGS;
		if (command->hello.audio_encodings_supported < _ds2_ds.snd_encodings_supported)
			_ds2_ds.snd_encodings_supported = command->hello.audio_encodings_supported;

		_ds2_ds.link_status = LINK_STATUS_PENDING_RECV;
		_ds2_ds.current_protocol = _pending_recv_protocol;
	}
}

void _pending_recv_protocol(const union card_command* command)
{
	_start_reply();

	switch (command->bytes[0]) {
		case CARD_COMMAND_RTC_BYTE:
		{
			const struct card_command_rtc* command_rtc = &command->rtc;

			_send_reply_4(0);
			_ds2_ds.rtc = command_rtc->data;
			_ds2_ds.pending_recvs &= ~PENDING_RECV_RTC;
			break;
		}

		case CARD_COMMAND_INPUT_BYTE:
		{
			const struct card_command_input* command_input = &command->input;

			_send_reply_4(0);
			_ds2_ds.in_state = command_input->data;
			_ds2_ds.pending_recvs &= ~PENDING_RECV_INPUT;
			break;
		}
	}

	if (_ds2_ds.pending_recvs == 0) {
		_ds2_ds.link_status = LINK_STATUS_ESTABLISHED;
		_ds2_ds.current_protocol = _main_protocol;
	}
}

void _main_protocol(const union card_command* command)
{
	_start_reply();

	switch (command->bytes[0]) {
		case CARD_COMMAND_RTC_BYTE:
		{
			const struct card_command_rtc* command_rtc = &command->rtc;

			_send_reply_4(0);
			_ds2_ds.rtc = command_rtc->data;
			break;
		}

		case CARD_COMMAND_INPUT_BYTE:
		{
			const struct card_command_input* command_input = &command->input;

			_send_reply_4(0);
			_merge_input(&command_input->data);
			break;
		}

		case CARD_COMMAND_VBLANK_BYTE:
			_send_reply_4(0);
			_ds2_ds.vblank_count++;
			break;

		case CARD_COMMAND_VIDEO_DISPLAYED_BYTE:
		{
			const struct card_command_video_displayed* command_video_displayed = &command->video_displayed;

			_send_reply_4(0);
			_video_displayed(command_video_displayed->index);
			break;
		}

		case CARD_COMMAND_AUDIO_CONSUMED_BYTE:
		{
			const struct card_command_audio_consumed* command_audio_consumed = &command->audio_consumed;

			_send_reply_4(0);
			_audio_consumed(command_audio_consumed->count);
			break;
		}

		case CARD_COMMAND_AUDIO_STATUS_BYTE:
		{
			const struct card_command_audio_status* command_audio_status = &command->audio_status;

			_send_reply_4(0);
			_ds2_ds.snd_status = command_audio_status->status ? AUDIO_STATUS_STARTED : AUDIO_STATUS_STOPPED;
			break;
		}

		case CARD_COMMAND_SEND_QUEUE_BYTE:
		{
			uint32_t pending_send = _take_pending_send();

			switch (pending_send) {
				case PENDING_SEND_EXCEPTION: _send_exception(); break;
				case PENDING_SEND_ASSERT:    _send_assert();    break;
				case PENDING_SEND_REQUESTS:  _send_requests();  break;
				case PENDING_SEND_AUDIO:     _audio_dequeue();  break;
				case PENDING_SEND_TEXT:      _text_dequeue();   break;
				case PENDING_SEND_VIDEO:     _video_dequeue();  break;
				case PENDING_SEND_END:       _send_end();       break;
			}
			break;
		}

		default:
			_send_reply_4(0);
			break;
	}
}
