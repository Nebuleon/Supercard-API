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

    .extern  DS2_GetScreen

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
     * Returns:
     *   0 on success.
     *   EINVAL if 'engine' is neither DS_ENGINE_MAIN nor DS_ENGINE_SUB.
     */
DS2_FillScreen:
    addiu   sp, sp, -24
    sw      ra, 20(sp)
    sw      s0, 16(sp)
    jal     DS2_GetScreen
    or      s0, a1, zero               # (delay slot) grab 'color' in s0

    andi    a1, s0, 0xFFFF             # ensure 'color' is 16-bit
    lw      ra, 20(sp)                 # grab everything from the stack
    lw      s0, 16(sp)                 # since we'll trash the data cache soon

    beq     v0, zero, fail
    addiu   sp, sp, 24                 # (delay slot) adjust the stack
    sll     t0, a1, 16
    or      a1, a1, t0                 # make a word full of argument 2
    li      t0, (DS_SCREEN_WIDTH * DS_SCREEN_HEIGHT * 2) / 32

1:  pref    30, 0(v0)                  # PrepareForStore: clear a cache line
    addiu   v0, v0, 32                 # without loading data from RAM for it
    addiu   t0, t0, -1
    sw      a1, -32(v0)
    sw      a1, -28(v0)
    sw      a1, -24(v0)
    sw      a1, -20(v0)
    sw      a1, -16(v0)
    sw      a1, -12(v0)
    sw      a1, -8(v0)
    bne     t0, zero, 1b
    sw      a1, -4(v0)                 # (delay slot) store to 28(old v0)

    jr      ra
    or      v0, zero, zero             # (delay slot) return success

fail:
    .set     push
    .set     reorder
    li      v0, EINVAL
    jr      ra
    .set     pop

    .end     DS2_FillScreen
