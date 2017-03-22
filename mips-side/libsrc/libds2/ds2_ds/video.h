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

#ifndef __DS2_DS_VIDEO_H__
#define __DS2_DS_VIDEO_H__

#include <ds2/ds.h>
#include <stdbool.h>
#include <stdint.h>

#include "globals.h"

/* The buffers for the Main Screen to be sent to the Nintendo DS.
 *
 * The Main Screen supports page flipping with many buffers. This buffer is
 * selected by the first array index, video_main[n].
 * The second array index, video_main[][n], has the screen pixels laid out
 * so that a row of pixels is contiguous in memory. */
extern uint16_t _video_main[MAIN_BUFFER_COUNT][DS_SCREEN_WIDTH * DS_SCREEN_HEIGHT];

/* The buffer for the Sub Screen to be sent to the Nintendo DS.
 *
 * The Sub Screen doesn't support page flipping, so video_sub[n] has the
 * screen pixels laid out so that a row of pixels is contiguous in memory. */
extern uint16_t _video_sub[DS_SCREEN_WIDTH * DS_SCREEN_HEIGHT];

extern void _video_dequeue(void);

extern void _video_displayed(uint_fast8_t index);

/* Converts the given pixel, in the pixel format used on the given Nintendo DS
 * engine, to BGR 555 with the high bit set. This is a software implementation
 * of the tasks done by the FPGA after the _send_video_reply function requests
 * it. (See card_protocol.h.)
 *
 * In:
 *   pixel: The pixel value to convert.
 *   engine: The Nintendo DS engine to convert the pixel for.
 * Returns:
 *   The converted pixel value.
 */
static inline uint16_t _video_convert_bgr555(uint16_t pixel, enum DS_Engine engine)
{
	enum DS2_PixelFormat format = _ds2_ds.vid_formats[engine - 1];
	switch (format) {
		case DS2_PIXEL_FORMAT_BGR555:
			return 0x8000 | pixel;

		case DS2_PIXEL_FORMAT_RGB555:
			return 0x8000
			     | ((pixel & UINT16_C(0x7C00)) >> 10)
			     | ((pixel & UINT16_C(0x001F)) << 10)
			     |  (pixel & UINT16_C(0x03E0));

		default:
			return 0;
	}
}

#endif /* !__DS2_DS_VIDEO_H__ */
