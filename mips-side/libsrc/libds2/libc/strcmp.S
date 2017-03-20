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

    .ent     strcmp
    .global  strcmp
    .type    strcmp,@function

    /* int strcmp(const char* a, const char* b)
     * Compares the strings at 'a' and 'b'.
     *
     * In:
     *   arguments 1, 2: Strings to be compared.
     * Input assumptions (not checked):
     * - Bytes are mapped and readable at 'a' and 'b' until the first byte
     *   that compares unequal in both, or the first zero byte in either,
     *   whichever occurs first.
     * Returns:
     *   0 if both strings are equal (and so are their lengths).
     *   Less than 0 if the first unequal byte in both strings is lesser in
     *   'a'.
     *   Greater than 0 if the first unequal byte in both strings is greater
     *   in 'a'.
     */
strcmp:
1:  lbu     t0, 0(a0)
    lbu     t1, 0(a1)
    bne     t0, t1, unequal_byte       # found an unequal pair of bytes?
    addiu   a0, a0, 1                  # (delay slot) advance 'a'
    # Here, the bytes are equal. If one of the bytes is zero, then both are;
    # in that case, the strings are equal, and we fall through to return 0.
    bne     t0, zero, 1b               # both bytes are non-zero: goto 1:
    addiu   a1, a1, 1                  # (delay slot) advance 'b'

    jr      ra
    or      v0, zero, zero             # (delay slot) return 0

unequal_byte:
    jr      ra
    subu    v0, t0, t1                 # (delay slot) return (int) *a - (int) *b

    .end     strcmp
