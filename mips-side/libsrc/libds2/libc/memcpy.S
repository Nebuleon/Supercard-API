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

    # For word and cache line implementations:
    # - t0 = the misalignment relative to 4 bytes in a0;
    # - t1 = the misalignment relative to 4 bytes in a1;
    # - t2 = the end of the byte prefix before copying words;
    # - t4 = the mask to round an address down to a word.

    # b) Copying fewer than 256 bytes. We'll copy words, which means we will
    #    dispatch to a word copier, either aligned or unaligned.
    sltiu   t7, a2, 256
    addiu   t2, a0, 3
    li      t4, ~3
    and     t2, t2, t4
    andi    t0, a0, 3                  # t0 = misalignment in a0 (dst)
    bne     t7, zero, w_bp
    andi    t1, a1, 3                  # (delay slot) same for t1 and a1 (src)

    # c) Copying 256 bytes or more. We'll copy cache lines, which means we
    #    will dispatch to a cache line copier, either aligned or unaligned.
c_wp_bp:
    # Round 'dst' up to the next word to figure out where the byte prefix
    # ends.
    beq     a0, t2, c_wp               # no byte misalignment?
    addiu   t3, a0, 0x1F               # (delay slot)
1:  lbu     t5, 0(a1)
    # Let the load complete in parallel with these.
    addiu   a0, a0, 1
    addiu   a1, a1, 1
    bne     a0, t2, 1b
    sb      t5, -1(a0)                 # (delay slot) store to 0(old a0)

c_wp:
    # For cache line implementations:
    # - t3 = the end of the word prefix before copying cache lines;
    # - t9 = the mask to round an address down to a cache line.
    li      t9, ~0x1F
    bne     t0, t1, cu_wup             # misalignments mismatch
    and     t3, t3, t9                 # (delay slot)

ca_wap:
    beq     a0, t3, ca                 # no words to copy?
    # Store, in t8, the end of the portion we can store as whole cache lines,
    # so that we can find the end with BNE.
    and     t8, a3, t9                 # (delay slot)
2:  lw      t5, 0(a1)
    # Let the load complete in parallel with these.
    addiu   a0, a0, 4
    addiu   a1, a1, 4
    bne     a0, t3, 2b
    sw      t5, -4(a0)                 # (delay slot) store to 0(old a0)

ca:
    pref    4, 0(a1)                   # LoadStreamed: expect this data not to
    pref    4, 28(a1)                  # be reused (+ 32 bytes)
    pref    30, 0(a0)                  # PrepareForStore: clear a cache line
                                       # without loading data from RAM for it
    lw      v1, 0(a1)
    lw      t0, 4(a1)
    lw      t1, 8(a1)
    lw      t2, 12(a1)
    lw      t3, 16(a1)
    lw      t5, 20(a1)
    lw      t6, 24(a1)
    lw      t7, 28(a1)
    # Let the loads complete in parallel with these.
    addiu   a0, a0, 32
    addiu   a1, a1, 32
    sw      v1, -32(a0)
    sw      t0, -28(a0)
    sw      t1, -24(a0)
    sw      t2, -20(a0)
    sw      t3, -16(a0)
    sw      t5, -12(a0)
    sw      t6, -8(a0)
    bne     a0, t8, ca
    sw      t7, -4(a0)                 # (delay slot) store to 28(old a0)

was:
    # Store, in t8, the end of the portion we can store as whole words, so
    # that we can find the end with BNE.
    and     t8, a3, t4
    beq     a0, t8, bs                 # we can't copy any words?
    nop                                # cannot delay usefully here
3:  lw      t5, 0(a1)
    # Let the load complete in parallel with these.
    addiu   a0, a0, 4
    addiu   a1, a1, 4
    bne     a0, t8, 3b
    sw      t5, -4(a0)                 # (delay slot) store to 0(old a0)

    b       bs                         # check for any byte suffix
    nop                                # cannot delay usefully here

cu_wup:
    beq     a0, t3, cu                 # no words to copy?
    # Store, in t8, the end of the portion we can store as whole cache lines,
    # so that we can find the end with BNE.
    and     t8, a3, t9                 # (delay slot)
4:  ulw     t5, 0(a1)
    # Let the load complete in parallel with these.
    addiu   a0, a0, 4
    addiu   a1, a1, 4
    bne     a0, t3, 4b
    sw      t5, -4(a0)                 # (delay slot) store to 0(old a0)

cu:
    pref    4, 0(a1)                   # LoadStreamed: expect this data not to
    pref    4, 28(a1)                  # be reused (+ 32 bytes)
    pref    30, 0(a0)                  # PrepareForStore: clear a cache line
                                       # without loading data from RAM for it
    ulw     v1, 0(a1)
    ulw     t0, 4(a1)
    ulw     t1, 8(a1)
    ulw     t2, 12(a1)
    ulw     t3, 16(a1)
    ulw     t5, 20(a1)
    ulw     t6, 24(a1)
    ulw     t7, 28(a1)
    # Let the loads complete in parallel with these.
    addiu   a0, a0, 32
    addiu   a1, a1, 32
    sw      v1, -32(a0)
    sw      t0, -28(a0)
    sw      t1, -24(a0)
    sw      t2, -20(a0)
    sw      t3, -16(a0)
    sw      t5, -12(a0)
    sw      t6, -8(a0)
    bne     a0, t8, cu
    sw      t7, -4(a0)                 # (delay slot) store to 28(old a0)

wus:
    # Store, in t8, the end of the portion we can store as whole words, so
    # that we can find the end with BNE.
    and     t8, a3, t4
    beq     a0, t8, bs                 # we can't copy any words?
    nop                                # cannot delay usefully here
5:  ulw     t5, 0(a1)
    # Let the load complete in parallel with these.
    addiu   a0, a0, 4
    addiu   a1, a1, 4
    bne     a0, t8, 5b
    sw      t5, -4(a0)                 # (delay slot) store to 0(old a0)

    b       bs                         # check for any byte suffix
    nop                                # cannot delay usefully here

w_bp:
    beq     a0, t2, w                  # no byte misalignment?
    # Store, in t8, the end of the portion we can store as words, so that we
    # can find the end with BNE. (Part of this was done by w_bp.)
    and     t8, a3, t4                 # (delay slot)
6:  lbu     t5, 0(a1)
    # Let the load complete in parallel with these.
    addiu   a0, a0, 1
    addiu   a1, a1, 1
    bne     a0, t2, 6b
    sb      t5, -1(a0)                 # (delay slot) store to 0(old a0)

w:
    beq     t0, t1, wa
    nop                                # cannot delay usefully here

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
7:  lbu     t2, 0(a1)
    # Let the load complete in parallel with these.
    addiu   a0, a0, 1
    addiu   a1, a1, 1
    bne     a0, a3, 7b
    sb      t2, -1(a0)                 # (delay slot) store to 0(old a0)

end:
    jr      ra
    nop                                # cannot delay usefully here

    .end     memcpy
