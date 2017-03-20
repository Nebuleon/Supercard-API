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

#ifndef STRUCTS_H
#define STRUCTS_H

/* This must be synchronised with the MIPS side's ds2/ds.h. */

/* - - - START SHARED PART - - - */
#if !defined __ASSEMBLY__
#  include <stdint.h>
#endif

#define DS_BUTTON_A      (1 << 0)   //!< Face button A, 1 if pressed, 0 if released.
#define DS_BUTTON_B      (1 << 1)   //!< Face button B, 1 if pressed, 0 if released.
#define DS_BUTTON_SELECT (1 << 2)   //!< Select button, 1 if pressed, 0 if released.
#define DS_BUTTON_START  (1 << 3)   //!< Start button, 1 if pressed, 0 if released.
#define DS_BUTTON_RIGHT  (1 << 4)   //!< D-pad Right, 1 if pressed, 0 if released.
#define DS_BUTTON_LEFT   (1 << 5)   //!< D-pad Left, 1 if pressed, 0 if released.
#define DS_BUTTON_UP     (1 << 6)   //!< D-pad Up, 1 if pressed, 0 if released.
#define DS_BUTTON_DOWN   (1 << 7)   //!< D-pad Down, 1 if pressed, 0 if released.
#define DS_BUTTON_R      (1 << 8)   //!< Right Trigger, 1 if pressed, 0 if released.
#define DS_BUTTON_L      (1 << 9)   //!< Left Trigger, 1 if pressed, 0 if released.
#define DS_BUTTON_X      (1 << 10)  //!< Face button X, 1 if pressed, 0 if released.
#define DS_BUTTON_Y      (1 << 11)  //!< Face button Y, 1 if pressed, 0 if released.
#define DS_BUTTON_TOUCH  (1 << 12)  //!< Touchscreen, 1 if touched, 0 if not.
#define DS_BUTTON_LID    (1 << 13)  //!< DS lid, 1 if closed, 0 if open.

#if !defined __ASSEMBLY__
struct __attribute__((packed)) DS_RTC {
	uint8_t year;    //!< The year, between 2000 and 2099, expressed as 0-99.
	uint8_t month;   //!< The month, 1-12.
	uint8_t day;     //!< The day of the month, 1-28, 1-30, 1-31.
	uint8_t weekday; //!< The day of the week, 0 = Monday, 6 = Sunday.
	uint8_t hour;    //!< The hour of the day, 0-11 for AM, 40-51 for PM, 0-23 for 24-hour mode.
	uint8_t minute;  //!< The minute of the hour, 0-59.
	uint8_t second;  //!< The second of the minute, 0-59.
};

struct __attribute__((packed)) DS_InputState
{
	uint16_t buttons;  //!< Buttons being held. Bitfield of DS_BUTTON_*.
	uint8_t touch_x;   //!< Touch X location, meaningful if buttons & DS_BUTTON_TOUCH.
	uint8_t touch_y;   //!< Touch Y location, meaningful if buttons & DS_BUTTON_TOUCH.
};
#endif /* !__ASSEMBLY__ */
/* - - - END SHARED PART - - - */

#endif /* !STRUCTS_H */
