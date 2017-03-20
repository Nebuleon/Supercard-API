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

    .ent     strrchr
    .global  strrchr
    .type    strrchr,@function

    /* char* strrchr(const char* s, int ch)
     * Searches for the last occurrence of 'ch' in 's' up to, and including,
     * its terminating zero byte. This allows for 'ch' to be 0 in order to
     * find the end of the string.
     *
     * In:
     *   argument 1: Pointer to the string to search for argument 2 in.
     *   argument 2: The byte to search for.
     * Input assumptions (not checked):
     * - Bytes are mapped and readable at 's' until its first zero byte.
     * Returns:
     *   A pointer to the last byte equal to 'ch' (that is, the pointer to
     *   'src + i', with the greatest value of 'i' under the length of the
     *   string, which is equal to 'ch'), or NULL if there is no such byte.
     */
strrchr:
    or      v0, zero, zero             # starting out, we haven't found 'ch'
    andi    a1, a1, 0xFF               # cast ch to unsigned int

1:  lbu     t0, 0(a0)
    beq     t0, a1, found              # found a byte equal to 'ch'?
    addiu   a0, a0, 1                  # (delay slot) advance the pointer
2:  bne     t0, zero, 1b               # not the zero byte yet: goto 1:
    nop                                # cannot delay usefully here

    jr      ra
    nop                                # cannot delay usefully here

found:
    # We're trying to find the last occurrence, not the first. Set this one as
    # the possible last one, but go back to the loop to find more (or the zero
    # byte).
    b       2b
    # Undo the work of the branch delay slot that brought us here (in another
    # branch delay slot).
    addiu   v0, a0, -1

    .end     strrchr
