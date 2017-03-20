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

    .extern  memcpy

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
    beq     a2, zero, 3f               # copying 0 bytes? we're done
    or      v0, a0, zero               # (delay slot) set the return value

    beq     a0, a1, 3f                 # copying from and to the same location

    # a0 + a2 bytes goes into t0; a1 + a2 bytes goes into t1.
    # a0 < t0; a1 < t1.
    addu    t0, a0, a2                 # (delay slot)
    addu    t1, a1, a2

    # There are two cases of non-overlapping moves, which we handle by going
    # to memcpy:
    # [src]        |        [src]
    #       [dst]  |  [dst]
    #   a0 >= t1   |    a1 >= t0
    sltu    t2, a0, t1                 # (a0 >= t1) == !(a0 < t1)
    beq     t2, zero, copy             # if !(a0 < t1) goto copy;
    sltu    t3, a1, t0                 # (a1 >= t0) == !(a1 < t0)
    beq     t3, zero, copy             # if !(a1 < t0) goto copy;

    # If the source is to the left of the destination, copying forwards would
    # just keep echoing bytes from the beginning of the destination. Copy
    # backwards.
    sltu    t4, a1, a0                 # (delay slot) t4 = (src < dest)
    bne     t4, zero, 2f               # if (src < dest) goto 2; (backwards)
    nop                                # cannot delay usefully here

    # Here we're copying forwards.
1:  lbu     t2, 0(a1)
    # Let the load complete in parallel with these.
    addiu   a0, a0, 1
    addiu   a1, a1, 1
    bne     a0, t0, 1b
    # This store is 0(old a0), but it was advanced above.
    sb      t2, -1(a0)                 # (delay slot)

    jr      ra
    nop                                # cannot delay usefully here

    # Here we're copying backwards.
2:  lbu     t2, -1(t1)
    # Let the load complete in parallel with these.
    addiu   t0, t0, -1
    addiu   t1, t1, -1
    bne     t0, a0, 2b
    sb      t2, 0(t0)                  # (delay slot)

3:  jr      ra
    nop                                # cannot delay usefully here

copy:
    j       memcpy
    nop

    .end     memmove
