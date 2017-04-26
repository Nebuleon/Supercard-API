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

#ifndef VIDEO_H
#define VIDEO_H

#include <nds.h>
#include <stdbool.h>
#include <stdint.h>

/* Main screen buffers. There are three, so page flipping can be used. */
extern DTCM_BSS uint16_t* video_main[3];

/* Sub screen buffer. Page flipping cannot be used. */
extern DTCM_BSS uint16_t* video_sub;

/* true if the Sub Screen is graphics; false if it's text. */
extern DTCM_BSS bool video_sub_graphics;

/* Index of the Main Screen buffer last set by set_main_buffer. */
extern uint8_t video_main_current;

/* Sets the currently-displayed Main Screen buffer.
 * In:
 *   buffer: 0 to 2.
 */
extern void set_main_buffer(uint8_t buffer);

/* Sets both screens to be displaying graphics. */
extern void video_init(void);

/* Sets the Sub Screen to be displaying graphics. No operation if the screen
 * was already displaying graphics. */
extern void set_sub_graphics(void);

/* Sets the Sub Screen to be displaying text. No operation if the screen
 * was already displaying text. */
extern void set_sub_text(void);

/* Called by the VBlank handler. Applies a pending flip operation on the Main
 * Screen, if there is one, and requests notification to the Supercard. */
extern void apply_pending_flip();

/* Displays the given Main Screen buffer at the next VBlank.
 *
 * In:
 *   buffer: The Main Screen buffer to be displayed at the next VBlank.
 */
extern void add_pending_flip(unsigned int buffer);

/* Called by the VBlank handler. Applies a pending screen swap operation, if
 * there is one. */
extern void apply_pending_swap();

/* Requests that the screens be swapped or unswapped at the next VBlank.
 * In:
 *   swap:
 *   - true puts the Main Screen on the top and the Sub Screen on the bottom.
 *   - false puts the Main Screen on the bottom and the Sub Screen on the top.
 */
extern void set_pending_swap(bool swap);

#endif /* !VIDEO_H */
