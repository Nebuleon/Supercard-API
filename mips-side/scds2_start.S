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

    .text
    .set     noreorder
    .set     noat

    .extern  _ds2_init
    .extern  _ds2_init_fs
    .extern  _ds2_ds_init
    .extern  _heap_init
    .extern  malloc
    .extern  main
    .extern  free
    .extern  exit
    .extern  _gp
    .extern  __all_end
    .extern  __bss_start
    .extern  __bss_end
    .extern  _irq_handler
    .extern  _exception_handler
    .extern  _unhandled_exception
    .extern  _syscall_handler

    .ent     _start
    .global  _start
    .type    _start,@function

_start:
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


    .extern  exception_entry

    .ent     _exception_jump
    .global  _exception_jump
    .type    _exception_jump,@function

    /* _ds2_init will copy 0x10 bytes (4 instructions) from here into
     * 3 locations: 0x8000_0000, 0x8000_0180 and 0x8000_0200. */
_exception_jump:
    j       exception_entry
    nop                                # cannot delay usefully here
    nop                                # pad the bytes to be copied
    nop

    .end     _exception_jump


    .ent     exception_entry
    .global  exception_entry
    .type    exception_entry,@function

exception_entry:
    # Prepare the switch to the kernel stack. There's the pointer for
    # DS2_StartAwait at 0x8000_1FFC.
    la      k0, 0x80001FE0 - 160
    sw      sp,  120(k0)               # Store the old stack pointer
    or      sp, k0, zero               # Change to the new one
    # These are the least likely to have been loaded by LB[U]/LH[U]/LW before
    # the code was interrupted. Save them first to give more time for any
    # loads of the others to complete.
    sw      $31, 128(sp)

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
    # The code below may save $16-$23 (s0-s7), $28 (gp) and $30 (fp).
    # Comments will have rationales.
    sw      $24, 108(sp)
    sw      $25, 112(sp)

    mfc0    t4, C0_Cause
    mfc0    t5, C0_Status
    mfc0    t6, C0_EPC

    # These instructions are now very far away from anything that could have
    # caused writes to HI and LO.
    mfhi    t0                         # [while MFC0s run]
    mflo    t1
    sw      t5,  140(sp)               # [while MFHI/MFLO run]
    sw      t6,  144(sp)

    andi    t4, t4, 0x1F << 2
    sw      t0,  132(sp)               # save HI
    bne     t4, zero, is_exc_or_syscall
    sw      t1,  136(sp)               # (delay slot) save LO

    # - - - TAKING AN INTERRUPT - - -

    # Saved registers don't need to be saved. We are calling a C function that
    # will preserve their values for the caller (= this code right here). Even
    # nested interrupts will preserve the value for the caller (= the previous
    # interrupt handler) if interrupts get re-enabled, and so on, until we get
    # to the end with the caller's (= the code that got interrupted) values in
    # $16-$23 (s0-s7), $28 (gp) and $30 (fp).

    # Call the C interrupt handler (with interrupts disabled).
    jal     _irq_handler
    nop                                # cannot delay usefully here

    lw      t4,  144(sp)
    lw      t5,  140(sp)

    # Code may be awaiting a certain condition to happen at the end of an
    # interrupt (see DS2_StartAwait).
    lui     t7, 0x8000                 # [while LW t4 and t5 run]
    lw      t6, 0x1FFC(t7)
    mtc0    t4, C0_EPC                 # [while LW t6 runs]
    bne     t6, zero, ret_to_await
    mtc0    t5, C0_Status              # (delay slot)

restore_gprs:
    # These are the most likely to be needed immediately. Load them first to
    # give the loads more time to complete before returning to the code that
    # was interrupted.
    lw      $31, 128(k0)
    lw      sp,  120(k0)

    lw      k1,  132(k0)               # load HI
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
    lw      k1,  136(k0)               # load LO
    lw      $10,  52(k0)
    lw      $11,  56(k0)
    lw      $12,  60(k0)
    lw      $13,  64(k0)
    lw      $14,  68(k0)
    lw      $15,  72(k0)
    # Don't restore $16-$23 (s0-s7), $28 (gp) or $30 (fp), because they were
    # saved by the exception handler's callees.
    mtlo    k1
    lw      $24, 108(k0)
    lw      $25, 112(k0)

    eret

ret_to_await:
    # Here, we have code that is awaiting a condition.
    # None of the GPRs' values matter.
    lw      sp,  120(k0)
    mtc0    t6, C0_EPC
    eret

is_exc_or_syscall:
    # Add $16-$23 (s0-s7), $28 (gp) and $30 (fp) to the register save block,
    # even if they're callee-saved, because the exception or system call
    # handler will look at them.
    sw      $16,  76(sp)
    sw      $17,  80(sp)
    sw      $18,  84(sp)
    sw      $19,  88(sp)
    sw      $20,  92(sp)
    sw      $21,  96(sp)
    sw      $22, 100(sp)
    sw      $23, 104(sp)
    sw      $28, 116(sp)
    sw      $30, 124(sp)

    ori     t3, zero, 0x08 << 2        # 0x08 = SYSCALL exception number
    bne     t4, t3, is_exc             # it's not a SYSCALL?
    # Provide the called function with the saved register block (at 'sp' +
    # 16, size of the argument area) as argument 2 in either case.
    addiu   a1, sp, 16                 # (delay slot)

    # - - - TAKING A SYSTEM CALL - - -
    # SYSCALL has the system call number in the instruction word. Load it and
    # parse it out.
    lw      a0, 0(t6)                  # t6 = C0_EPC
    sll     a0, a0, 6                  # a0 = SYSCALL code; kill bits 26..31
    jal     _syscall_handler
    srl     a0, a0, 12                 # (delay slot) then kill bits 0..5

    lw      t4,  140(sp)
    lw      t5,  136(sp)
    addiu   t4, t4, 4                  # return at EPC + 4, after SYSCALL
    mtc0    t4, C0_EPC
    b       restore_gprs
    mtc0    t5, C0_Status              # (delay slot)

is_exc:
    # - - - TAKING AN EXCEPTION - - -
    jal     _exception_handler
    srl     a0, t4, 2                  # (delay slot) with the exception code as argument 1

    # Check the return value to see if the exception was handled.
    beq     v0, zero, exc_unhandled
    nop                                # cannot delay usefully here

    # Here, it was handled.
    lw      t4,  140(sp)
    lw      t5,  136(sp)
    mtc0    t4, C0_EPC
    b       restore_gprs
    mtc0    t5, C0_Status              # (delay slot)

exc_unhandled:
    # Here, the exception was NOT handled. Since the code that will send the
    # exception report to the DS needs to run with interrupts enabled, we set
    # EPC to its address and return from exception.
    # The function will run in the normal non-exception level with the saved
    # register block intact, but the values are not in the MIPS registers as
    # their values no longer matter.
    mfc0    t4, C0_Cause
    la      t6, _unhandled_exception
    mtc0    t6, C0_EPC
    andi    t4, t4, 0x1F << 2
    srl     a0, t4, 2                  # with the exception code as argument 1
    addiu   a1, sp, 16                 # and the saved registers as argument 2
    eret

    .end exception_entry
