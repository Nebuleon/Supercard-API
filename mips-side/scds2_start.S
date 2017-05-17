/*
 * This file is part of the initialisation code for the Supercard DSTwo.
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
#include <ds2/except.h>

    .text
    .set     noreorder
    .set     noat

    .extern  _gp
    .extern  __stack
    .extern  __bss_start
    .extern  __bss_end
    .extern  __heap_start
    .extern  __heap_end
    .extern  _heap_init
    .extern  _ds2_init
    .extern  _ds2_ds_init
    .extern  _ds2_init_fs
    .extern  malloc
    .extern  main
    .extern  free
    .extern  exit
    .extern  _exception_handlers
    .extern  _unhandled_exception_save
    .extern  _irq_handler

    .ent     _start
    .global  _start
    .type    _start,@function

_start:
    /* Initialise Coprocessor 0 registers. */
    mtc0    zero, C0_WatchLo           # reset hardware watchpoints
    mtc0    zero, C0_WatchHi

    # In 0x00800000:
    # Bit 23 = 1: Special interrupt vector (0x8000_0200)
    # Bit 22 = 0: No Watch exception pending
    # Bits 9..8 = 00: No software interrupts pending
    li      t1, 0x00800000
    mtc0    t1, C0_Cause

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

    /* Since we are executing from 0x8000_2000, the caches must be initialised
     * already, so skip that initialisation. */

    # Initialise the address used by DS2_StartAwait.
    lui     t7, 0x8000
    sw      zero, 0x1FFC(t7)

    /* Set up the stack and the GP register, preparing for the jump to C */
    la      gp, _gp
    la      sp, __stack

    /* Clear the uninitialised data section */
    la      a0, __bss_start            # start = __bss_start
    la      a2, __bss_end
    subu    a2, a2, a0                 # count = __bss_end - __bss_start
    jal     memset
    or      a1, zero, zero             # (delay slot) byte = 0

    /* Initialise the heap for malloc, realloc and free. */
    la      a0, __heap_start
    la      a1, __heap_end
    jal     _heap_init
    nop                                # cannot delay usefully here

    jal     _ds2_init
    nop                                # cannot delay usefully here

    jal     _ds2_ds_init
    nop                                # cannot delay usefully here

    jal     _ds2_init_fs
    nop                                # cannot delay usefully here

    /* Call int main(int argc, char** argv). */
    /* TODO Implement an argv protocol */
    jal     malloc
    ori     a0, zero, 4                # temp = malloc(4)

    beq     v0, zero, exit_fail        # if (temp == NULL) goto exit_fail
    or      s0, v0, zero               # (delay slot) main() will preserve this

    or      a1, v0, zero               # argv = temp

    /* "The value of argv[argc] shall be 0." - C99 */
    sw      zero, 0(v0)                # argv[argc] = 0

    jal     main
    or      a0, zero, zero             # (delay slot) argc = 0

    or      s1, v0, zero               # grab the return value from main()
    jal     free
    or      a0, s0, zero               # (delay slot) free(argv)

2:  j       exit                       # exit(main());
    or      a0, s1, zero               # (delay slot) argument 1 = return value

exit_fail:
    b       2b
    ori     s1, zero, 1

    .end     _start


    .ent     _tlb_refill_vector
    .global  _tlb_refill_vector
    .global  _tlb_refill_vector_end
    .type    _tlb_refill_vector,@function

_tlb_refill_vector:
    /* Copied to 0x8000_0000. Next up is: the cache error vector
     * (0x8000_0100). */

    la      k0, _exception_handlers + EXCODE_TLB_REFILL << 2
    lw      k0, 0(k0)
    beq     k0, zero, tlb_refill_unhandled
    li      k1, EXCODE_TLB_REFILL

    # Here, there was a handler.
    jr      k0
    nop                                # cannot delay usefully here

tlb_refill_unhandled:
    j       _unhandled_exception_save
    nop                                # cannot delay usefully here

_tlb_refill_vector_end:

    .end     _tlb_refill_vector


    .ent     _cache_error_vector
    .global  _cache_error_vector
    .global  _cache_error_vector_end
    .type    _cache_error_vector,@function

_cache_error_vector:
    /* Copied to 0x8000_0100, run at 0xA000_0100. Next up is: the general
     * exception vector (0x8000_0180). */

    la      k0, _exception_handlers + EXCODE_CACHE_ERROR << 2
    lw      k0, 0(k0)
    beq     k0, zero, cache_error_unhandled
    li      k1, EXCODE_CACHE_ERROR

    # Here, there was a handler.
    jr      k0
    nop                                # cannot delay usefully here

cache_error_unhandled:
    j       _unhandled_exception_save
    nop                                # cannot delay usefully here

_cache_error_vector_end:

    .end     _cache_error_vector

    .ent     _general_exception_vector
    .global  _general_exception_vector
    .global  _general_exception_vector_end
    .type    _general_exception_vector,@function

