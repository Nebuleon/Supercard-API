/*
 * This file is part of the MIPS unpacker for the Supercard DSTwo.
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

#include "mips.h"

    .text
    .set     noreorder
    .set     at

    .extern  __stack
    .extern  __text_start
    .extern  __text_end
    .extern  __data_start
    .extern  packed_data
    .extern  __bss_start
    .extern  __bss_end
    .extern  memcpy
    .extern  memset
    .extern  unpacked_size
    .extern  packed_size
    .extern  __heap_start
    .extern  __heap_end
    .extern  _heap_init
    .extern  entry

    .ent     _start
    .global  _start
    .type    _start,@function

_start:
    /* BEGIN MAGIC SECTION - SET BY THE PACKER */
    lui     s4, 0                      # s4 = unpacked size
    ori     s4, s4, 0
    lui     s5, 0                      # s5 = packed size
    ori     s5, s5, 0
    /* END MAGIC SECTION - SET BY THE PACKER */

    /* Initialise Coprocessor 0 registers. */
    mtc0    zero, C0_WatchLo           # reset hardware watchpoints
    mtc0    zero, C0_WatchHi

    li      t1, 0x00800000
    mtc0    t1, C0_Cause               # reset interrupts

    # In 0x00400000:
    # Bits 31..28 = 0000: Disable access to all coprocessors in user mode
    # Bit 27 = 0: Disable reduced power mode
    # Bit 25 = 0: Disable endian reversal in user mode
    # Bit 22 = 1: Bootstrap exception vector locations (BFC0_xxxx)
    # Bit 21 = 0: Resetting TLB Multiple Matches bit
    # Bit 20 = 0: Not in Soft Reset exception
    # Bit 19 = 0: Not in NMI exception
    # Bits 15..8 = 00000000: Interrupts: Hardware disabled, software disabled
    # Bit 4 = 0: Operating in kernel mode
    # Bit 2 = 0: Not in error level
    # Bit 1 = 0: Not in exception level
    # Bit 0 = 0: Interrupts disabled
    li      t0, 0x00400000
    mtc0    t0, C0_Status

    /* Initialise the cache */
    # For each index at 0x8000_0000 + 16 KiB (cache size), write an empty
    # cache tag to the data cache and the instruction cache.
    # Important: 0x8000_0000 is not mapped through the MMU. Virtual addresses
    # can raise exceptions if they're not mapped.
    li      t0, 0x80000000
    ori     t1, t0, 0x4000
    mtc0    zero, C0_TagLo
    mtc0    zero, C0_TagLo, 2
    mtc0    zero, C0_TagHi
    mtc0    zero, C0_TagHi, 2

1:  cache   DCIndexStTag, 0(t0)
    cache   ICIndexStTag, 0(t0)
    bne     t0, t1, 1b
    addiu   t0, t0, 0x20               # cache lines are 32 bytes

    sync

    # Enable the use of the cache for accesses through KSEG0.
    li      t0, 3
    mtc0    t0, C0_Config

    /* Invalidate the BTB */
    mfc0    t0, C0_Config, 7
    nop
    ori     t0, t0, 0x2
    mtc0    t0, C0_Config, 7
    nop

    /* Set up the stack, preparing for the jump to C */
    la      sp, __stack

    /* Copy code from the load address to the execution address */
    la      s0, 0x80002000
    la      s1, __text_start
    la      s2, __text_end

2:  lw      t0, 0(s0)
    # Let the load complete in parallel with these.
    addiu   s0, s0, 4
    addiu   s1, s1, 4
    bne     s1, s2, 2b
    sw      t0, -4(s1)                 # (delay slot) store to 0(old s1)

    /* Write back and invalidate both caches. */
    li      t0, 0x80000000
    ori     t1, t0, 0x4000
3:  cache   DCIndexWBInv, 0(t0)
    cache   ICIndexInv, 0(t0)
    bne     t0, t1, 3b
    addiu   t0, t0, 0x20               # cache lines are 32 bytes

    sync

    /* Invalidate the BTB */
    mfc0    t0, C0_Config, 7
    nop
    ori     t0, t0, 0x2
    mtc0    t0, C0_Config, 7
    nop

    /* Now that the code is copied, copy data too, with memcpy */
    la      a0, __data_start           # destination = __data_start
    la      a2, packed_data
    /* Jump to ourselves at 4:. Since the linker thinks we're at 0x8100_0000,
     * this J instruction will actually transfer us to the new copy. */
    j       4f                         # v delay slot v
    or      a1, s0, zero               # source = 0x80002000 + .text size
4:  subu    a2, a2, a0                 # length = packed_data - __data_start
    jal     memcpy
    addu    a2, a2, s5                 # (delay slot) + packed size

    /* Clear the uninitialised data section */
    la      a0, __bss_start            # start = __bss_start
    la      a2, __bss_end
    subu    a2, a2, a0                 # count = __bss_end - __bss_start
    jal     memset
    or      a1, zero, zero             # (delay slot) byte = 0

    /* Now that the uninitialised data section is cleared, store the packed
     * and unpacked sizes to memory */
    sw      s4, unpacked_size
    sw      s5, packed_size

    /* Initialise the heap for malloc, realloc and free. */
    la      a0, __heap_start
    lui     a1, %hi(__heap_end)
    jal     _heap_init
    addiu   a1, a1, %lo(__heap_end)    # (delay slot)

    /* Call int entry(void). */
    jal     entry
    nop                                # cannot delay usefully here

    bne     v0, zero, exit_fail
    # This BNE has a harmless delay slot from below.

    /* Write back and invalidate both caches. */
    li      t0, 0x80000000
    ori     t1, t0, 0x4000
5:  cache   DCIndexWBInv, 0(t0)
    cache   ICIndexInv, 0(t0)
    bne     t0, t1, 5b
    addiu   t0, t0, 0x20               # cache lines are 32 bytes

    sync

    /* Invalidate the BTB */
    mfc0    t0, C0_Config, 7
    nop
    ori     t0, t0, 0x2
    mtc0    t0, C0_Config, 7
    nop

    j       0x80002000                 # transfer control to the unpacked executable
    nop                                # cannot delay usefully here

exit_fail:
    wait
    b       exit_fail
    nop                                # cannot delay usefully here

    .end     _start
