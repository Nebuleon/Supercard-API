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

#ifndef __DS2_EXCEPT_H__
#define __DS2_EXCEPT_H__

/* TLB Refill exceptions are raised when a memory access goes through the TLB
 * and it has no mapping at all for the address, but the reference is proper
 * (aligned, not to kernel addresses in user mode, etc.). The exception code
 * for Interrupt is used for dispatch purposes. */
#define EXCODE_TLB_REFILL               0x00
/* TLB Modified exceptions are raised when a store to memory goes through a
 * TLB entry that does not allow writing. */
#define EXCODE_TLB_MODIFIED             0x01
/* TLB Load exceptions are raised when a load or instruction fetch fails to
 * resolve to a valid TLB entry. */
#define EXCODE_TLB_LOAD                 0x02
/* TLB Store exceptions are raised when a store to memory fails to resolve
 * to a valid TLB entry. */
#define EXCODE_TLB_STORE                0x03
/* Address Error exceptions are raised when a load or instruction fetch is
 * not naturally aligned, or when a user mode memory reference attempts to
 * access kernel memory. */
#define EXCODE_ADDRESS_ERROR_LOAD       0x04
/* Address Error exceptions are raised when a store to memory is not
 * naturally aligned, or when a user mode memory reference attempts to
 * access kernel memory. */
#define EXCODE_ADDRESS_ERROR_STORE      0x05
/* Bus Error exceptions are raised when the bus transaction for an instruction
 * fetch fails. */
#define EXCODE_BUS_ERROR_INSTRUCTION    0x06
/* Bus Error exceptions are raised when the bus transaction for a data access
 * fails. */
#define EXCODE_BUS_ERROR_DATA           0x07
/* Syscall exceptions are raised by the SYSCALL instruction. */
#define EXCODE_SYSCALL                  0x08
/* Breakpoint exceptions are raised by the BREAK instruction. */
#define EXCODE_BREAKPOINT               0x09
/* Reserved Instruction exceptions are raised by unrecognised instructions. */
#define EXCODE_RESERVED_INSTRUCTION     0x0A
/* Coprocessor Unusable exceptions are raised by instructions aimed at a
 * disabled coprocessor in user mode. */
#define EXCODE_COPROCESSOR_UNUSABLE     0x0B
/* Arithmetic Overflow exceptions are raised by ADD, SUB and ADDI if the sign
 * of the two operands is the same and the sign of the result is different. */
#define EXCODE_ARITHMETIC_OVERFLOW      0x0C
/* Trap exceptions are raised by T* instructions if they fail. */
#define EXCODE_TRAP                     0x0D
/* Floating-point exceptions are raised in the circumstances prescribed by
 * IEEE 754. */
#define EXCODE_FLOATING_POINT           0x0F
#define EXCODE_COPROCESSOR_2            0x12
/* Watch reference exceptions are raised when the address in WatchHi/WatchLo
 * is referenced. */
#define EXCODE_WATCH_REFERENCE          0x17
#define EXCODE_MACHINE_CHECK            0x18
/* Cache Error exceptions are raised when the cache returns invalid data. A
 * Cache Error exception does not update the Coprocessor 0 Cause register,
 * but the exception code is used for dispatch purposes. */
#define EXCODE_CACHE_ERROR              0x1E

#if !defined __ASSEMBLY__
#  include <stdbool.h>

typedef void (*exception_handler) (void);

struct DS2_ExceptionRegisters {
	unsigned long at;
	unsigned long v0;
	unsigned long v1;
	unsigned long a0;
	unsigned long a1;
	unsigned long a2;
	unsigned long a3;
	unsigned long t0;
	unsigned long t1;
	unsigned long t2;
	unsigned long t3;
	unsigned long t4;
	unsigned long t5;
	unsigned long t6;
	unsigned long t7;
	unsigned long s0;
	unsigned long s1;
	unsigned long s2;
	unsigned long s3;
	unsigned long s4;
	unsigned long s5;
	unsigned long s6;
	unsigned long s7;
	unsigned long t8;
	unsigned long t9;
	unsigned long gp;
	unsigned long sp;
	unsigned long fp;
	unsigned long ra;
	unsigned long hi;    /* Multiplier unit HI */
	unsigned long lo;    /* Multiplier unit LO */
};

