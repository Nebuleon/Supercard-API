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

extern int _ds2ds_dma_channel;

/* The last data sent by the Nintendo DS pertaining to its real-time clock.
 * Must be read in a critical section or with interrupts disabled to prevent
 * reads from getting interrupted by writes of a new RTC value from the DS. */
extern struct DS_RTC _rtc;

/* The last data returned by DS2_GetInputState(). */
extern struct DS_InputState _input_state __attribute__((aligned (4)));

/* Button presses that have yet to be added to _input_state.
 * Must be read in a critical section or with interrupts disabled to prevent
 * reads from getting interrupted by writes of new input state from the DS. */
extern struct DS_InputState _input_presses __attribute__((aligned (4)));

/* Button releases that have yet to be added to _input_state.
 * Must be read in a critical section or with interrupts disabled to prevent
 * reads from getting interrupted by writes of new input state from the DS. */
extern struct DS_InputState _input_releases __attribute__((aligned (4)));

/* Bitfield of things to be sent to the Nintendo DS when it asks for the send
 * queue. Whenever this hits 0, the end bit is sent, and further queued data
 * must be sent after triggering the card line.
 * Must be accessed in a critical section or with interrupts disabled to
 * prevent a situation where the card command handler makes it hit 0, but the
 * function that is adding to the queue does not trigger the card line because
 * it didn't see 0, and the queue is forever stuck. */
extern uint32_t _pending_sends;

/* Used when the code needs global memory to send a reply that is constructed
 * on-the-fly, because stack memory is undefined after a function exits.
 * Aligned to 32 bytes so as to affect one fewer cache line than if it were
 * not. */
extern union card_reply_512 _global_reply __attribute__((aligned (32)));

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
