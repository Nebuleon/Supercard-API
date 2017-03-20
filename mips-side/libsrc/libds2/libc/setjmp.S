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

    .ent     setjmp
    .global  setjmp
    .type    setjmp,@function

    /* int setjmp(jmp_buf env)
     * Saves the caller's execution environment for later returning there via
     * longjmp.
     *
     * Out:
     *   argument 1: Pointer to an opaque structure where the execution
     *     environment will be stored.
     * Returns:
     *   0. Later calls to longjmp with the values saved in 'env' will make it
     *   appear as if that call made this call to setjmp return again.
     */
setjmp:
    # First save the registers that shouldn't be being loaded with LB/LH/LW,
    # to give more time for the other registers to be loaded, if applicable.
    sw      sp, 36(a0)
    sw      ra, 44(a0)

    # Then save the execution environment. Note that, because setjmp is called
    # as a function, any values that the caller still cares about will not be
    # in caller-saved registers anymore; they will be on the stack (we saved
    # the stack pointer) or in callee-saved registers, saved below.
    sw      $16,  0(a0)
    sw      $17,  4(a0)
    sw      $18,  8(a0)
    sw      $19, 12(a0)
    sw      $20, 16(a0)
    sw      $21, 20(a0)
    sw      $22, 24(a0)
    sw      $23, 28(a0)
    sw      $28, 32(a0)
    sw      $30, 40(a0)

    jr      ra
    or      v0, zero, zero             # (delay slot) this returns 0

    .end     setjmp

    .ent     longjmp
    .global  longjmp
    .type    longjmp,@function

    /* int longjmp(jmp_buf env, int ret)
     * Restores the execution environment from a prior call to setjmp.
     *
     * In:
     *   argument 1: Pointer to an opaque structure containing the execution
     *     environment to be restored.
     *   argument 2: Value to be returned. If this is 0, longjmp behaves as if
     *   it were 1.
     * Returns:
     *   As if the call to 'setjmp' saved in 'env' returned again, with the
     *   value in 'ret' (or 1, if it was 0).
     */
longjmp:
    # First load the registers that will be required immediately after
    # returning, to give the loads more time to complete.
    lw      sp, 36(a0)
    lw      ra, 44(a0)

    lw      $16,  0(a0)
    lw      $17,  4(a0)
    lw      $18,  8(a0)
    lw      $19, 12(a0)
    lw      $20, 16(a0)
    lw      $21, 20(a0)
    lw      $22, 24(a0)
    lw      $23, 28(a0)
    lw      $28, 32(a0)
    lw      $30, 40(a0)

    sltiu   v0, a1, 1                  # ret == 0 ? 1 : 0
    jr      ra                         # return to the caller of setjmp
    addu    v0, v0, a1                 # (delay slot) ret == 0 ? 1 : ret

    .end     longjmp
