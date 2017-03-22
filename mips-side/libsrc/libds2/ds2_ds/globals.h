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

#ifndef __DS2_DS_GLOBALS_H__
#define __DS2_DS_GLOBALS_H__

#include <ds2/ds.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "card_protocol.h"

#define MAIN_BUFFER_COUNT 3

struct _video_entry {
	uint16_t* src;
	volatile uint8_t* busy;
	uint16_t pixel_offset;
	uint16_t pixel_count;
	uint8_t buffer;
	enum DS_Engine engine;
};

/* This struct contains all the variables used for communication with the
 * Nintendo DS. For quicker access, it should be kept under 32768 bytes. */
struct __attribute__((aligned(32))) _ds2_ds {
	/* The last data returned by DS2_GetInputState(). */
	struct DS_InputState in_state __attribute__((aligned (4)));

	/* Button presses that have yet to be added to _in_state.
	 * Must be read in a critical section or with interrupts disabled to prevent
	 * reads from getting interrupted by writes of new input state from the DS. */
	struct DS_InputState in_presses __attribute__((aligned (4)));

	/* Button releases that have yet to be added to _in_state.
	 * Must be read in a critical section or with interrupts disabled to prevent
	 * reads from getting interrupted by writes of new input state from the DS. */
	struct DS_InputState in_releases __attribute__((aligned (4)));

	/* The last data sent by the Nintendo DS pertaining to its real-time clock.
	 * Must be read in a critical section or with interrupts disabled to prevent
	 * reads from getting interrupted by writes of a new RTC value from the DS. */
	struct DS_RTC rtc __attribute__((aligned(4)));

	/* DMA channel used by the communication link to the DS. */
	int dma_channel;

	/* Bitfield of things to be sent to the Nintendo DS when it asks for the send
	 * queue. Whenever this hits 0, the end bit is sent, and further queued data
	 * must be sent after triggering the card line.
	 * Must be accessed in a critical section or with interrupts disabled to
	 * prevent a situation where the card command handler makes it hit 0, but the
	 * function that is adding to the queue does not trigger the card line because
	 * it didn't see 0, and the queue is forever stuck. */
	uint32_t pending_sends;

	/* true if audio is playing; false if it's stopped. */
	bool snd_started;

	/* The audio sampling frequency (samples per second). */
	uint16_t snd_freq;

	/* true if the audio is signed little-endian 16-bit PCM;
	 * false if it's unsigned 8-bit PCM. */
	bool snd_16bit;

	/* true if the audio is stereo, interleaved in this way:
	 * [left right] [left right] ...
	 * false if the audio is mono. */
	bool snd_stereo;

	/* The base-2 logarithm of the length in bytes of a sample. Can be used with
	 * << to know how many bytes are taken up by a certain number of samples, or
	 * >> to know how many samples fit in a certain number of bytes. */
	size_t snd_size_shift;

	/* The audio output buffer. It is dynamically allocated, as its length in
	 * samples is controlled by the application. It is a ring buffer defined by
	 * snd_buffer_size, snd_read_index and snd_write_index (all in samples,
	 * not bytes).
	 *
	 * Its format is 8-bit or 16-bit according to the value of 'snd_16bit',
	 * as well as mono or stereo according to the value of 'snd_stereo'. */
	uint8_t* snd_buffer;

	/* Number of samples in the audio buffer. To allow us to distinguish an empty
	 * buffer from a full one, an empty one will have snd_read_index ==
	 * snd_write_index, but a full one won't; it will instead have a 1-sample
	 * gap. */
	size_t snd_samples;

	/* Index of the first sample that has not been yet reported as played by the
	 * Nintendo DS.
	 * volatile because it can be modified by the card command interrupt handler,
	 * and it's then tested in a loop to see if more data can be submitted.
	 * Reading both 'snd_write_index' and 'snd_read_index' at once must be done
	 * in a critical section or with interrupts disabled to prevent reads from
	 * getting interrupted. */
	volatile size_t snd_read;

	/* Index of the first sample that has not been sent to the Nintendo DS
	 * altogether.
	 * Must be accessed in a critical section or with interrupts disabled. */
	size_t snd_send;

	/* Index of the first sample that may be written by the Supercard.
	 * volatile because it's used along with 'snd_read_index' to determine the
	 * number of free or used samples in the buffer.
	 * Reading both 'snd_write_index' and 'snd_read_index' at once must be done
	 * in a critical section or with interrupts disabled to prevent reads from
	 * getting interrupted. */
	volatile size_t snd_write;

	/* Text waiting to be sent to the Nintendo DS.
	 * Aligned to 32 bytes so as to affect one fewer cache line than if it were
	 * not. */
	char txt_data[508] __attribute__((aligned (32)));

