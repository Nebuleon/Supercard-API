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

    .extern  malloc
    .extern  memset

    .ent     calloc
    .global  calloc
    .type    calloc,@function

    /* int calloc(size_t count, size_t size)
     * Allocates and zero-fills a memory region of count * size bytes.
     *
     * In:
     *   argument 1: Number of elements to allocate memory for.
     *   argument 2: Size, in bytes, of each element.
     * Input assumptions (checked):
     * - count * size does not overflow.
     * Returns:
     *   A pointer to the newly-allocated memory region on success.
     *   NULL if either malloc returns NULL or count * size overflows.
     */
calloc:
    multu   a0, a1
    # Let the multiplication complete in parallel with these.
    addiu   sp, sp, -24
    sw      ra, 16(sp)
    sw      s0, 20(sp)

    mfhi    t0                         # grab the high bits
    bne     t0, zero, 2f               # overflow occurs if HI != 0
    mflo    a0                         # (delay slot) grab LO into argument 1

    # FALLTHROUGH with a0 forwarded from the multiplication.
    jal     malloc
    or      s0, a0, zero               # (delay slot) save the size for memset

    beq     v0, zero, 1f               # v0 is already NULL; bail out at 1:
    or      a1, zero, zero             # (delay slot) prepare argument 2
    or      a0, v0, zero               # forward allocation as argument 1
    jal     memset                     # memset(allocation, 0, count * size);
    or      a2, s0, zero               # (delay slot) prepare argument 3

    # FALLTHROUGH with v0 forwarded from memset.
1:  lw      ra, 16(sp)
    lw      s0, 20(sp)
    jr      ra
    addiu   sp, sp, 24                 # (delay slot)

2:  b       1b
    or      v0, zero, zero             # (delay slot) failed: return NULL

    .end     calloc