typedef void (*c_exception_handler) (unsigned int excode, struct DS2_ExceptionRegisters* regs);

/*
 * Sets the function to be used to handle a certain kind of exception on the
 * Supercard.
 *
 * To allow for maximum efficiency, the exception handler is entered without
 * saving any registers; it's up to the handler to save the ones it wants to
 * use. However, all registers that are used must be saved, not just callee-
 * saved registers, if the handler wishes to return to user code, so handler
 * functions should consist of:
 *
 * - assembly functions that save registers, have a JAL to a C function, and
 *   restore registers before returning (with ERET);
 * - pure assembly functions that save and restore registers.
 *
 * As some registers are needed to do anything on MIPS, and they must all be
 * saved, the ABI reserves $k0 ($26) and $k1 ($27) for handlers. The handler
 * gets the exception code in bits 6..2 of $k1 ($27) and can load the 'data'
 * passed in to this function in _exception_data through it.
 *
 * Due to how they are run, i.e. with interrupts disabled, handlers must not
 * do anything that awaits a condition, including, but not limited to:
 *
 * - writing to stdout or stderr;
 * - displaying to the screen and awaiting a screen update;
 * - displaying to a screen that is still being sent to the Nintendo DS;
 * - awaiting an input change.
 *
 * They must also not do anything that would alter user data, including, but
 * not limited to:
 *
 * - allocating memory, because user code may be doing the same (but using a
 *   dedicated allocation is fine);
 * - changing the contents or current position of a file opened by user code
 *   (but doing this on a file dedicated to exception logging is fine).
 *
 * If they need stack space, they should move the stack to 0x8000_1FE0 after
 * saving the old value of $sp ($29) there.
 *
 * In:
 *   excode: The EXCODE_* value corresponding to the kind of exception to add
 *     a handler for.
 *   handler: The handler to be added:
 *     - NULL: No handler. In that case, if the exception is raised, details
 *       of it will be sent to the Nintendo DS and the Supercard's execution
 *       will be suspended.
 *     - non-NULL: If the exception is raised, the handler is called; it may
 *       either end with ERET or suspend execution on the Supercard.
 *   data: Data to be associated with the handler.
 */
void DS2_SetExceptionHandler(unsigned int excode, exception_handler handler, void* data);

/*
 * Sets the function to be used to handle a certain kind of exception on the
 * Supercard.
 *
 * The function is given the exception code in argument 1, and the values in
 * the MIPS integer registers at the time of the exception in argument 2. It
 * can manipulate Coprocessor 0 registers using the read_c0_* and write_c0_*
 * functions in <mipsregs.h>. The MIPS integer registers may be modified via
 * argument 2; their new values are applied before returning to user code.
 *
 * Due to how they are run, i.e. with interrupts disabled, handlers must not
 * do anything that awaits a condition, including, but not limited to:
 *
 * - writing to stdout or stderr;
 * - displaying to the screen and awaiting a screen update;
 * - displaying to a screen that is still being sent to the Nintendo DS;
 * - awaiting an input change.
 *
 * They must also not do anything that would alter user data, including, but
 * not limited to:
 *
 * - allocating memory, because user code may be doing the same (but using a
 *   dedicated allocation is fine);
 * - changing the contents or current position of a file opened by user code
 *   (but doing this on a file dedicated to exception logging is fine).
 *
 * The handler is guaranteed to be able to use 6,144 bytes of stack space.
 *
 * In:
 *   excode: The EXCODE_* value corresponding to the kind of exception to add
 *     a handler for.
 *   handler: The handler to be added:
 *     - NULL: No handler. In that case, if the exception is raised, details
 *       of it will be sent to the Nintendo DS and the Supercard's execution
 *       will be suspended.
 *     - non-NULL: If the exception is raised, the handler is called; if the
 *       handler returns, ERET is then executed to return to user code.
 */
void DS2_SetCExceptionHandler(unsigned int excode, c_exception_handler handler);

#endif /* !__ASSEMBLY__ */

#endif /* !__DS2_EXCEPT_H__ */
