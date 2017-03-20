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

    .ent     strlen
    .global  strlen
    .type    strlen,@function

    /* size_t strlen(const char* s)
     * Returns the length of the string at 's'.
     *
     * In:
     *   argument 1: String to be examined.
     * Input assumptions (not checked):
     * - Bytes are mapped and readable at 's' until its first zero byte.
     * Returns:
     *   The number of bytes before the first zero byte in 's'.
     */
strlen:
    or      v0, zero, zero             # start at 0

1:  lbu     t0, 0(a0)
    addiu   a0, a0, 1
    bne     t0, zero, 1b               # not the zero byte yet: goto 1:
    addiu   v0, v0, 1                  # (delay slot) increment the result

    jr      ra
    # For performance reasons, the result was incremented in the delay slot,
    # even for the zero byte. Rewind it by 1.
    addiu   v0, v0, -1                 # (delay slot)

    .end     strlen
