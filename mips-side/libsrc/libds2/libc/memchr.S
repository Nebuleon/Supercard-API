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
    sltiu   v0, a2, 32
    bne     v0, zero, suffix
    addu    a3, a0, a2                 # (delay slot) a3 = end (exclusive bound)

    # Create some magic words.
    # First, in t2, a word that has the byte in a1 repeated 4 times.
    sll     t2, a1, 8
    or      t2, t2, a1
    sll     v0, t2, 16
    or      t2, t2, v0
    # Then, words that have the lowest (t3) and highest (t4) bits set in each
    # byte, respectively.
    li      t3, 0x01010101
    sll     t4, t3, 7

    # Figure out where the byte prefix ends, if any.
    addiu   v1, a0, 3
    li      t9, ~3
    and     v1, v1, t9
    beq     v1, a0, words
    nop                                # cannot delay usefully here

1:  lbu     v0, 0(a0)
    beq     v0, a1, 5f                 # if (*src == (unsigned char) ch) ...
    addiu   a0, a0, 1                  # (delay slot)
    bne     a0, v1, 1b
    nop                                # cannot delay usefully here

words:
    # Store in v1 the end of aligned words, so we can find the end with BNE.
    and     v1, a3, t9

2:  lw      v0, 0(a0)
    # Detect matches in the word by XORing it with the word full of the search
    # byte; any zero byte in the result indicates a match.
    xor     v0, v0, t2
    # Subtract the low ones. Borrows will make the zero bytes into 0xFF.
    subu    t0, v0, t3
    # Prevent a borrow from creating a false match on the high bit of another
    # byte. Require a match on the high bit again.
    nor     t1, v0, zero
    and     v0, t0, t1                 # combine with required matches
    and     v0, v0, t4                 # only look at the remaining borrows
    bne     v0, zero, match
    addiu   a0, a0, 4                  # (delay slot)
    bne     a0, v1, 2b
    nop                                # cannot delay usefully here

suffix:
    beq     a0, a3, 4f
    nop                                # cannot delay usefully here
3:  lbu     v0, 0(a0)
    beq     v0, a1, 5f                 # if (*src == (unsigned char) ch) ...
    addiu   a0, a0, 1                  # (delay slot)
    bne     a0, a3, 3b
    nop                                # cannot delay usefully here

4:  jr      ra
    move    v0, zero                   # (delay slot) not found: return NULL

5:  jr      ra
    # We came here from a delay slot that advanced a0 past the byte matching
    # 'ch' for performance reasons. Rewind it and return that.
    addiu   v0, a0, -1                 # (delay slot)

match:
    # Jump ahead of the part of 'suffix' that checks whether a0 == a3, because
    # a0 will always be at least 4 bytes before a3.
    b       3b
    # Undo the work done by the delay slot that brought us here (in a delay
    # slot). The suffix will catch the matching byte.
    addiu   a0, a0, -4

    .end     memchr
