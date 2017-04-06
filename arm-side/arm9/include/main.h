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

#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include <nds.h>

#include "card_protocol.h"

/* Current state of input on the DS. Used by requests.c to know when to stop
 * sleeping. */
struct DS_InputState input;

/* The following must be sorted in order of priority to be sent.
 * Bit 0 has the highest priority; bit 31 has the lowest. */

/* A VBlank notification must be sent. Highest priority. */
#define PENDING_SEND_VBLANK              0x00000001

/* A notification must be sent that a new Main Screen buffer is shown on
 * screen. */
#define PENDING_SEND_VIDEO_DISPLAYED     0x00000002

/* A notification must be sent that some of the oldest sound data has been
 * consumed. */
#define PENDING_SEND_AUDIO_CONSUMED      0x00000004

/* A notification must be sent that the audio is either done being started
 * or done being stopped. */
#define PENDING_SEND_AUDIO_STATUS        0x00000008

/* The newly-pressed button mask and touch position data must be sent. */
#define PENDING_SEND_INPUT               0x00000010

/* The latest reading of the Real-Time Clock must be sent. */
#define PENDING_SEND_RTC                 0x00000020

/* The Supercard's queue still has some data we must ask for. Lowest priority. */
#define PENDING_SEND_QUEUE               0x80000000

/* Adds one or more things to the list of things to be sent to the Supercard.
 * The caller must protect calls to this function with a critical section. */
void add_pending_send(uint32_t mask);

/* Removes one or more things from the list of things to be sent to the
 * Supercard. The caller must protect calls to this function with a critical
 * section. */
void remove_pending_send(uint32_t mask);

#endif /* !MAIN_H */