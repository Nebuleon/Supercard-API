/*
 * This file is part of the C standard library for the Supercard DSTwo.
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

#ifndef __DS2DS_H__
#define __DS2DS_H__

#if !defined __ASSEMBLY__
#  include <stdbool.h>
#  include <stddef.h>
#endif

/* Some of this must be synchronised with the ARM side's structs.h. */

#define DS_SCREEN_WIDTH  256
#define DS_SCREEN_HEIGHT 192

#define RGB555(r, g, b) (((r) << 10) | ((g) << 5) | ((b)))
#define BGR555(b, g, r) (((b) << 10) | ((g) << 5) | ((r)))

#define RGB555_R(rgb555) (((rgb555) >> 10) & 0x1F)
#define RGB555_G(rgb555) (((rgb555) >> 5) & 0x1F)
#define RGB555_B(rgb555) ((rgb555) & 0x1F)

#define BGR555_R(bgr555) ((bgr555) & 0x1F)
#define BGR555_G(bgr555) (((bgr555) >> 5) & 0x1F)
#define BGR555_B(bgr555) (((bgr555) >> 10) & 0x1F)

/* This enum describes the screens to be affected by a command, designating
 * them by engine name to properly deal with swapping.
 * Constraints: DS_ENGINE_MAIN and DS_ENGINE_SUB must have values with only
 * one bit set, and that bit must be either bit 0 or bit 1; the exact value
 * doesn't matter. DS_ENGINE_BOTH must be the bitwise OR of both values. */
#if !defined __ASSEMBLY__
enum DS_Engine {
	DS_ENGINE_MAIN = 1,
	DS_ENGINE_SUB  = 2,
	DS_ENGINE_BOTH = 3
};
#else
#  define DS_ENGINE_MAIN 1
#  define DS_ENGINE_SUB  2
#  define DS_ENGINE_BOTH 3
#endif

/* This enum describes the screens to be affected by a command, designating
 * them by screen position.
 * Constraints: DS_SCREEN_LOWER must be 1; DS_SCREEN_UPPER must be 2. Those
 * values are used verbatim in backlight setting commands on the DS.
 * DS_SCREEN_BOTH must be the bitwise OR of both values, which is 3. */
#if !defined __ASSEMBLY__
enum DS_Screen {
	DS_SCREEN_LOWER = 1,
	DS_SCREEN_UPPER = 2,
	DS_SCREEN_BOTH  = 3
};
#else
#  define DS_SCREEN_LOWER 1
#  define DS_SCREEN_UPPER 2
#  define DS_SCREEN_BOTH  3
#endif

/* This enum describes the pixel formats supported by the Supercard DSTwo.
 * One may be chosen for the two engines independently from each other. */
#if !defined __ASSEMBLY__
enum DS2_PixelFormat {
	DS2_PIXEL_FORMAT_BGR555,
	DS2_PIXEL_FORMAT_RGB555
};
#else
#  define DS2_PIXEL_FORMAT_BGR555 0
#  define DS2_PIXEL_FORMAT_RGB555 1
#endif

#define DS_SCREEN_COUNT 2

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

#if !defined __ASSEMBLY__
/*
 * Requests the use or avoidance of compression when sending video data of any
 * kind to the Nintendo DS.
 *
 * Depending on the compression used, it may apply during the next frame only,
 * or it may apply during the next packet.
 *
 * In:
 *   compress:
 *   - true to request compression that may reduce the transfer bandwidth (and
 *     therefore latency) of video data, but may fail and increase processing.
 *   - false to request avoiding compression. The transfer bandwidth / latency
 *     will be higher, but constant. This is the default.
 */
extern void DS2_UseVideoCompression(bool compress);

/* Sets the entirety of the current screen of the given Nintendo DS display
 * engine to the given color. Does not update or flip the screen.
 *
 * In:
 *   engine: The Nintendo DS engine to fill the current screen of.
 *   color: The color to fill the region with. If the pixel format used for
 *     screens of the given engine is BGR555, this value should be made by
 *     the BGR555 macro. Similarly for RGB555.
 * Returns:
 *   0 on success.
 *   EINVAL if 'engine' is neither DS_ENGINE_MAIN nor DS_ENGINE_SUB.
 */
extern int DS2_FillScreen(enum DS_Engine engine, uint16_t color);

