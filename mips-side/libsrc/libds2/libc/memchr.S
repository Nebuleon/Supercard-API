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

    .ent     memchr
    .global  memchr
    .type    memchr,@function

    /* char* memchr(const void* src, int ch, size_t n)
     * Searches for the first occurrence of 'ch' in 'src'.
     *
     * In:
     *   argument 1: Pointer to data to search for argument 2 in.
     *   argument 2: The byte to search for.
     *   argument 3: The number of bytes to be searched.
     * Input assumptions (not checked):
     * - 'n' bytes are mapped and readable at 'src'.
     * Returns:
     *   A pointer to the first byte equal to 'ch' (that is, the pointer to
     *   'src + i', with the least value of 'i' under 'n', which is equal to
     *   'ch'), or NULL if there is no such byte.
     */
memchr:
    # If we're asked to search in less than 32 bytes, it's more efficient to
    # just search bytes than to compute the search word, prefix misalignment
    # and the size of the suffix.
    andi    a1, a1, 0xFF               # cast ch to unsigned int
    sltiu   t0, a2, 32
    bne     t0, zero, suffix
    addu    a3, a0, a2                 # (delay slot) a3 = end (exclusive bound)

    # Create, in t4, a word that has the byte in a1 repeated 4 times.
    sll     t4, a1, 8
    or      t4, t4, a1
    sll     t5, t4, 16
    or      t4, t4, t5

    andi    t0, a0, 0x3                # t0 = misalignment in 'src'
    beq     t0, zero, words
    subu    t0, a0, t0                 # (delay slot) delete the misalignment
    addiu   t0, t0, 4                  # add 4; that's where the prefix ends

1:  lbu     t1, 0(a0)
    beq     t1, a1, 6f                 # if (*src == (unsigned char) ch) ...
    addiu   a0, a0, 1                  # (delay slot)
    bne     a0, t0, 1b
    nop                                # cannot delay usefully here

words:
    srl     t0, a3, 2                  # store in t0 the end of aligned words,
    sll     t0, t0, 2                  # so we can find the end with BNE

2:  lw      t1, 0(a0)
    # Detect possible matches in the word by XORing it with the word full of
    # the search byte, then ANDing the bytes with each other. The result will
    # be 0 if the word MAY contain the byte (due to ch XOR ch being 0): the
    # result will also be 0 if the bytes in the result of XOR have distinct
    # bits set (such as <0C 30 03 C0> with ch == 0), due to these distinct
    # bits ANDing to 0.
    xor     t1, t1, t4
    srl     t2, t1, 16
    and     t1, t1, t2                 # fold the two halfwords into one
    srl     t2, t1, 8
    and     t1, t1, t2                 # fold the four bytes into one
    beq     t1, zero, possible_match
    addiu   a0, a0, 4                  # (delay slot)
3:  bne     a0, t0, 2b
    nop                                # cannot delay usefully here

suffix:
    beq     a0, a3, 5f
    nop                                # cannot delay usefully here
4:  lbu     t1, 0(a0)
    beq     t1, a1, 6f                 # if (*src == (unsigned char) ch) ...
    addiu   a0, a0, 1                  # (delay slot)
    bne     a0, a3, 4b
    nop                                # cannot delay usefully here

5:  jr      ra
    or      v0, zero, zero             # (delay slot) not found: return NULL

6:  jr      ra
    # We came here from a delay slot that advanced a0 past the byte matching
    # 'ch' for performance reasons. Rewind it and return that.
    addiu   v0, a0, -1                 # (delay slot)

possible_match:
    # Undo the work done by the delay slot that brought us here.
    lbu     t1, -4(a0)
    beq     t1, a1, 6b
    addiu   a0, a0, -3                 # (delay slot)
    lbu     t1, 0(a0)
    beq     t1, a1, 6b
    addiu   a0, a0, 1                  # (delay slot)
    lbu     t1, 0(a0)
    beq     t1, a1, 6b
    addiu   a0, a0, 1                  # (delay slot)
    lbu     t1, 0(a0)
    beq     t1, a1, 6b
    addiu   a0, a0, 1                  # (delay slot)
    b       3b                         # none of the bytes matched, go back
    nop                                # after the BEQ that brought us here

    .end     memchr