_general_exception_vector:
    /* Copied to 0x8000_0180. Next up is: the interrupt vector (0x8000_0200). */

    mfc0    k1, C0_Cause
    la      k0, _exception_handlers
    andi    k1, k1, 0x1F << 2          # no shifting needed to get a function
    addu    k0, k0, k1                 #   pointer for dispatch
    lw      k0, 0(k0)
    beq     k0, zero, general_exception_unhandled
    nop                                # cannot delay usefully here

    # Here, there was a handler.
    jr      k0
    nop                                # cannot delay usefully here

general_exception_unhandled:
    j       _unhandled_exception_save
    nop                                # cannot delay usefully here

_general_exception_vector_end:

    .end     _general_exception_vector


    .ent     _interrupt_vector
    .global  _interrupt_vector
    .global  _interrupt_vector_end
    .type    _interrupt_vector,@function

_interrupt_vector:
    /* Copied to 0x8000_0200. Next up is: the kernel stack (0x8000_1FE0, going
     * down). */
    # Prepare the switch to the kernel stack. There's the pointer for
    # DS2_StartAwait at 0x8000_1FFC.
    la      k0, 0x80001FE0 - 104
    pref    30,    8(k0)               # PrepareForStore: clear cache lines
    pref    30,   40(k0)               # without loading their data from RAM
    pref    30,   72(k0)
    sw      sp,   84(k0)               # Store the old stack pointer
    or      sp, k0, zero               # Change to the new one
    # These are the least likely to have been loaded by LB[U]/LH[U]/LW before
    # the code was interrupted. Save them first to give more time for any
    # loads of the others to complete.
    sw      $31,  88(sp)

    sw      $1,   16(sp)
    sw      $2,   20(sp)
    sw      $3,   24(sp)
    sw      $4,   28(sp)
    sw      $5,   32(sp)
    sw      $6,   36(sp)
    sw      $7,   40(sp)
    sw      $8,   44(sp)
    sw      $9,   48(sp)
    sw      $10,  52(sp)
    sw      $11,  56(sp)
    sw      $12,  60(sp)
    sw      $13,  64(sp)
    sw      $14,  68(sp)
    sw      $15,  72(sp)
    sw      $24,  76(sp)
    sw      $25,  80(sp)

    # These instructions are now very far away from anything that could have
    # caused writes to HI and LO.
    mfhi    t0                         # [while MFC0s run]
    mflo    t1

    sw      t0,   92(sp)               # save HI
    sw      t1,   96(sp)               # save LO

    # Saved registers don't need to be saved. We are calling a C function that
    # will preserve their values for the caller (= this code right here). Even
    # nested interrupts will preserve the value for the caller (= the previous
    # interrupt handler) if interrupts get re-enabled, and so on, until we get
    # to the end with the caller's (= the code that got interrupted) values in
    # $16-$23 (s0-s7), $28 (gp) and $30 (fp).

    # Call the C interrupt handler (with interrupts disabled).
    jal     _irq_handler
    nop                                # cannot delay usefully here

    # Code may be awaiting a certain condition to happen at the end of an
    # interrupt (see DS2_StartAwait).
    lui     t7, 0x8000                 # [while LW t4 and t5 run]
    lw      t6, 0x1FFC(t7)
    bne     t6, zero, ret_to_await
    lw      sp,   84(k0)               # (delay slot) restore stack regardless

    # These are the most likely to be needed immediately. Load them first to
    # give the loads more time to complete before returning to the code that
    # was interrupted.
    lw      $31,  88(k0)

    lw      k1,   92(k0)               # load HI
    lw      $1,   16(k0)
    lw      $2,   20(k0)
    lw      $3,   24(k0)
    lw      $4,   28(k0)
    lw      $5,   32(k0)
    lw      $6,   36(k0)
    lw      $7,   40(k0)
    mthi    k1
    lw      $8,   44(k0)
    lw      $9,   48(k0)
    lw      k1,   96(k0)               # load LO
    lw      $10,  52(k0)
    lw      $11,  56(k0)
    lw      $12,  60(k0)
    lw      $13,  64(k0)
    lw      $14,  68(k0)
    lw      $15,  72(k0)
    # Don't restore $16-$23 (s0-s7), $28 (gp) or $30 (fp), because they were
    # saved by the exception handler's callees.
    mtlo    k1
    lw      $24,  76(k0)
    lw      $25,  80(k0)

    # These values will never be needed again, both by kernel code and by user
    # code. To avoid user code needing to write them back, even when it's only
    # reading memory and evicting one of our dirty lines, invalidate them. The
    # DCHitInv encoding is optional, but we're only using it for optimisation.
    cache   DCHitInv,  8(k0)
    cache   DCHitInv, 40(k0)
    cache   DCHitInv, 72(k0)

    eret

ret_to_await:
    # Here, we have code that is awaiting a condition.
    # None of the GPRs' values matter.
    mtc0    t6, C0_EPC

    eret

_interrupt_vector_end:

    .end     _interrupt_vector