/* Causes the current buffer of the given engine to be sent to the Nintendo DS
 * and displayed as soon as it's received. This may cause screen tearing.
 *
 * This call returns immediately and allows further use of the same buffer.
 * Writes into the buffer may be applied during the transfer to the Nintendo
 * DS, causing more screen tearing.
 *
 * If any part of the current buffer of the given engine is already being
 * sent to the Nintendo DS, this function will first wait for the operation
 * to be done. Additionally, if frames are queued to be displayed, they are
 * displayed first.
 *
 * In:
 *   engine: The Nintendo DS engine to send the screen for. May not be
 *     DS_ENGINE_BOTH.
 * Returns:
 *   0 on success.
 *   EINVAL if the engine is invalid.
 */
extern int DS2_UpdateScreen(enum DS_Engine engine);

/* Causes the current Main Screen buffer to be sent to the Nintendo DS and
 * displayed at the next VBlank, avoiding screen tearing. Another Main
 * Screen buffer is then made the current buffer, which the Supercard will
 * write into.
 *
 * If any part of the buffer used before the flip is still being sent to the
 * Nintendo DS, or the DS did not yet switch to another buffer so that sends
 * can be hidden, this function will first wait for everything to be done.
 *
 * Returns:
 *   0.
 */
extern int DS2_FlipMainScreen(void);

/* Causes part of the current buffer of the given engine to be sent to the
 * Nintendo DS and displayed as soon as it's received. This may cause screen
 * tearing.
 *
 * This call returns immediately and allows further use of the same buffer.
 * Writes into the buffer may be applied during the transfer to the Nintendo
 * DS, causing more screen tearing.
 *
 * If any part of the current buffer of the given engine is already being
 * sent to the Nintendo DS, this function will first wait for the operation
 * to be done. Additionally, if frames are queued to be displayed, they are
 * displayed first.
 *
 * In:
 *   engine: The Nintendo DS engine to send the screen for. May not be
 *     DS_ENGINE_BOTH.
 *   start_y: The first row of the screen to be sent.
 *   end_y: One past the last row of the screen to be sent.
 * Returns:
 *   0 on success.
 *   EINVAL: the engine is invalid, start_y or end_y is out of range,
 *   start_y > end_y.
 */
extern int DS2_UpdateScreenPart(enum DS_Engine engine, size_t start_y, size_t end_y);

/* Causes part of the current Main Screen buffer to be sent to the Nintendo
 * DS and displayed at the next VBlank, avoiding screen tearing. Another Main
 * Screen buffer is then made the current buffer, which the Supercard will
 * write into.
 *
 * The part of the screen that does not get updated will keep its contents
 * from 3 flips ago or the last update that affected the current buffer.
 * To preserve a border at the top and bottom of the screen, please ensure
 * that you update or flip the screen 3 times before doing partial updates.
 *
 * If any part of the buffer used before the flip is still being sent to the
 * Nintendo DS, or the DS did not yet switch to another buffer so that sends
 * can be hidden, this function will first wait for everything to be done.
 *
 * In:
 *   start_y: The first row of the buffer to be sent.
 *   end_y: One past the last row of the buffer to be sent.
 * Returns:
 *   0 on success.
 *   EINVAL: start_y or end_y is out of range, start_y > end_y.
 */
extern int DS2_FlipMainScreenPart(size_t start_y, size_t end_y);

/*
 * Waits until the Supercard is done sending data from the current screen of
 * the given Nintendo DS engine(s).
 *
 * In:
 *   engine: The Nintendo DS engine to wait for. Can be DS_ENGINE_BOTH.
 * Returns:
 *   0 on success.
 *   EINVAL: engine is out of range.
 */
extern int DS2_AwaitScreenUpdate(enum DS_Engine engine);

/* Returns the address of the current Main Screen buffer. Following the
 * returned address, there are DS_SCREEN_WIDTH * DS_SCREEN_HEIGHT pixels, each
 * 16 bits wide.
 *
 * Because the Main Screen may be multiple buffered, the returned address is
 * subject to change after calls to DS2_FlipMainScreen.
 */
extern uint16_t* DS2_GetMainScreen(void);

/* Returns the address of the Sub Screen buffer. Following the returned
 * address, there are DS_SCREEN_WIDTH * DS_SCREEN_HEIGHT pixels, each 16 bits
 * wide.
 *
 * Because the Sub Screen is single-buffered, the returned address is
 * guaranteed not to change during the course of program execution.
 */
extern uint16_t* DS2_GetSubScreen(void);

