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
#include <stdint.h>
#include <string.h>

#include "../intc.h"
#include "globals.h"

void _merge_input(const struct DS_InputState* new_state)
{
	/* Merge in any new button presses. Those are 0 in _input_state,
	 * 1 in new_state. */
	_input_presses.buttons |= (new_state->buttons & ~_input_state.buttons)
	/* Merge in any new button presses of buttons for which a release is still
	 * pending. Those are 1 in _input_state, 1 in new_state,
	 * 1 in _input_releases. */
		| (_input_state.buttons & new_state->buttons & _input_releases.buttons);
	/* Merge in any new button releases. Those are 1 in _input_state,
	 * 0 in new_state. */
	_input_releases.buttons |= (_input_state.buttons & ~new_state->buttons)
	/* Merge in any new button releases of buttons for which a press is still
	 * pending. Those are 0 in _input_state, 0 in new_state,
	 * 1 in _input_presses. */
		| (~_input_state.buttons & ~new_state->buttons & _input_presses.buttons);
	/* Update touchscreen input if DS_BUTTON_TOUCH is set. */
	if (new_state->buttons & DS_BUTTON_TOUCH) {
		_input_presses.touch_x = new_state->touch_x;
		_input_presses.touch_y = new_state->touch_y;
	}
}

void DS2_GetInputState(struct DS_InputState* input_state)
{
	uint32_t section = DS2_EnterCriticalSection();
	uint16_t presses = ~_input_state.buttons & _input_presses.buttons,
	         releases = _input_state.buttons & _input_releases.buttons;
	_input_state.buttons = (_input_state.buttons | presses) & ~releases;
	_input_state.touch_x = _input_presses.touch_x;
	_input_state.touch_y = _input_presses.touch_y;
	_input_presses.buttons &= ~presses;
	_input_releases.buttons &= ~releases;
	*input_state = _input_state;
	DS2_LeaveCriticalSection(section);
}

void DS2_AwaitInputChange(struct DS_InputState* input_state)
{
	struct DS_InputState old_input;
	DS2_GetInputState(&old_input);
	DS2_StartAwait();
	while (true) {
		DS2_GetInputState(input_state);
		if (memcmp(&old_input, input_state, sizeof(struct DS_InputState)) != 0)
			break;
		DS2_AwaitInterrupt();
	}
	DS2_StopAwait();
}

void DS2_AwaitAllButtonsIn(uint16_t buttons)
{
	struct DS_InputState input_state;
	DS2_StartAwait();
	while (true) {
		DS2_GetInputState(&input_state);
		if ((input_state.buttons & buttons) == buttons)
			break;
		DS2_AwaitInterrupt();
	}
	DS2_StopAwait();
}

void DS2_AwaitAnyButtonsIn(uint16_t buttons)
{
	struct DS_InputState input_state;
	DS2_StartAwait();
	while (true) {
		DS2_GetInputState(&input_state);
		if (input_state.buttons & buttons)
			break;
		DS2_AwaitInterrupt();
	}
	DS2_StopAwait();
	/* Ensure that the buttons that caused this call to succeed are all
	 * pressed in the next reading from user code. */
	uint32_t section = DS2_EnterCriticalSection();
	_input_state.buttons &= ~(input_state.buttons & buttons);
	_input_presses.buttons |= input_state.buttons & buttons;
	DS2_LeaveCriticalSection(section);
}

void DS2_AwaitNotAllButtonsIn(uint16_t buttons)
{
	struct DS_InputState input_state;
	DS2_StartAwait();
	while (true) {
		DS2_GetInputState(&input_state);
		if ((input_state.buttons & buttons) != buttons)
			break;
		DS2_AwaitInterrupt();
	}
	DS2_StopAwait();
	/* Ensure that the buttons that caused this call to succeed are all
	 * released in the next reading from user code. */
	uint32_t section = DS2_EnterCriticalSection();
	_input_releases.buttons |= ~input_state.buttons & buttons;
	_input_state.buttons |= buttons;
	DS2_LeaveCriticalSection(section);
}

void DS2_AwaitNoButtonsIn(uint16_t buttons)
{
	struct DS_InputState input_state;
	DS2_StartAwait();
	while (true) {
		DS2_GetInputState(&input_state);
		if (!(input_state.buttons & buttons))
			break;
		DS2_AwaitInterrupt();
	}
	DS2_StopAwait();
}

void DS2_AwaitAnyButtons(void)
{
	struct DS_InputState input_state;
	DS2_StartAwait();
	while (true) {
		DS2_GetInputState(&input_state);
		if (input_state.buttons)
			break;
		DS2_AwaitInterrupt();
	}
	DS2_StopAwait();
	/* Ensure that the buttons that caused this call to succeed are all
	 * pressed in the next reading from user code. */
	uint32_t section = DS2_EnterCriticalSection();
	_input_state.buttons &= ~input_state.buttons;
	_input_presses.buttons |= input_state.buttons;
	DS2_LeaveCriticalSection(section);
}

void DS2_AwaitNoButtons(void)
{
	struct DS_InputState input_state;
	DS2_StartAwait();
	while (true) {
		DS2_GetInputState(&input_state);
		if (!input_state.buttons)
			break;
		DS2_AwaitInterrupt();
	}
	DS2_StopAwait();
}

uint16_t DS2_GetNewlyPressed(const struct DS_InputState* old_state, const struct DS_InputState* new_state)
{
	return new_state->buttons & ~old_state->buttons;
}

uint16_t DS2_GetNewlyReleased(const struct DS_InputState* old_state, const struct DS_InputState* new_state)
{
	return old_state->buttons & ~new_state->buttons;
}
