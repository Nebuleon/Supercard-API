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

#include <ds2/ds.h>
#include <ds2/pm.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include "../intc.h"
#include "video.h"
#include "globals.h"
#include "video_encoding_0.h"

uint16_t _video_main[MAIN_BUFFER_COUNT][DS_SCREEN_WIDTH * DS_SCREEN_HEIGHT] __attribute__((aligned (32)));

uint16_t _video_sub[DS_SCREEN_WIDTH * DS_SCREEN_HEIGHT] __attribute__((aligned (32)));

enum DS2_PixelFormat _video_pixel_formats[2] = {
	DS2_PIXEL_FORMAT_BGR555, DS2_PIXEL_FORMAT_BGR555
};

uint8_t _video_main_current;

volatile uint8_t _video_main_displayed;

/* true if the last operation on the Main Screen was a flip; false if it was
 * an update. This is tracked so we can know if '_video_main_current' should
 * match '_video_main_displayed' (previous operation was an update) or if it
 * should NOT match it (previous operation was a flip). */
static bool _video_last_was_flip;

bool _video_swap;

enum DS_Screen _video_backlights;

volatile uint8_t _video_main_busy[MAIN_BUFFER_COUNT];

volatile uint8_t _video_sub_busy;

volatile uint32_t _vblank_count;

struct _video_entry {
	uint16_t* src;
	volatile uint8_t* busy;
	uint16_t pixel_offset;
	uint16_t pixel_count;
	uint8_t buffer;
	enum DS_Engine engine;
};

static struct _video_entry _video_send_queue[MAIN_BUFFER_COUNT + 1];

static size_t _video_send_queue_count;

static int video_enqueue(enum DS_Engine engine, size_t start_y, size_t end_y, bool flip)
{
	volatile uint8_t* busy;
	uint16_t* src;

	if (start_y == end_y)
		return 0;
	if ((engine != DS_ENGINE_MAIN && engine != DS_ENGINE_SUB)
	 || (engine == DS_ENGINE_SUB && flip)
	 || start_y >= DS_SCREEN_HEIGHT || end_y > DS_SCREEN_HEIGHT || start_y > end_y) {
		return EINVAL;
	}

	if (engine == DS_ENGINE_MAIN) {
		busy = &_video_main_busy[_video_main_current];
		/* Wait for any transfer of this very screen to the Nintendo DS to end. */
		DS2_StartAwait();
		while (*busy != 0)
			DS2_AwaitInterrupt();
		DS2_StopAwait();
		src = _video_main[_video_main_current];

		/* If we're using multiple buffering and the Nintendo DS is still
		 * displaying the screen we're about to send, wait until the DS flips
		 * to another screen. We don't want to send new data to the screen
		 * that's currently displayed, because it will appear immediately and
		 * tear the screen!
		 * However, if the previous operation was NOT a flip, the Nintendo DS
		 * will obviously be displaying the buffer we want to flip to... */
		if (flip && _video_last_was_flip) {
			DS2_StartAwait();
			while (_video_main_displayed == _video_main_current)
				DS2_AwaitInterrupt();
			DS2_StopAwait();
		}
		/* If we're starting to update the same screen after the application
		 * used multiple buffering, wait until the Nintendo DS displays the
		 * buffer before the one we're about to update. That way, we're not
		 * updating a hidden buffer for the next 2 VBlanks. */
		else if (!flip && _video_last_was_flip) {
			DS2_StartAwait();
			while ((_video_main_displayed + 1) % MAIN_BUFFER_COUNT != _video_main_current)
				DS2_AwaitInterrupt();
			DS2_StopAwait();
		}
	} else {
		busy = &_video_sub_busy;
		/* Wait for any transfer of this very screen to the Nintendo DS to end. */
		DS2_StartAwait();
		while (*busy != 0)
			DS2_AwaitInterrupt();
		DS2_StopAwait();
		src = _video_sub;
	}

	{
		uint32_t section = DS2_EnterCriticalSection();
		struct _video_entry* tail = &_video_send_queue[_video_send_queue_count];

		tail->src = src + DS_SCREEN_WIDTH * start_y;
		tail->engine = engine;
		tail->buffer = (engine == DS_ENGINE_MAIN) ? _video_main_current : 0;
		tail->pixel_offset = DS_SCREEN_WIDTH * start_y;
		tail->pixel_count = DS_SCREEN_WIDTH * (end_y - start_y);
		tail->busy = busy;

		_video_send_queue_count++;
		*busy = 1;

		if (flip)
			_video_main_current = (_video_main_current + 1) % MAIN_BUFFER_COUNT;
		if (engine == DS_ENGINE_MAIN)
			_video_last_was_flip = flip;

		_add_pending_send(PENDING_SEND_VIDEO);
		DS2_LeaveCriticalSection(section);
	}

	return 0;
}