/* Returns the address of the current buffer of the given DS engine. Following
 * the returned address, if the value of 'engine' is valid, there are
 * DS_SCREEN_WIDTH * DS_SCREEN_HEIGHT pixels, each 16 bits wide.
 *
 * In:
 *   engine: The engine to get the address of the current buffer for.
 * Returns:
 *   If engine == DS_ENGINE_MAIN: the same value as DS2_GetMainScreen().
 *   If engine == DS_ENGINE_SUB: the same value as DS2_GetSubScreen().
 *   Otherwise: NULL.
 */
extern uint16_t* DS2_GetScreen(enum DS_Engine engine);

/* Gets the pixel format in use on the given Nintendo DS engine.
 *
 * In:
 *   engine: The Nintendo DS engine to get the pixel format for. May not be
 *     DS_ENGINE_BOTH. Undefined Behavior results if this is an invalid value.
 * Returns:
 *   The pixel format in use on the given Nintendo DS engine.
 */
extern enum DS2_PixelFormat DS2_GetPixelFormat(enum DS_Engine engine);

/* Sets the pixel format in use on the given Nintendo DS engine.
 *
 * In:
 *   engine: The Nintendo DS engine to set the pixel format for. If this is
 *     DS_ENGINE_MAIN, all Main Screen buffers are affected.
 *     DS_ENGINE_BOTH is allowed, and if used, sets the pixel format on both
 *     engines.
 *   format: The new pixel format to set.
 * Returns:
 *   EINVAL if the engine or pixel format are not valid. 0 on success.
 */
extern int DS2_SetPixelFormat(enum DS_Engine engine, enum DS2_PixelFormat format);

/* Returns the current state of screen swapping on the Nintendo DS.
 *
 * The last value set by DS2_SetScreenSwap, or the default of false, is
 * returned even if it's not currently applied on the Nintendo DS.
 *
 * Returns:
 * - true: The Main Screen is on the bottom; the Sub Screen is on the top.
 * - false: The Main Screen is on the top; the Sub Screen is on the bottom.
 */
extern bool DS2_GetScreenSwap(void);

/* Sets the state of screen swapping on the Nintendo DS.
 *
 * The new state will be applied in the next vertical blanking period.
 *
 * In:
 *   swap:
 *   - true: The Main Screen is on the bottom; the Sub Screen is on the top.
 *   - false: The Main Screen is on the top; the Sub Screen is on the bottom.
 */
extern void DS2_SetScreenSwap(bool swap);

/* Returns the current state of screen backlights on the Nintendo DS.
 *
 * The last value set by DS2_SetScreenBacklights, or the default of
 * DS_SCREEN_BOTH, is returned even if it's not currently applied on the
 * Nintendo DS.
 *
 * Returns:
 * - DS_SCREEN_UPPER if the Upper Screen's backlight is on.
 * - DS_SCREEN_LOWER if the Lower Screen's backlight is on.
 * - DS_SCREEN_BOTH if both screens' backlights are on.
 */
extern enum DS_Screen DS2_GetScreenBacklights(void);

/* Sets the state of screen backlights on the Nintendo DS.
 *
 * The new state will be applied in the next vertical blanking period.
 *
 * In:
 *   screens:
 *   - DS_SCREEN_UPPER if the Upper Screen's backlight should be on.
 *   - DS_SCREEN_LOWER if the Lower Screen's backlight should be on.
 *   - DS_SCREEN_BOTH if both screens' backlights should be on.
 */
extern int DS2_SetScreenBacklights(enum DS_Screen screens);

/* Waits until vertical blanking occurs on the Nintendo DS, with power saving.
 *
 * Note that there is insufficient time during vertical blanking to send a new
 * image to the Nintendo DS. The main purpose of this function is to allow the
 * Supercard to synchronise itself to the Nintendo DS.
 */
extern void DS2_AwaitVBlank(void);

/* Starts an audio stream with the specified parameters.
 *
 * May be called if audio is already started, in which case the prior audio
 * stream is stopped, its buffer is emptied and a new one is started. If an
 * audio stream was started and needed to be stopped, and starting the new
 * one fails, audio will remain stopped.
 *
 * In:
 *   frequency: The audio sampling rate, in Hertz.
 *   buffer_size: The maximum number of samples that can be held by the
 *     Supercard before being sent to the Nintendo DS. Also controls the size
 *     of a similar buffer on the Nintendo DS.
 *   is_16bit:
 *   - true: Data is 16-bit little-endian signed PCM.
 *   - false: Data is 8-bit unsigned PCM.
 *   is_stereo:
 *   - true: There are 2 pieces of data (each 16-bit or 8-bit) for a sample,
 *     interleaved so the left channel appears before the right channel:
 *     [int16_t left] [int16_t right] [int16_t left] [int16_t right] ... or
 *     [uint8_t left] [uint8_t right] [uint8_t left] [uint8_t right] ...
 *   - false: There is 1 piece of data, 16-bit or 8-bit, for a sample.
 *
 * Returns:
 *   0 on success.
 *   ENOMEM if the required audio buffer cannot be allocated.
 */
