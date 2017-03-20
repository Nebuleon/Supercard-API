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

    .ent     memset
    .global  memset
    .type    memset,@function

    /* void* memset(void* dst, int value, size_t n)
     * Sets 'n' bytes at 'dst' to 'value'.
     *
     * In:
     *   argument 1: The destination pointer.
     *   argument 2: The byte value to fill the region with.
     *   argument 3: The number of bytes to set.
     * Input assumptions (not checked):
     * - 'n' bytes are mapped and writable at 'dst'.
     * Returns:
     *   Argument 1.
     */
memset:
    # If we're asked to set fewer than 32 bytes, it's more efficient to just
    # set bytes than to compute word setting, prefix misalignment and the
    # size of the suffix.
    or      v0, a0, zero
    sltiu   t0, a2, 32
    bne     t0, zero, suffix
    addu    a3, a0, a2                 # (delay slot) a3 = end (exclusive bound)

    andi    a1, a1, 0xFF
    sll     t0, a1, 8
    or      a1, a1, t0                 # a1 is now 2 copies of the byte
    sll     t0, a1, 16

    # Here, it's 32 bytes or more. Is it 512 bytes or more, too?
    sltiu   t1, a2, 512
    beq     t1, zero, 5f               # yes: go to the cache-line version
    or      a1, a1, t0                 # (delay slot) ... and now 4 copies

    andi    t0, a0, 0x3                # t0 = misalignment in 'dst'
    beq     t0, zero, words
    subu    t0, a0, t0                 # (delay slot) delete the misalignment
    addiu   t0, t0, 4                  # add 4; that's where the prefix ends

1:  addiu   a0, a0, 1
    bne     a0, t0, 1b
    sb      a1, -1(a0)                 # (delay slot) store to 0(old a0)

words:
    srl     t0, a3, 2                  # store in t0 the end of aligned words,
    sll     t0, t0, 2                  # so we can find the end with BNE

2:  addiu   a0, a0, 4
    bne     a0, t0, 2b
    sw      a1, -4(a0)                 # (delay slot) store to 0(old a0)

suffix:
    beq     a0, a3, 4f
    nop                                # cannot delay usefully here
3:  addiu   a0, a0, 1
    bne     a0, a3, 3b
    sb      a1, -1(a0)                 # (delay slot) store to 0(old a0)

4:  jr      ra
    nop

5:  # Now prepare to memset entire cache lines. The misalignment is calculated
    # relative to cache lines here.
    andi    t0, a0, 0x1F               # t0 = misalignment in 'dst'
    beq     t0, zero, lines
    subu    t0, a0, t0                 # (delay slot) delete the misalignment
    addiu   t0, t0, 32                 # add this; that's where the prefix ends

6:  addiu   a0, a0, 1
    bne     a0, t0, 6b
    sb      a1, -1(a0)                 # (delay slot) store to 0(old a0)

lines:
    srl     t0, a3, 5                  # store in t0 the end of cache lines,
    sll     t0, t0, 5                  # so we can find the end with BNE

7:  pref    30, 0(a0)                  # PrepareForStore: clear a cache line
    addiu   a0, a0, 32                 # without loading data from RAM for it
    sw      a1, -32(a0)
    sw      a1, -28(a0)
    sw      a1, -24(a0)
    sw      a1, -20(a0)
    sw      a1, -16(a0)
    sw      a1, -12(a0)
    sw      a1, -8(a0)
    bne     a0, t0, 7b
    sw      a1, -4(a0)                 # (delay slot) store to 28(old a0)

    b       suffix
    nop                                # cannot delay usefully here

    .end     memset
