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

    .ent     memmove
    .global  memmove
    .type    memmove,@function

    /* void* memmove(void* dst, const void* src, size_t n)
     * Copies 'n' bytes from 'src' to 'dst', handling overlap in the memory
     * regions correctly.
     *
     * In:
     *   argument 1: The destination pointer.
     *   argument 2: The source pointer.
     *   argument 3: The number of bytes to copy.
     * Input assumptions (not checked):
     * - 'n' bytes are mapped and readable at 'src'.
     * - 'n' bytes are mapped and writable at 'dst'.
     * Returns:
     *   Argument 1.
     */
memmove:
    # a0 + a2 bytes goes into t0; a1 + a2 bytes goes into t1.
    # a0 < t0; a1 < t1.
    addu    t0, a0, a2                 # (delay slot)
    addu    t1, a1, a2

    # If the source is to the left of the destination, copying forwards would
    # just keep echoing bytes from the beginning of the destination. Copy
    # backwards.
    sltu    t4, a1, a0                 # (delay slot) t4 = (src < dest)
    bne     t4, zero, B                # if (src < dest) copy backwards
    or      v0, a0, zero               # (delay) memmove returns argument 1

    # The implementations are named according to the kind of work they do:
    # - F, B: forwards, backwards;
    # - b, h, w: byte, halfword, word;
    # - u, a: unaligned (source), aligned (both);
    # - p, s: prefix, suffix.
    # - a_b: 'b' enabling the use of 'a'.
F:
    sltiu   t7, a2, 16                 # if we're asked to move fewer than 16
    bne     t7, zero, Fbs              #   bytes, go copy bytes
    # t2 gets the gap between the destination and the source.
    subu    t2, a1, a0                 # (delay slot)
    li      t3, 1
    beq     t2, t3, Fbs                # the gap is 1 byte; go copy bytes
    sltiu   t7, t2, 4                  # (delay slot) gap = 2..3 bytes?
    bne     t7, zero, Fh_bp            # yes; go copy halfwords
    li      t5, ~0x3                   # (delay slot) mask to align to words

Fw_bp:
    # Store, in t7, the end of the byte prefix, so that we can find the end
    # with BNE.
    addiu   t7, a0, 3
    and     t7, t7, t5
    beq     a0, t7, Fw                 # no misalignment in the word?
    # If misalignments mismatch, we will need to use unaligned loads.
    # Currently, a0 is aligned.
    andi    t4, a1, 0x3                # (delay slot)
1:  lbu     t3, 0(a1)
    # Let the load complete in parallel with these.
    addiu   a0, a0, 1
    addiu   a1, a1, 1
    bne     a0, t7, 1b
    # This store is 0(old a0), but it was advanced above.
    sb      t3, -1(a0)                 # (delay slot)

Fw:
    beq     t4, zero, Fwa
    # Store, in t8, the end of the portion we can store as whole words, so
    # that we can find the end with BNE.
    and     t8, t0, t5                 # (delay slot)

Fwu:
    ulw     t3, 0(a1)
    # Let the load complete in parallel with these.
    addiu   a0, a0, 4
    addiu   a1, a1, 4
    bne     a0, t8, Fwu
    sw      t3, -4(a0)                 # (delay slot) store to 0(old a0)

    b       Fbs                        # check for any byte suffix
    nop                                # cannot delay usefully here

Fwa:
    lw      t3, 0(a1)
    # Let the load complete in parallel with these.
    addiu   a0, a0, 4
    addiu   a1, a1, 4
    bne     a0, t8, Fwa
    sw      t3, -4(a0)                 # (delay slot) store to 0(old a0)

    b       Fbs                        # check for any byte suffix
    nop                                # cannot delay usefully here

Fh_bp:
    # If misalignments mismatch, go copy bytes.
    andi    t3, a0, 0x1
    andi    t4, a1, 0x1
    bne     t3, t4, Fbs
    # Store, in t8, the end of the portion we can store as whole halfwords, so
    # that we can find the end with BNE.
    srl     t8, t0, 1                  # (delay slot)
    beq     t3, zero, Fh
    sll     t8, t8, 1                  # (delay slot)

    lbu     t3, 0(a1)
    # Let the load complete in parallel with these.
    addiu   a0, a0, 1
    addiu   a1, a1, 1
    # This store is 0(old a0), but it was advanced above.
    sb      t3, -1(a0)                 # (delay slot)