	/* The number of meaningful bytes at the start of _text_data.
	 * volatile because it's modified by _text_dequeue as part of the card command
	 * interrupt handler, and it's then tested in a loop to see if more text can
	 * be submitted. */
	volatile size_t txt_size;

	/* Pixel formats for each engine. Indexed by 'enum DS2_Engine' - 1. */
	enum DS2_PixelFormat vid_formats[2];

	/* Contains the number of the Main Screen buffer being written into by the
	 * Supercard. */
	uint8_t vid_main_current;

	/* Contains the number of the Main Screen buffer being displayed by the
	 * Nintendo DS.
	 * volatile because it can be modified by the card command interrupt handler,
	 * and it's then tested in a loop to see if more data can be submitted. */
	volatile uint8_t vid_main_displayed;

	/* true if the Main Screen is on the top and the Sub Screen is on the bottom.
	 * false if the Main Screen is on the bottom and the Sub Screen is on the top.
	 */
	bool vid_swap;

	/* Contains bits described in 'enum DS_Screen' detailing which backlights are
	 * on. */
	enum DS_Screen vid_backlights;

	/* true if the last operation on the Main Screen was a flip; false if it was
	 * an update. This is tracked so we can know if '_vid_main_current' should
	 * match '_ds2_ds.vid_main_displayed' (previous operation was an update) or if it
	 * should NOT match it (previous operation was a flip). */
	bool vid_last_was_flip;

	/* Contains an entry for each Main Screen buffer stating whether it's being
	 * sent.
	 * volatile because it can be modified by the card command interrupt handler,
	 * and it's then tested in a loop to see if more data can be submitted.
	 * Reading all elements at once must be done in a critical section or with
	 * interrupts disabled to prevent reads from getting interrupted. */
	volatile uint8_t vid_main_busy[MAIN_BUFFER_COUNT];

	/* 1 if the Sub Screen buffer is being transferred to the Nintendo DS,
	 * or 0 if it is free to use.
	 * volatile because it can be modified by the card command interrupt handler,
	 * and it's then tested in a loop to see if more data can be submitted. */
	volatile uint8_t vid_sub_busy;

	struct _video_entry vid_queue[MAIN_BUFFER_COUNT + 1];

	size_t vid_queue_count;

	/* Number of vertical blanking interrupts (VBlank) encountered by the Nintendo
	 * DS. Overflows every 414.252 days.
	 * volatile because it can be modified by the card command interrupt handler,
	 * and it's then tested for inequality in a loop to await the next VBlank. */
	uint32_t vblank_count;

	/* Status of the link between the DS and the Supercard.
	 * volatile because it can be modified by the card command interrupt handler. */
	volatile enum _mips_side_link_status link_status;

	/* Current protocol handler. Depends on the link status.
	 * Not volatile, despite _link_status being volatile, because it's never used
	 * outside of interrupt handlers. */
	void (*current_protocol) (const union card_command*);

	/* Things to be received from the DS before the link can be considered
	 * established. */
	uint32_t pending_recvs;

	uint8_t vid_encodings_supported;

	uint8_t snd_encodings_supported;

	struct card_reply_mips_assert assert_failure __attribute__((aligned (32)));

	struct card_reply_requests requests __attribute__((aligned (32)));

	struct card_reply_mips_exception exception __attribute__((aligned (32)));

	/* Used when the code needs global memory to send a reply that is constructed
	 * on-the-fly, because stack memory is undefined after a function exits.
	 * Aligned to 32 bytes so as to affect one fewer cache line than if it were
	 * not. */
	union card_reply_512 temp __attribute__((aligned (32)));
};

extern struct _ds2_ds _ds2_ds;

/* The following must be sorted in order of priority to be sent.
 * Bit 0 has the highest priority; bit 31 has the lowest. */

/* An exception report must be sent to the Nintendo DS. */
#define PENDING_SEND_EXCEPTION 0x00000001

/* An assertion failure report must be sent to the Nintendo DS. */
#define PENDING_SEND_ASSERT    0x00000002

/* The pending requests must be sent to the Nintendo DS. */
#define PENDING_SEND_REQUESTS  0x00000004

/* Some audio can be submitted to the Nintendo DS. */
#define PENDING_SEND_AUDIO     0x00000008

/* Some text can be submitted to the Nintendo DS. */
#define PENDING_SEND_TEXT      0x20000000

/* Some video can be submitted to the Nintendo DS. */
#define PENDING_SEND_VIDEO     0x40000000

/* An end of queue reply must be sent to the Nintendo DS. Must have the lowest
 * priority. */
#define PENDING_SEND_END       0x80000000

#endif /* !__DS2_DS_GLOBALS_H__ */
