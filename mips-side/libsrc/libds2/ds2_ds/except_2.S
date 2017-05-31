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

    .set     noreorder
    .set     noat

    .section .text._unhandled_exception_save, "ax", %progbits

    .extern  _unhandled_exception

    .ent     _unhandled_exception_save
    .global  _unhandled_exception_save
    .type    _unhandled_exception_save,@function

_unhandled_exception_save:
    # For the layout of this area, see ds2_ds/except.h.
    la      k0, 0x80001FE0 - 160
    sw      sp,  120(k0)               # Store the old stack pointer
    or      sp, k0, zero               # Change to the new one

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
    sw      $24, 108(sp)
    sw      $25, 112(sp)

    mfc0    v0, C0_Cause
    mfc0    v1, C0_Status
    mfc0    t2, C0_EPC

    # These instructions are now very far away from anything that could have
    # caused writes to HI and LO.
    mfhi    t0                         # [while MFC0s run]
    mflo    t1
    sw      v1,  140(sp)               # [while MFHI/MFLO run]
    sw      t2,  144(sp)
    andi    v0, v0, 0x1F << 2          # keep only excode from C0 Cause

    sw      t0,  132(sp)               # save HI
    sw      t1,  136(sp)               # save LO

    # Since the code that will send the exception report to the DS needs to
    # run with interrupts enabled, we set EPC to its address and return from
    # exception with the stack still below 0x8000_1FE0.
    la      t2, _unhandled_exception
    mtc0    t2, C0_EPC
    srl     a0, v0, 2                  # with the exception code as argument 1
    addiu   a1, sp, 16                 # and the saved registers as argument 2
    eret

    .end     _unhandled_exception_save

    .section .text._c_exception_save, "ax", %progbits

    .extern  _exception_data

    .ent     _c_exception_save
    .global  _c_exception_save
    .type    _c_exception_save,@function

_c_exception_save:
    # For the layout of this area, see <ds2/except.h>.
    la      k0, 0x80001FE0 - 144
    pref    30,   16(k0)               # PrepareForStore: clear cache lines
    pref    30,   48(k0)               # without loading their data from RAM
    pref    30,   80(k0)
    pref    30,  112(k0)
    sw      sp,  120(k0)               # Store the old stack pointer
    or      sp, k0, zero               # Change to the new one

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
    sw      $24, 108(sp)
    sw      $25, 112(sp)

    # These instructions are now very far away from anything that could have
    # caused writes to HI and LO.
    mfhi    v1
    mflo    t0
    la      v0, _exception_data        # [while MFHI/MFLO run]
    addu    v0, v0, k1                 # v0 = &_exception_data[excode]
    lw      v0, 0(v0)                  # v0 = _exception_data[excode]
    srl     a0, k1, 2                  # argument 1 = exception code
    addiu   a1, sp, 16                 # argument 2 = saved registers

    sw      v1,  132(sp)               # save HI

    jalr    v0
    sw      t0,  136(sp)               # (delay slot) save LO

    # Restore the (possibly modified) values of MIPS integer registers.

    # These are the most likely to be needed immediately. Load them first to
    # give the loads more time to complete before returning to the code that
    # caused the exception.
    lw      sp,  120(k0)
    lw      $31, 128(k0)

    lw      k1,  132(k0)               # load HI
    lw      $1,   16(k0)
    lw      $2,   20(k0)
    lw      $3,   24(k0)
    lw      $4,   28(k0)
    lw      $5,   32(k0)
    lw      $6,   36(k0)
    lw      $7,   40(k0)
    lw      $8,   44(k0)
    lw      $9,   48(k0)
    mthi    k1
    lw      $10,  52(k0)
    lw      $11,  56(k0)
    lw      k1,  136(k0)               # load LO
    lw      $12,  60(k0)
    lw      $13,  64(k0)
    lw      $14,  68(k0)
    lw      $15,  72(k0)
    lw      $16,  76(k0)
    lw      $17,  80(k0)
    lw      $18,  84(k0)
    lw      $19,  88(k0)
    lw      $20,  92(k0)
    mtlo    k1
    lw      $21,  96(k0)
    lw      $22, 100(k0)
    lw      $23, 104(k0)
    lw      $28, 116(k0)
    lw      $30, 124(k0)
    lw      $24, 108(k0)
    lw      $25, 112(k0)

    # These values will never be needed again, both by kernel code and by user
    # code. To avoid user code needing to write them back, even when it's only
    # reading memory and evicting one of our dirty lines, invalidate them. The
    # DCHitInv encoding is optional, but we're only using it for optimisation.
    cache   DCHitInv,  16(k0)
    cache   DCHitInv,  48(k0)
    cache   DCHitInv,  80(k0)
    cache   DCHitInv, 112(k0)

    eret

    .end     _c_exception_save
