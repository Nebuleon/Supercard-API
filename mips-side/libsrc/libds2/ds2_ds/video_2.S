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

#include <mips.h>
#include <errno.h>
#include <ds2/ds.h>

    .text
    .set     noreorder

    .extern  _video_main
    .extern  _video_main_current 1
    .extern  _video_sub

    .ent     DS2_FillScreen
    .global  DS2_FillScreen
    .type    DS2_FillScreen,@function

    /* int DS2_FillScreen(enum DS_Engine engine, uint16_t color)
     * Sets the entirety of the current screen of the given Nintendo DS
     * display engine to the given color.
     *
     * In:
     *   argument 1: The Nintendo DS engine to fill the current screen of.
     *   argument 2: The color to fill the screen with.
     * Environment assumptions:
     * - _video_main and _video_sub are aligned to 32 bytes.
     * - The type of _video_main_current is uint8_t.
     * Returns:
     *   0 on success.
     *   EINVAL if 'engine' is neither DS_ENGINE_MAIN nor DS_ENGINE_SUB.
     */
DS2_FillScreen:
    li      t4, DS_ENGINE_MAIN
    beq     a0, t4, main_screen
    li      t5, DS_ENGINE_SUB          # (delay slot)
    beq     a0, t5, sub_screen
    nop                                # cannot delay usefully here

    # Here, argument 1 is invalid.
    .set     push
    .set     reorder
    li      v0, EINVAL
    jr      ra
    .set     pop

main_screen:
    lbu     t0, _video_main_current
    la      a0, _video_main
    li      t1, DS_SCREEN_WIDTH * DS_SCREEN_HEIGHT * 2
    multu   t1, t0
    mflo    t0
    b       common
    addu    a0, a0, t0                 # (delay slot) a0 = _video_main[_video_main_current]

sub_screen:
    la      a0, _video_sub

common:
    andi    a1, a1, 0xFFFF             # ensure arg 2 is 16-bit
    sll     t0, a1, 16
    or      a1, a1, t0                 # make a word full of argument 2
    li      t0, (DS_SCREEN_WIDTH * DS_SCREEN_HEIGHT * 2) / 32

1:  pref    30, 0(a0)                  # PrepareForStore: clear a cache line
    addiu   a0, a0, 32                 # without loading data from RAM for it
    addiu   t0, t0, -1
    sw      a1, -32(a0)
    sw      a1, -28(a0)
    sw      a1, -24(a0)
    sw      a1, -20(a0)
    sw      a1, -16(a0)
    sw      a1, -12(a0)
    sw      a1, -8(a0)
    bne     t0, zero, 1b
    sw      a1, -4(a0)                 # (delay slot) store to 28(old a0)

    jr      ra
    or      v0, zero, zero             # (delay slot) return success

    .end     DS2_FillScreen
