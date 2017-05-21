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

#include <mips.h>

    .text
    .set     noreorder

    .ent     strcpy
    .global  strcpy
    .type    strcpy,@function

    /* char* strcpy(char* dst, const char* src)
     * Copies the string at 'src' to 'dst' up to, and including, the first
     * zero byte in 'src'.
     *
     * In:
     *   argument 1: Pointer to the buffer to be copied into.
     *   argument 2: The string to be copied.
     * Input assumptions (not checked):
     * - Bytes are mapped and readable at 'src' until its first zero byte.
     * - As many bytes as there are in 'src' until its first zero byte, plus
     *   1, are mapped at 'dst'.
     * Returns:
     *   Argument 1.
     */
strcpy:
    move    v0, a0                     # set the return value

1:  lbu     v1, 0(a1)
    # Let the load complete in parallel with these.
    addiu   a0, a0, 1
    addiu   a1, a1, 1
    bne     v1, zero, 1b               # not the zero byte yet: goto 1:
    sb      v1, -1(a0)                 # (delay slot) store to 0(old a0)

    jr      ra
    nop                                # cannot delay usefully here

    .end     strcpy
