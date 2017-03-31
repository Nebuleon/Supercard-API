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
    # Dispatch to the proper implementation.
    # The implementations are named according to the kind of work they do:
    # - b, w, c: byte, word, cache line;
    # - u, a: unaligned (source), aligned (both);
    # - p, s: prefix, suffix.
    # - a_b: 'b' enabling the use of 'a'.
    # All implementations expect to end at a3 (dst + n), an exclusive bound.
    # a) Copying fewer than 16 bytes. Avoid computing too much auxiliary stuff
    #    and just copy bytes.
    sltiu   t7, a2, 16
    or      v0, a0, zero               # memcpy returns argument 1
    bne     t7, zero, bs
    addu    a3, a0, a2                 # (delay slot) provide a3 to everyone

    # Word and cache line implementations have access to t0 and t1, the
    # misalignments relative to 4 bytes in a0 and a1.
    # b) Copying fewer than 256 bytes. We'll copy words, which means we will
    #    dispatch to a word copier, either aligned or unaligned.
    sltiu   t7, a2, 256
    andi    t0, a0, 3                  # t0 = misalignment in a0 (dst)
    bne     t7, zero, w_bp
    andi    t1, a1, 3                  # (delay slot) same for t1 and a1 (src)

    # c) Copying 256 bytes or more. We'll copy cache lines, which means we
    #    will dispatch to a cache line copier, either aligned or unaligned.
c_wp_bp:
    beq     t0, zero, c_wp             # no byte misalignment?
    li      t4, 32                     # (delay slot) t4 = cache line size
    li      t3, 4
    subu    t3, t3, t0                 # prefix bytes = 4 - misalignment (dst)
1:  lbu     t5, 0(a1)
    # Let the load complete in parallel with these.
    addiu   t3, t3, -1
    addiu   a0, a0, 1
    addiu   a1, a1, 1
    bne     t3, zero, 1b
    sb      t5, -1(a0)                 # (delay slot) store to 0(old a0)

c_wp:
    # Cache line implementations have access to t2, the misalignment relative
    # to cache lines in a0 (dst).
    bne     t0, t1, cu_wup             # misalignments mismatch
    andi    t2, a0, 0x1F               # (delay slot) cache line misalignment

ca_wap:
    subu    t3, t4, t2                 # prefix bytes = size - misalignment
    srl     t3, t3, 2                  # round down to words
    beq     t3, zero, ca               # no words to copy?
    srl     t8, a3, 5                  # (delay slot) [see ca for comments]
2:  lw      t5, 0(a1)
    # Let the load complete in parallel with these.
    addiu   t3, t3, -1
    addiu   a0, a0, 4
    addiu   a1, a1, 4
    bne     t3, zero, 2b
    sw      t5, -4(a0)                 # (delay slot) store to 0(old a0)

ca:
    # Store, in t8, the end of the portion we can store as whole cache lines,
    # so that we can find the end with BNE. (Part of this was done by ca_wap.)
    sll     t8, t8, 5

3:  pref    4, 0(a1)                   # LoadStreamed: expect this data not to
    pref    4, 28(a1)                  # be reused (+ 32 bytes)
    pref    30, 0(a0)                  # PrepareForStore: clear a cache line
                                       # without loading data from RAM for it
    lw      t0, 0(a1)
    lw      t1, 4(a1)
    lw      t2, 8(a1)
    lw      t3, 12(a1)
    lw      t4, 16(a1)
    lw      t5, 20(a1)
    lw      t6, 24(a1)
    lw      t7, 28(a1)
    # Let the loads complete in parallel with these.
    addiu   a0, a0, 32
    addiu   a1, a1, 32
    sw      t0, -32(a0)
    sw      t1, -28(a0)
    sw      t2, -24(a0)
    sw      t3, -20(a0)
    sw      t4, -16(a0)
    sw      t5, -12(a0)
    sw      t6, -8(a0)
    bne     a0, t8, 3b
    sw      t7, -4(a0)                 # (delay slot) store to 28(old a0)

was:
    # Store, in t8, the end of the portion we can store as whole words, so
    # that we can find the end with BNE.
    srl     t8, a3, 2
    sll     t8, t8, 2
    beq     a0, t8, bs                 # we can't copy any words?
    nop                                # cannot delay usefully here
4:  lw      t5, 0(a1)
    # Let the load complete in parallel with these.
    addiu   a0, a0, 4
    addiu   a1, a1, 4
    bne     a0, t8, 4b
    sw      t5, -4(a0)                 # (delay slot) store to 0(old a0)

    b       bs                         # check for any byte suffix
    nop                                # cannot delay usefully here

