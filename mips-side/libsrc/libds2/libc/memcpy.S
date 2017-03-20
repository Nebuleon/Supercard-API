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

    .ent     memcpy
    .global  memcpy
    .type    memcpy,@function

    /* void* memcpy(void* dst, const void* src, size_t n)
     * Copies 'n' bytes from 'src' to 'dst'.
     *
     * In:
     *   argument 1: The destination pointer.
     *   argument 2: The source pointer.
     *   argument 3: The number of bytes to copy.
     * Input assumptions (not checked):
     * - 'n' bytes are mapped and readable at 'src'.
     * - 'n' bytes are mapped and writable at 'dst'.
     * - For any 'i' between 0 and 'n' exclusive, there is no 'j', also
     *   between 0 and 'num' exclusive, for which 'src + i == dst + j' (no
     *   overlap).
     * Returns:
     *   Argument 1.
     */
memcpy:
    # If we're asked to copy fewer than 16 bytes, it's more efficient to just
    # copy bytes than to compute the misalignment mismatch, prefix size and
    # suffix size.
    or      v0, a0, zero
    sltiu   t0, a2, 16
    bne     t0, zero, suffix
    addu    a3, a0, a2                 # (delay slot) a3 = end of dst (exclusive bound)

    andi    t0, a0, 3                  # t0 = misalignment in a0 (dst)
    andi    t1, a1, 3                  # t1 = misalignment in a1 (src)
    bne     t0, t1, suffix             # misalignments mismatch; just copy bytes
    nop                                # cannot delay usefully here

    # Misalignments match; we are certain to be able to copy words at some
    # point. If we're copying more than 256 bytes, go to the version that
    # copies entire cache lines.
    sltiu   t2, a2, 256
    beq     t2, zero, 5f
    nop                                # cannot delay usefully here

    beq     t0, zero, words
    subu    t1, a0, t1                 # (delay slot) delete the misalignment
    addiu   t1, t1, 4                  # add 4; that's where the prefix ends

1:  lbu     t2, 0(a1)
    # Let the load complete in parallel with these.
    addiu   a0, a0, 1
    addiu   a1, a1, 1
    bne     a0, t1, 1b
    sb      t2, -1(a0)                 # (delay slot) store to 0(old a0)

words:
    srl     t1, a3, 2                  # store in t1 the end of aligned words,
    sll     t1, t1, 2                  # so we can find the end with BNE

2:  lw      t2, 0(a1)
    # Let the load complete in parallel with these.
    addiu   a0, a0, 4
    addiu   a1, a1, 4
    bne     a0, t1, 2b
    sw      t2, -4(a0)                 # (delay slot) store to 0(old a0)

suffix:
    beq     a0, a3, 4f
    nop                                # cannot delay usefully here
3:  lbu     t2, 0(a1)
    # Let the load complete in parallel with these.
    addiu   a0, a0, 1
    addiu   a1, a1, 1
    bne     a0, a3, 3b
    sb      t2, -1(a0)                 # (delay slot) store to 0(old a0)

4:  jr      ra
    nop                                # cannot delay usefully here

5:  # Now prepare to memcpy entire cache lines. The misalignment is calculated
    # relative to cache lines here, and only on 'dst', because 'src' need only
    # be 4-byte-aligned when the cache-line copy loop starts.
    andi    t0, a0, 0x1F               # t0 = misalignment in 'dst'
    beq     t0, zero, lines
    subu    t0, a0, t0                 # (delay slot) delete the misalignment
    addiu   t0, t0, 32                 # add this; that's where the prefix ends

6:  lbu     t2, 0(a1)
    # Let the load complete in parallel with these.
    addiu   a0, a0, 1
    addiu   a1, a1, 1
    bne     a0, t0, 6b
    sb      t2, -1(a0)                 # (delay slot) store to 0(old a0)

lines:
    srl     t0, a3, 5                  # store in t0 the end of cache lines,
    sll     t0, t0, 5                  # so we can find the end with BNE

7:  pref    4, 0(a1)                   # LoadStreamed: expect this data not to
    pref    4, 28(a1)                  # be reused (+ 32 bytes)
    pref    30, 0(a0)                  # PrepareForStore: clear a cache line
                                       # without loading data from RAM for it
    lw      t1, 0(a1)
    lw      t2, 4(a1)
    lw      t3, 8(a1)
    lw      t4, 12(a1)
    lw      t5, 16(a1)
    lw      t6, 20(a1)
    lw      t7, 24(a1)
    lw      t8, 28(a1)
    # Let the loads complete in parallel with these.
    addiu   a0, a0, 32
    addiu   a1, a1, 32
    sw      t1, -32(a0)
    sw      t2, -28(a0)
    sw      t3, -24(a0)
    sw      t4, -20(a0)
    sw      t5, -16(a0)
    sw      t6, -12(a0)
    sw      t7, -8(a0)
    bne     a0, t0, 7b
    sw      t8, -4(a0)                 # (delay slot) store to 28(old a0)

    b       suffix
    nop                                # cannot delay usefully here

    .end     memcpy
