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

#define MAIN_BUFFER_COUNT 3

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

/* Pixel formats for each engine. Indexed by 'enum DS2_Engine' - 1. */
extern enum DS2_PixelFormat _video_pixel_formats[2];

/* Contains the number of the Main Screen buffer being written into by the
 * Supercard. */
extern uint8_t _video_main_current;

/* Contains the number of the Main Screen buffer being displayed by the
 * Nintendo DS.
 * volatile because it can be modified by the card command interrupt handler,
 * and it's then tested in a loop to see if more data can be submitted. */
extern volatile uint8_t _video_main_displayed;

/* true if the Main Screen is on the top and the Sub Screen is on the bottom.
 * false if the Main Screen is on the bottom and the Sub Screen is on the top.
 */
extern bool _video_swap;

/* Contains bits described in 'enum DS_Screen' detailing which backlights are
 * on. */
extern enum DS_Screen _video_backlights;

/* Contains an entry for each Main Screen buffer stating whether it's being
 * sent.
 * volatile because it can be modified by the card command interrupt handler,
 * and it's then tested in a loop to see if more data can be submitted.
 * Reading all elements at once must be done in a critical section or with
 * interrupts disabled to prevent reads from getting interrupted. */
extern volatile uint8_t _video_main_busy[MAIN_BUFFER_COUNT];

/* 1 if the Sub Screen buffer is being transferred to the Nintendo DS,
 * or 0 if it is free to use.
 * volatile because it can be modified by the card command interrupt handler,
 * and it's then tested in a loop to see if more data can be submitted. */
extern volatile uint8_t _video_sub_busy;

/* Number of vertical blanking interrupts (VBlank) encountered by the Nintendo
 * DS. Overflows every 414.252 days.
 * volatile because it can be modified by the card command interrupt handler,
 * and it's then tested for inequality in a loop to await the next VBlank. */
volatile uint32_t _vblank_count;

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
	enum DS2_PixelFormat format = _video_pixel_formats[engine - 1];
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