Fh:
    lhu     t3, 0(a1)
    # Let the load complete in parallel with these.
    addiu   a0, a0, 2
    addiu   a1, a1, 2
    bne     a0, t8, Fh
    # This store is 0(old a0), but it was advanced above.
    sh      t3, -2(a0)                 # (delay slot)

Fbs:
    beq     a0, t0, Fend
    nop                                # cannot delay usefully here
2:  lbu     t3, 0(a1)
    # Let the load complete in parallel with these.
    addiu   a0, a0, 1
    addiu   a1, a1, 1
    bne     a0, t0, 2b
    # This store is 0(old a0), but it was advanced above.
    sb      t3, -1(a0)                 # (delay slot)

Fend:
    jr      ra
    nop                                # cannot delay usefully here

B:
    sltiu   t7, a2, 16                 # if we're asked to move fewer than 16
    bne     t7, zero, Bbs              #   bytes, go copy bytes
    # t2 gets the gap between the source and the destination.
    subu    t2, a0, a1                 # (delay slot)
    li      t3, 1
    beq     t2, t3, Bbs                # the gap is 1 byte; go copy bytes
    sltiu   t7, t2, 4                  # (delay slot) gap = 2..3 bytes?
    bne     t7, zero, Bh_bp            # yes; go copy halfwords
    li      t5, ~0x3                   # (delay slot) mask to align to words

Bw_bp:
    # Store, in t7, the beginning of the byte prefix, so that we can find it
    # with BNE.
    and     t7, t0, t5
    beq     t0, t7, Bw                 # no misalignment in the word?
    addiu   t8, a0, 3                  # (delay slot) [see Bw for comments]
3:  lbu     t3, -1(t1)
    # Let the load complete in parallel with these.
    addiu   t0, t0, -1
    addiu   t1, t1, -1
    bne     t0, t7, 3b
    # This store is -1(old t0), but it was decremented above.
    sb      t3, 0(t0)                  # (delay slot)

Bw:
    # If misalignments mismatch, we need to use unaligned loads.
    # Currently, t0 is aligned.
    andi    t4, t1, 0x3
    beq     t4, zero, Bwa
    # Store, in t8, the beginning of the portion we can store as whole words,
    # so that we can find it with BNE. (Part of this was done by Bw_bp.)
    and     t8, t8, t5                 # (delay slot)

Bwu:
    ulw     t3, -4(t1)
    # Let the load complete in parallel with these.
    addiu   t0, t0, -4
    addiu   t1, t1, -4
    bne     t0, t8, Bwu
    # This store is -4(old t0), but it was decremented above.
    sw      t3, 0(t0)                  # (delay slot)

    b       Bbs                        # check for any byte suffix
    nop                                # cannot delay usefully here

Bwa:
    lw      t3, -4(t1)
    # Let the load complete in parallel with these.
    addiu   t0, t0, -4
    addiu   t1, t1, -4
    bne     t0, t8, Bwa
    # This store is -4(old t0), but it was decremented above.
    sw      t3, 0(t0)                  # (delay slot)

    b       Bbs                        # check for any byte suffix
    nop                                # cannot delay usefully here

Bh_bp:
    # If misalignments mismatch, go copy bytes.
    andi    t3, t0, 0x1
    andi    t4, t1, 0x1
    bne     t3, t4, Bbs
    # Store, in t8, the beginning of the portion we can store as whole
    # halfwords, so that we can find it with BNE.
    addiu   t8, a0, 1                  # (delay slot)
    srl     t8, t8, 1
    beq     t3, zero, Bh
    sll     t8, t8, 1                  # (delay slot)

    lbu     t3, -1(t1)
    # Let the load complete in parallel with these.
    addiu   t0, t0, -1
    addiu   t1, t1, -1
    # This store is -1(old t0), but it was decremented above.
    sb      t3, 0(t0)                  # (delay slot)

Bh:
    lhu     t3, -2(t1)
    # Let the load complete in parallel with these.
    addiu   t0, t0, -2
    addiu   t1, t1, -2
    bne     t0, t8, Bh
    # This store is -2(old t0), but it was decremented above.
    sh      t3, 0(t0)                  # (delay slot)

Bbs:
    beq     t0, a0, Bend
    nop                                # cannot delay usefully here
4:  lbu     t3, -1(t1)
    # Let the load complete in parallel with these.
    addiu   t0, t0, -1
    addiu   t1, t1, -1
    bne     t0, a0, 4b
    sb      t3, 0(t0)                  # (delay slot)

Bend:
    jr      ra
    nop                                # cannot delay usefully here

    .end     memmove