extern int DS2_StartAudio(uint16_t frequency, uint16_t buffer_size, bool is_16bit, bool is_stereo);

/* Returns the number of audio samples that may be submitted using
 * DS2_SubmitAudio without first waiting for the Nintendo DS to consume the
 * oldest samples.
 *
 * If audio is not started, returns 0.
 */
extern size_t DS2_GetFreeAudioSamples();

/* Submits audio to the active stream.
 *
 * If more samples are submitted than are available in the buffer, this
 * function will repeatedly send some samples, wait for the Nintendo DS
 * to consume the oldest ones, and so on. To avoid this wait, check the
 * return value of DS2_GetFreeAudioSamples.
 *
 * In:
 *   data: Pointer to the sample data to be submitted. The data starts with
 *     the oldest sample and ends with the newest; each sample is 1, 2 or 4
 *     bytes, depending on the active sample format.
 *   n: Number of samples to be submitted.
 * Input assumptions (not checked):
 *   - (n * sample size) bytes are mapped and readable at 'data'.
 * Returns:
 *   0 on success.
 *   EFAULT if audio is not started.
 */
extern int DS2_SubmitAudio(const void* data, size_t n);

/* Stops the last started audio stream.
 *
 * Any samples that were going to be sent are cancelled; as of receiving the
 * request to stop audio, the Nintendo DS does not play any more samples that
 * were already sent.
 *
 * May be called if audio is already stopped, in which case nothing happens.
 */
extern void DS2_StopAudio(void);

/* Retrieves the current state of Nintendo DS input.
 *
 * Out:
 *   input_state: A pointer to a structure that gets updated with the current
 *     state of Nintendo DS input.
 */
extern void DS2_GetInputState(struct DS_InputState* input_state);

/* Retrieves the state of Nintendo DS input when it changes, with power saving
 * while it's still the same.
 *
 * Out:
 *   input_state: A pointer to a structure that gets updated with the new
 *     state of Nintendo DS input.
 */
extern void DS2_AwaitInputChange(struct DS_InputState* input_state);

/* Waits until all of the buttons in the argument are pressed on the Nintendo
 * DS, with power saving.
 *
 * In:
 *   buttons: A bitmask of the buttons to require to be pressed.
 */
extern void DS2_AwaitAllButtonsIn(uint16_t buttons);

/* Waits until any of the buttons in the argument is pressed on the Nintendo
 * DS, with power saving.
 *
 * In:
 *   buttons: A bitmask of the buttons to require at least one of to be
 *     pressed.
 */
extern void DS2_AwaitAnyButtonsIn(uint16_t buttons);

/* Waits until any of the buttons in the argument is released on the Nintendo
 * DS, with power saving.
 *
 * In:
 *   buttons: A bitmask of the buttons to require at least one of to be
 *     released.
 */
extern void DS2_AwaitNotAllButtonsIn(uint16_t buttons);

/* Waits until all of the buttons in the argument are released on the Nintendo
 * DS, with power saving.
 *
 * In:
 *   buttons: A bitmask of the buttons to require to be released.
 */
extern void DS2_AwaitNoButtonsIn(uint16_t buttons);

/* Waits until any of the buttons on the Nintendo DS is pressed, with power
 * saving.
 */
extern void DS2_AwaitAnyButtons(void);

/* Waits until all of the buttons on the Nintendo DS are released, with power
 * saving.
 */
extern void DS2_AwaitNoButtons(void);

/* Given an old input state and a new input state, returns the buttons that
 * are pressed in the new input state and released in the old input state. */
extern uint16_t DS2_GetNewlyPressed(const struct DS_InputState* old_state, const struct DS_InputState* new_state);

/* Given an old input state and a new input state, returns the buttons that
 * are released in the new input state and pressed in the old input state. */
extern uint16_t DS2_GetNewlyReleased(const struct DS_InputState* old_state, const struct DS_InputState* new_state);

#endif /* !__ASSEMBLY__ */

#endif /* !__DS2DS_H__ */
