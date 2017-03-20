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

#include "globals.h"

int _ds2ds_dma_channel;

struct DS_RTC _rtc;

struct DS_InputState _input_state __attribute__((aligned (4)));

struct DS_InputState _input_presses __attribute__((aligned (4)));

struct DS_InputState _input_releases __attribute__((aligned (4)));

uint32_t _pending_sends;

union card_reply_512 _global_reply __attribute__((aligned (32)));
