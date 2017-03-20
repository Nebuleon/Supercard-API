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

    .ent     memcmp
    .global  memcmp
    .type    memcmp,@function

    /* int memcmp(const void* a, const void* b, size_t n)
     * Compares 'n' bytes at 'a' and 'b'.
     *
     * In:
     *   arguments 1 and 2: Pointers to data to be compared.
     *   argument 3: The number of bytes to compare.
     * Input assumptions (not checked):
     * - 'n' bytes are mapped and readable at 'a'.
     * - 'n' bytes are mapped and readable at 'b'.
     * Returns:
     *   0 if, for each 'i' among 0 and 'n', the bytes at 'a + i' and 'b + i'
     *   are equal.
     *   Less than 0 if the first unequal byte is less in 'a'.
     *   Greater than 0 if the first unequal byte is greater in 'a'.
     */
memcmp:
    # If we're asked to compare fewer than 16 bytes, it's more efficient to
    # just compare bytes than to compute the misalignment mismatch, prefix
    # size and suffix size.
    sltiu   t0, a2, 16
    bne     t0, zero, suffix
    addu    a3, a0, a2                 # (delay slot) a3 = end of a (exclusive bound)

    andi    t0, a0, 3                  # t0 = misalignment in a0
    andi    t1, a1, 3                  # t1 = misalignment in a1
    bne     t0, t1, suffix             # misalignments mismatch; just compare bytes
    nop                                # cannot delay usefully here

    beq     t0, zero, words
    subu    t1, a0, t1                 # (delay slot) delete the misalignment
    addiu   t1, t1, 4                  # add 4; that's where the prefix ends

1:  lbu     t2, 0(a0)
    lbu     t3, 0(a1)
    bne     t2, t3, unequal_byte
    addiu   a0, a0, 1                  # (delay slot)
    bne     a0, t1, 1b
    addiu   a1, a1, 1                  # (delay slot)

words:
    srl     t1, a3, 2                  # store in t1 the end of aligned words,
    sll     t1, t1, 2                  # so we can find the end with BNE

2:  lw      t2, 0(a0)
    lw      t3, 0(a1)
    bne     t2, t3, unequal_word
    addiu   a0, a0, 4                  # (delay slot)
    bne     a0, t1, 2b
    addiu   a1, a1, 4                  # (delay slot)

suffix:
    beq     a0, a3, 4f
    nop                                # cannot delay usefully here
3:  lbu     t2, 0(a0)
    lbu     t3, 0(a1)
    bne     t2, t3, unequal_byte
    addiu   a0, a0, 1                  # (delay slot)
    bne     a0, a3, 3b
    addiu   a1, a1, 1                  # (delay slot)

4:  jr      ra
    or      v0, zero, zero             # (delay slot) all bytes are equal

unequal_byte:
    jr      ra
    subu    v0, t2, t3                 # (delay slot) compute the difference

unequal_word:
    # Jump ahead of the part of 'suffix' that checks whether a0 == a3, because
    # a0 will always be at least 4 bytes before a3.
    b       3b
    # Undo the work done by the delay slot that brought us here (in a delay
    # slot). The suffix will catch the different byte and send execution to
    # unequal_byte to return.
    addiu   a0, a0, -4

    .end     memcmp