void _video_dequeue(void)
{
	struct _video_entry* head = &_video_send_queue[0];
	size_t result;

	result = _video_encoding_0(head->src, head->engine, head->buffer, head->pixel_offset, head->pixel_count);

	head->src += result;
	head->pixel_offset += result;
	head->pixel_count -= result;

	if (head->pixel_count == 0) {
		size_t i;
		*head->busy = 0;
		for (i = 1; i < _video_send_queue_count; i++) {
			_video_send_queue[i - 1] = _video_send_queue[i];
		}
		_video_send_queue_count--;
	}

	if (_video_send_queue_count > 0) {
		_add_pending_send(PENDING_SEND_VIDEO);
	}
}

void _video_displayed(uint_fast8_t index)
{
	_video_main_displayed = index;
}

int DS2_UpdateScreen(enum DS_Engine engine)
{
	return video_enqueue(engine, 0, DS_SCREEN_HEIGHT, false);
}

int DS2_FlipMainScreen(void)
{
	return video_enqueue(DS_ENGINE_MAIN, 0, DS_SCREEN_HEIGHT, true);
}

int DS2_UpdateScreenPart(enum DS_Engine engine, size_t start_y, size_t end_y)
{
	return video_enqueue(engine, start_y, end_y, false);
}

int DS2_FlipMainScreenPart(size_t start_y, size_t end_y)
{
	return video_enqueue(DS_ENGINE_MAIN, start_y, end_y, true);
}

int DS2_AwaitScreenUpdate(enum DS_Engine engine)
{
	if (engine & ~DS_ENGINE_BOTH)
		return EINVAL;

	if (engine & DS_ENGINE_MAIN) {
		DS2_StartAwait();
		while (_video_main_busy[_video_main_current] != 0)
			DS2_AwaitInterrupt();
		DS2_StopAwait();
	}

	if (engine & DS_ENGINE_SUB) {
		DS2_StartAwait();
		while (_video_sub_busy != 0)
			DS2_AwaitInterrupt();
		DS2_StopAwait();
	}

	return 0;
}

uint16_t* DS2_GetMainScreen(void)
{
	return _video_main[_video_main_current];
}

uint16_t* DS2_GetSubScreen(void)
{
	return _video_sub;
}

uint16_t* DS2_GetScreen(enum DS_Engine engine)
{
	return engine == DS_ENGINE_MAIN
		? _video_main[_video_main_current]
		: engine == DS_ENGINE_SUB
			? _video_sub
			: NULL;
}

enum DS2_PixelFormat DS2_GetPixelFormat(enum DS_Engine engine)
{
	return _video_pixel_formats[engine - 1];
}

int DS2_SetPixelFormat(enum DS_Engine engine, enum DS2_PixelFormat format)
{
	if ((format != DS2_PIXEL_FORMAT_BGR555 && format != DS2_PIXEL_FORMAT_RGB555)
	 || ((engine & ~DS_ENGINE_BOTH) != 0)) {
		return EINVAL;
	}

	if (engine & DS_ENGINE_MAIN)
		_video_pixel_formats[DS_ENGINE_MAIN - 1] = format;
	if (engine & DS_ENGINE_SUB)
		_video_pixel_formats[DS_ENGINE_SUB - 1] = format;
	return 0;
}

bool DS2_GetScreenSwap(void)
{
	return _video_swap;
}

enum DS_Screen DS2_GetScreenBacklights(void)
{
	return _video_backlights;
}

void DS2_AwaitVBlank(void)
{
	uint32_t saved_count = _vblank_count;
	DS2_StartAwait();
	while (_vblank_count == saved_count)
		DS2_AwaitInterrupt();
	DS2_StopAwait();
}