cu_wup:
    subu    t3, t4, t2                 # prefix bytes = 32 - misalignment
    srl     t3, t3, 2                  # round down to words
    beq     t3, zero, cu               # no words to copy?
    srl     t8, a3, 5                  # (delay slot) [see cu for comments]
5:  ulw     t5, 0(a1)
    # Let the load complete in parallel with these.
    addiu   t3, t3, -1
    addiu   a0, a0, 4
    addiu   a1, a1, 4
    bne     t3, zero, 5b
    sw      t5, -4(a0)                 # (delay slot) store to 0(old a0)

cu:
    # Store, in t8, the end of the portion we can store as whole cache lines,
    # so that we can find the end with BNE. (Part of this was done by cu_wup.)
    sll     t8, t8, 5

6:  pref    4, 0(a1)                   # LoadStreamed: expect this data not to
    pref    4, 28(a1)                  # be reused (+ 32 bytes)
    pref    30, 0(a0)                  # PrepareForStore: clear a cache line
                                       # without loading data from RAM for it
    ulw     t0, 0(a1)
    ulw     t1, 4(a1)
    ulw     t2, 8(a1)
    ulw     t3, 12(a1)
    ulw     t4, 16(a1)
    ulw     t5, 20(a1)
    ulw     t6, 24(a1)
    ulw     t7, 28(a1)
    # Let the loads complete in parallel with these.
    addiu   a0, a0, 32
    addiu   a1, a1, 32
    sw      t0, -32(a0)
    sw      t1, -28(a0)
    sw      t2, -24(a0)
    sw      t3, -20(a0)
    sw      t4, -16(a0)
    sw      t5, -12(a0)
    sw      t6, -8(a0)
    bne     a0, t8, 6b
    sw      t7, -4(a0)                 # (delay slot) store to 28(old a0)

wus:
    # Store, in t8, the end of the portion we can store as whole words, so
    # that we can find the end with BNE.
    srl     t8, a3, 2
    sll     t8, t8, 2
    beq     a0, t8, bs                 # we can't copy any words?
    nop                                # cannot delay usefully here
7:  ulw     t5, 0(a1)
    # Let the load complete in parallel with these.
    addiu   a0, a0, 4
    addiu   a1, a1, 4
    bne     a0, t8, 7b
    sw      t5, -4(a0)                 # (delay slot) store to 0(old a0)

    b       bs                         # check for any byte suffix
    nop                                # cannot delay usefully here

w_bp:
    beq     t0, zero, w                # no byte misalignment?
    srl     t8, a3, 2                  # (delay slot) [see w for comments]
    li      t3, 4
    subu    t3, t3, t0                 # prefix bytes = 4 - misalignment
8:  lbu     t5, 0(a1)
    # Let the load complete in parallel with these.
    addiu   t3, t3, -1
    addiu   a0, a0, 1
    addiu   a1, a1, 1
    bne     t3, zero, 8b
    sb      t5, -1(a0)                 # (delay slot) store to 0(old a0)

w:
    # Store, in t8, the end of the portion we can store as words, so that we
    # can find the end with BNE. (Part of this was done by w_bp.)
    beq     t0, t1, wa
    sll     t8, t8, 2                  # (delay slot)

wu:
    ulw     t5, 0(a1)
    # Let the load complete in parallel with these.
    addiu   a0, a0, 4
    addiu   a1, a1, 4
    bne     a0, t8, wu
    sw      t5, -4(a0)                 # (delay slot) store to 0(old a0)

    b       bs                         # check for any byte suffix
    nop                                # cannot delay usefully here

wa:
    lw      t5, 0(a1)
    # Let the load complete in parallel with these.
    addiu   a0, a0, 4
    addiu   a1, a1, 4
    bne     a0, t8, wa
    sw      t5, -4(a0)                 # (delay slot) store to 0(old a0)
    # Fall through to bs.

bs:
    beq     a0, a3, end
    nop                                # cannot delay usefully here
9:  lbu     t2, 0(a1)
    # Let the load complete in parallel with these.
    addiu   a0, a0, 1
    addiu   a1, a1, 1
    bne     a0, a3, 9b
    sb      t2, -1(a0)                 # (delay slot) store to 0(old a0)

end:
    jr      ra
    nop                                # cannot delay usefully here

    .end     memcpy
