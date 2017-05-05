/*
 * This file is part of the DS communication library for the Supercard DSTwo.
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

#include <nds.h>
#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "audio.h"
#include "card_protocol.h"
#include "video.h"

/*
 * mips_except.c: Displays information about an exception that was raised on
 * the Supercard, including a disassembly of instructions at the point where
 * the exception was raised, and the values of all registers.
 *
 * The canonical form of all instructions is shown, and the short form of an
 * instruction is shown if a compiler would emit a canonical instruction for
 * it:
 *
 * - NOP for SLL $0, $0, 0;
 * - LI Rt, Imm16 for ORI Rt, $0, Imm16, ADDIU Rt, $0, Imm16 and OR Rd, $0, $0;
 * - MOVE Rd, Rs for OR Rd, Rs, $0 and ADDU Rd, Rs, $0;
 * - B Target for BEQ $0, $0, Target, BLEZ $0, Target and BGEZ $0, Target;
 * - BAL Target for BGEZAL $0, Target.
 *
 * Other instructions may be used for those purposes, but are much rarer. To
 * display short forms for other instructions, define UNLIKELY_SHORT_FORMS.
 */

#define REG_COUNT 29

static const uint8_t reg_nums[REG_COUNT] = {
	 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16,
	17, 18, 19, 20, 21, 22, 23, 24, 25, 28, 29, 30, 31
};

static const size_t reg_offs[REG_COUNT] = {
	offsetof(struct card_reply_mips_exception, at),
	offsetof(struct card_reply_mips_exception, v0),
	offsetof(struct card_reply_mips_exception, v1),
	offsetof(struct card_reply_mips_exception, a0),
	offsetof(struct card_reply_mips_exception, a1),
	offsetof(struct card_reply_mips_exception, a2),
	offsetof(struct card_reply_mips_exception, a3),
	offsetof(struct card_reply_mips_exception, t0),
	offsetof(struct card_reply_mips_exception, t1),
	offsetof(struct card_reply_mips_exception, t2),
	offsetof(struct card_reply_mips_exception, t3),
	offsetof(struct card_reply_mips_exception, t4),
	offsetof(struct card_reply_mips_exception, t5),
	offsetof(struct card_reply_mips_exception, t6),
	offsetof(struct card_reply_mips_exception, t7),
	offsetof(struct card_reply_mips_exception, s0),
	offsetof(struct card_reply_mips_exception, s1),
	offsetof(struct card_reply_mips_exception, s2),
	offsetof(struct card_reply_mips_exception, s3),
	offsetof(struct card_reply_mips_exception, s4),
	offsetof(struct card_reply_mips_exception, s5),
	offsetof(struct card_reply_mips_exception, s6),
	offsetof(struct card_reply_mips_exception, s7),
	offsetof(struct card_reply_mips_exception, t8),
	offsetof(struct card_reply_mips_exception, t9),
	offsetof(struct card_reply_mips_exception, gp),
	offsetof(struct card_reply_mips_exception, sp),
	offsetof(struct card_reply_mips_exception, fp),
	offsetof(struct card_reply_mips_exception, ra)
};

static const char* excode_desc(uint32_t excode)
{
	switch (excode) {
	case  1: return "Store disallowed by TLB";
	case  2: return "TLB load exception";
	case  3: return "TLB store exception";
	case  4: return "Unaligned load";
	case  5: return "Unaligned store";
	case  6: return "Instruction fetch bus error";
	case  7: return "Data reference bus error";
	case  9: return "Breakpoint";
	case 10: return "Unknown instruction";
	case 11: return "Coprocessor unusable";
	case 12: return "Arithmetic overflow";
	case 13: return "Trap";
	case 15: return "Floating point exception";
	case 18: return "Coprocessor 2 exception";
	case 23: return "Watched data reference";
	case 24: return "Machine check exception";
	case 30: return "Cache consistency error";
	default: return "Unknown exception";
	}
}

static bool disassemble_mips32_special(uint32_t op, uint32_t loc, char* shrt, char* canon);
static bool disassemble_mips32_regimm(uint32_t op, uint32_t loc, char* shrt, char* canon);
static bool disassemble_mips32_cop0(uint32_t op, uint32_t loc, char* shrt, char* canon);
static bool disassemble_mips32_pref(uint32_t op, uint32_t loc, char* shrt, char* canon);
static bool disassemble_mips32_cache(uint32_t op, uint32_t loc, char* shrt, char* canon);
static bool disassemble_mips32_i(uint32_t op, uint32_t loc, char* shrt, char* canon);
static bool disassemble_mips32_special2(uint32_t op, uint32_t loc, char* shrt, char* canon);
static bool disassemble_mips32_j(uint32_t op, uint32_t loc, char* shrt, char* canon);

static uint32_t rel_jump(uint32_t loc, int16_t rel)
{
	return loc + 4 + (int32_t) rel * 4;
}

static uint32_t abs_jump(uint32_t loc, uint32_t imm26)
{
	return ((loc + 4) & UINT32_C(0xF0000000)) | (imm26 << 2);
}

/*
 * Disassembles a MIPS32 instruction. Both the shortest form accepted by an
 * assembler and the canonical representation of the instruction are output.
 * shrt and canon are both expected to point to 64-byte buffers.
 * If the instruction is not recognised, the 32-bit value encoding the
 * instruction is output in upper-case hexadecimal form between angle brackets
 * ('<', '>') to 'canon', and "?" is output to 'shrt'.
 */
static void disassemble_mips32(uint32_t op, uint32_t loc, char* shrt, char* canon)
{
	uint32_t op_maj = (op >> 26) & 0x3F;
	bool valid = false;
	switch (op_maj) {
	case 0x00:  // SPECIAL prefix
		valid = disassemble_mips32_special(op, loc, shrt, canon);
		break;
	case 0x01:  // REGIMM prefix
		valid = disassemble_mips32_regimm(op, loc, shrt, canon);
		break;
	case 0x04:  // I-type
	case 0x05:
	case 0x06:
	case 0x07:
	case 0x08:
	case 0x09:
	case 0x0A:
	case 0x0B:
	case 0x0C:
	case 0x0D:
	case 0x0E:
	case 0x0F:
	case 0x14:
	case 0x15:
	case 0x16:
	case 0x17:
	case 0x20:
	case 0x21:
	case 0x22:
	case 0x23:
	case 0x24:
	case 0x25:
	case 0x26:
	case 0x28:
	case 0x29:
	case 0x2A:
	case 0x2B:
	case 0x2E:
	case 0x30:
	case 0x38:
		valid = disassemble_mips32_i(op, loc, shrt, canon);
		break;
	case 0x02:  // J-type
	case 0x03:
		valid = disassemble_mips32_j(op, loc, shrt, canon);
		break;
	case 0x10:  // Coprocessor 0
		valid = disassemble_mips32_cop0(op, loc, shrt, canon);
		break;
	case 0x1C:  // SPECIAL2 prefix
		valid = disassemble_mips32_special2(op, loc, shrt, canon);
		break;
	case 0x2F:
		valid = disassemble_mips32_cache(op, loc, shrt, canon);
		break;
	case 0x33:
		valid = disassemble_mips32_pref(op, loc, shrt, canon);
		break;
	}
	if (!valid) {
		siprintf(canon, "<%08" PRIX32 ">", op);
		strcpy(shrt, "?");
	}
}

/*
 * Disassembles a MIPS32 SPECIAL prefix instruction.
 * Returns true and fills both buffers if the instruction is defined.
 * Returns false if the instruction is undefined.
 */
static bool disassemble_mips32_special(uint32_t op, uint32_t loc, char* shrt, char* canon)
{
	uint32_t s = (op >> 21) & 0x1F,
		t = (op >> 16) & 0x1F,
		d = (op >> 11) & 0x1F,
		sa = (op >> 6) & 0x1F,
		op_min = op & 0x3F;
	switch (op_min) {
	case 0x00:  // SLL RD, RT, SA
		siprintf(canon, "sll $%" PRIu32 ", $%" PRIu32 ", %" PRIu32, d, t, sa);
		if (d == 0)  // Effectively a nop
			strcpy(shrt, "nop");
#if defined UNLIKELY_SHORT_FORMS
		else if (t == 0)  // Moving zero into RD
			siprintf(shrt, "li $%" PRIu32 ", 0", d);
		else if (sa == 0)  // Moving RT into RD
			siprintf(canon, "move $%" PRIu32 ", $%" PRIu32, d, t);
#endif
		else
			strcpy(shrt, canon);
		return true;
	case 0x02:  // SRL RD, RT, SA
		siprintf(canon, "srl $%" PRIu32 ", $%" PRIu32 ", %" PRIu32, d, t, sa);
#if defined UNLIKELY_SHORT_FORMS
		if (d == 0)  // Effectively a nop
			strcpy(shrt, "nop");
		else if (t == 0)  // Moving zero into RD
			siprintf(shrt, "li $%" PRIu32 ", 0", d);
		else if (sa == 0)  // Moving RT into RD
			siprintf(canon, "move $%" PRIu32 ", $%" PRIu32, d, t);
		else
#endif
			strcpy(shrt, canon);
		return true;
	case 0x03:  // SRA RD, RT, SA
		siprintf(canon, "sra $%" PRIu32 ", $%" PRIu32 ", %" PRIu32, d, t, sa);
#if defined UNLIKELY_SHORT_FORMS
		if (d == 0)  // Effectively a nop
			strcpy(shrt, "nop");
		else if (t == 0)  // Moving zero into RD
			siprintf(shrt, "li $%" PRIu32 ", 0", d);
		else if (sa == 0)  // Moving RT into RD
			siprintf(canon, "move $%" PRIu32 ", $%" PRIu32, d, t);
		else
#endif
			strcpy(shrt, canon);
		return true;
	case 0x04:  // SLLV RD, RT, RS
		siprintf(canon, "sllv $%" PRIu32 ", $%" PRIu32 ", $%" PRIu32, d, t, s);
#if defined UNLIKELY_SHORT_FORMS
		if (d == 0)  // Effectively a nop
			strcpy(shrt, "nop");
		else if (t == 0)  // Moving zero into RD
			siprintf(shrt, "li $%" PRIu32 ", 0", d);
		else if (s == 0)  // Moving RT into RD
			siprintf(canon, "move $%" PRIu32 ", $%" PRIu32, d, t);
		else
#endif
			strcpy(shrt, canon);
		return true;
	case 0x06:  // SRLV RD, RT, SA
		siprintf(canon, "srlv $%" PRIu32 ", $%" PRIu32 ", $%" PRIu32, d, t, s);
#if defined UNLIKELY_SHORT_FORMS
		if (d == 0)  // Effectively a nop
			strcpy(shrt, "nop");
		else if (t == 0)  // Moving zero into RD
			siprintf(shrt, "li $%" PRIu32 ", 0", d);
		else if (s == 0)  // Moving RT into RD
			siprintf(canon, "move $%" PRIu32 ", $%" PRIu32, d, t);
		else
#endif
			strcpy(shrt, canon);
		return true;
	case 0x07:  // SRAV RD, RT, SA
		siprintf(canon, "srav $%" PRIu32 ", $%" PRIu32 ", $%" PRIu32, d, t, s);
#if defined UNLIKELY_SHORT_FORMS
		if (d == 0)  // Effectively a nop
			strcpy(shrt, "nop");
		else if (t == 0)  // Moving zero into RD
			siprintf(shrt, "li $%" PRIu32 ", 0", d);
		else if (s == 0)  // Moving RT into RD
			siprintf(canon, "move $%" PRIu32 ", $%" PRIu32, d, t);
		else
#endif
			strcpy(shrt, canon);
		return true;
	case 0x08:  // JR RS
		siprintf(canon, "jr $%" PRIu32, s);
		strcpy(shrt, canon);
		return true;
	case 0x09:  // JALR RD, RS
		siprintf(canon, "jalr $%" PRIu32 ", $%" PRIu32, d, s);
		strcpy(shrt, canon);
		return true;
	case 0x0A:  // MOVZ RD, RS, RT
		siprintf(canon, "movz $%" PRIu32 ", $%" PRIu32 ", $%" PRIu32, d, s, t);
#if defined UNLIKELY_SHORT_FORMS
		if (d == 0 || d == s)  // Effectively a nop
			strcpy(shrt, "nop");
		else if (t == 0)  // Moving RS into RD
			siprintf(shrt, "move $%" PRIu32 ", $%" PRIu32, d, s);
		else
#endif
			strcpy(shrt, canon);
		return true;
	case 0x0B:  // MOVN RD, RS, RT
		siprintf(canon, "movn $%" PRIu32 ", $%" PRIu32 ", $%" PRIu32, d, s, t);
#if defined UNLIKELY_SHORT_FORMS
		if (d == 0 || t == 0 || d == s)  // Effectively a nop
			strcpy(shrt, "nop");
		else
#endif
			strcpy(shrt, canon);
		return true;
	case 0x0C:  // SYSCALL imm20
		siprintf(canon, "syscall 0x%" PRIX32, (op >> 6) & UINT32_C(0xFFFFF));
		strcpy(shrt, canon);
		return true;
	case 0x0D:  // BREAK imm20
		siprintf(canon, "break 0x%" PRIX32, (op >> 6) & UINT32_C(0xFFFFF));
		strcpy(shrt, canon);
		return true;
	case 0x0F:  // SYNC stype (the same field as sa)
		siprintf(canon, "sync 0x%" PRIX32, sa);
		strcpy(shrt, canon);
		return true;
	case 0x10:  // MFHI RD
		siprintf(canon, "mfhi $%" PRIu32, d);
#if defined UNLIKELY_SHORT_FORMS
		if (d == 0)  // Effectively a nop
			strcpy(shrt, "nop");
		else
#endif
			strcpy(shrt, canon);
		return true;
	case 0x11:  // MTHI RS
		siprintf(canon, "mthi $%" PRIu32, s);
		strcpy(shrt, canon);
		return true;
	case 0x12:  // MFLO RD
		siprintf(canon, "mflo $%" PRIu32, d);
#if defined UNLIKELY_SHORT_FORMS
		if (d == 0)  // Effectively a nop
			strcpy(shrt, "nop");
		else
#endif
			strcpy(shrt, canon);
		return true;
	case 0x13:  // MTLO RS
		siprintf(canon, "mtlo $%" PRIu32, s);
		strcpy(shrt, canon);
		return true;
	case 0x18:  // MULT RS, RT
		siprintf(canon, "mult $%" PRIu32 ", $%" PRIu32, s, t);
		strcpy(shrt, canon);
		return true;
	case 0x19:  // MULTU RS, RT
		siprintf(canon, "multu $%" PRIu32 ", $%" PRIu32, s, t);
		strcpy(shrt, canon);
		return true;
	case 0x1A:  // DIV RS, RT
		siprintf(canon, "div $%" PRIu32 ", $%" PRIu32, s, t);
		strcpy(shrt, canon);
		return true;
	case 0x1B:  // DIVU RS, RT
		siprintf(canon, "divu $%" PRIu32 ", $%" PRIu32, s, t);
		strcpy(shrt, canon);
		return true;
	case 0x20:  // ADD RD, RS, RT
		siprintf(canon, "add $%" PRIu32 ", $%" PRIu32 ", $%" PRIu32, d, s, t);
#if defined UNLIKELY_SHORT_FORMS
		if (d == 0 || (d == s && s != 0 && t == 0)
		 || (d == t && t != 0 && s == 0))  // Effectively a nop
			strcpy(shrt, "nop");
		else if (s == 0 && t == 0)  // Moving zero into RD
			siprintf(shrt, "li $%" PRIu32 ", 0", d);
		else if (s == 0)  // Moving RT into RD
			siprintf(shrt, "move $%" PRIu32 ", $%" PRIu32, d, t);
		else if (t == 0)  // Moving RS into RD
			siprintf(shrt, "move $%" PRIu32 ", $%" PRIu32, d, s);
		else
#endif
			strcpy(shrt, canon);
		return true;
	case 0x21:  // ADDU RD, RS, RT
		siprintf(canon, "addu $%" PRIu32 ", $%" PRIu32 ", $%" PRIu32, d, s, t);
#if defined UNLIKELY_SHORT_FORMS
		if (d == 0 || (d == s && s != 0 && t == 0)
		 || (d == t && t != 0 && s == 0))  // Effectively a nop
			strcpy(shrt, "nop");
		else if (s == 0 && t == 0)  // Moving zero into RD
			siprintf(shrt, "li $%" PRIu32 ", 0", d);
		else
#endif
		if (s == 0)  // Moving RT into RD
			siprintf(shrt, "move $%" PRIu32 ", $%" PRIu32, d, t);
		else if (t == 0)  // Moving RS into RD
			siprintf(shrt, "move $%" PRIu32 ", $%" PRIu32, d, s);
		else
			strcpy(shrt, canon);
		return true;
	case 0x22:  // SUB RD, RS, RT
		siprintf(canon, "sub $%" PRIu32 ", $%" PRIu32 ", $%" PRIu32, d, s, t);
#if defined UNLIKELY_SHORT_FORMS
		if (d == 0 || (d == s && s != 0 && t == 0))  // Effectively a nop
			strcpy(shrt, "nop");
		else if (s == t)  // Moving zero into RD
			siprintf(shrt, "li $%" PRIu32 ", 0", d);
		else if (t == 0)  // Moving RS into RD
			siprintf(shrt, "move $%" PRIu32 ", $%" PRIu32, d, s);
		else
#endif
		if (s == 0)  // Negating RT into RD
			siprintf(shrt, "negu $%" PRIu32 ", $%" PRIu32, d, t);
		else
			strcpy(shrt, canon);
		return true;
	case 0x23:  // SUBU RD, RS, RT
		siprintf(canon, "subu $%" PRIu32 ", $%" PRIu32 ", $%" PRIu32, d, s, t);
#if defined UNLIKELY_SHORT_FORMS
		if (d == 0 || (d == s && s != 0 && t == 0))  // Effectively a nop
			strcpy(shrt, "nop");
		else if (s == t)  // Moving zero into RD
			siprintf(shrt, "li $%" PRIu32 ", 0", d);
		else if (t == 0)  // Moving RS into RD
			siprintf(shrt, "move $%" PRIu32 ", $%" PRIu32, d, s);
		else
#endif
		if (s == 0)  // Negating RT into RD
			siprintf(shrt, "negu $%" PRIu32 ", $%" PRIu32, d, t);
		else
			strcpy(shrt, canon);
		return true;
	case 0x24:  // AND RD, RS, RT
		siprintf(canon, "and $%" PRIu32 ", $%" PRIu32 ", $%" PRIu32, d, s, t);
#if defined UNLIKELY_SHORT_FORMS
		if (d == 0 || (d == s && s == t))  // Effectively a nop
			strcpy(shrt, "nop");
		else if (s == 0 || t == 0)  // Moving zero into RD
			siprintf(shrt, "li $%" PRIu32 ", 0", d);
		else if (s == t)  // Moving RS (== RT) into RD
			siprintf(shrt, "move $%" PRIu32 ", $%" PRIu32, d, s);
		else
#endif
			strcpy(shrt, canon);
		return true;
	case 0x25:  // OR RD, RS, RT
		siprintf(canon, "or $%" PRIu32 ", $%" PRIu32 ", $%" PRIu32, d, s, t);
#if defined UNLIKELY_SHORT_FORMS
		if (d == 0 || (d == s && (t == s || t == 0))
		 || (d == t && (s == t || s == 0)))  // Effectively a nop
			strcpy(shrt, "nop");
		else
#endif
		if (s == t) {  // Moving RS (== RT) into RD
			if (s == 0)  // Moving zero into RD
				siprintf(shrt, "li $%" PRIu32 ", 0", d);
			else
				siprintf(shrt, "move $%" PRIu32 ", $%" PRIu32, d, s);
		} else if (s == 0 /* && t != 0 */)  // Moving RT into RD
			siprintf(shrt, "move $%" PRIu32 ", $%" PRIu32, d, t);
		else if (t == 0 /* && s != 0 */)  // Moving RS into RD
			siprintf(shrt, "move $%" PRIu32 ", $%" PRIu32, d, s);
		else
			strcpy(shrt, canon);
		return true;
	case 0x26:  // XOR RD, RS, RT
		siprintf(canon, "xor $%" PRIu32 ", $%" PRIu32 ", $%" PRIu32, d, s, t);
#if defined UNLIKELY_SHORT_FORMS
		if (d == 0 || (d == s && t == 0) || (d == t && s == 0))  // Effectively a nop
			strcpy(shrt, "nop");
		else if (s == t)  // Moving zero into RD
			siprintf(shrt, "li $%" PRIu32 ", 0", d);
		else
#endif
			strcpy(shrt, canon);
		return true;
	case 0x27:  // NOR RD, RS, RT
		siprintf(canon, "nor $%" PRIu32 ", $%" PRIu32 ", $%" PRIu32, d, s, t);
#if defined UNLIKELY_SHORT_FORMS
		if (d == 0)  // Effectively a nop
			strcpy(shrt, "nop");
		else
#endif
		if (s == 0 && t == 0)  // Loading -1 into RD
			siprintf(shrt, "li $%" PRIu32 ", -1", d);
		else
			strcpy(shrt, canon);
		return true;
	case 0x2A:  // SLT RD, RS, RT
		siprintf(canon, "slt $%" PRIu32 ", $%" PRIu32 ", $%" PRIu32, d, s, t);
#if defined UNLIKELY_SHORT_FORMS
		if (d == 0)  // Effectively a nop
			strcpy(shrt, "nop");
		else if (s == t)  // Moving zero into RD (Reg < Reg: 0)
			siprintf(shrt, "li $%" PRIu32 ", 0", d);
		else
#endif
			strcpy(shrt, canon);
		return true;
	case 0x2B:  // SLTU RD, RS, RT
		siprintf(canon, "sltu $%" PRIu32 ", $%" PRIu32 ", $%" PRIu32, d, s, t);
#if defined UNLIKELY_SHORT_FORMS
		if (d == 0)  // Effectively a nop
			strcpy(shrt, "nop");
		else if (s == t)  // Moving zero into RD (Reg < Reg: 0)
			siprintf(shrt, "li $%" PRIu32 ", 0", d);
		else
#endif
			strcpy(shrt, canon);
		return true;
	case 0x30:  // TGE RS, RT
		siprintf(canon, "tge $%" PRIu32 ", $%" PRIu32, s, t);
		strcpy(shrt, canon);
		return true;
	case 0x31:  // TGEU RS, RT
		siprintf(canon, "tgeu $%" PRIu32 ", $%" PRIu32, s, t);
		strcpy(shrt, canon);
		return true;
	case 0x32:  // TLT RS, RT
		siprintf(canon, "tlt $%" PRIu32 ", $%" PRIu32, s, t);
		strcpy(shrt, canon);
		return true;
	case 0x33:  // TLTU RS, RT
		siprintf(canon, "tltu $%" PRIu32 ", $%" PRIu32, s, t);
		strcpy(shrt, canon);
		return true;
	case 0x34:  // TEQ RS, RT
		siprintf(canon, "teq $%" PRIu32 ", $%" PRIu32, s, t);
		strcpy(shrt, canon);
		return true;
	case 0x36:  // TNE RS, RT
		siprintf(canon, "tne $%" PRIu32 ", $%" PRIu32, s, t);
		strcpy(shrt, canon);
		return true;
	default:
		return false;
	}
}

/*
 * Disassembles a MIPS32 REGIMM prefix instruction.
 * Returns true and fills both buffers if the instruction is defined.
 * Returns false if the instruction is undefined.
 */
static bool disassemble_mips32_regimm(uint32_t op, uint32_t loc, char* shrt, char* canon)
{
	uint32_t s = (op >> 21) & 0x1F,
		op_min = (op >> 16) & 0x1F;
	uint16_t imm = op;
	switch (op_min) {
	case 0x00:  // BLTZ RS, PC+IMM+4
		siprintf(canon, "bltz $%" PRIu32 ", 0x%08" PRIX32, s, rel_jump(loc, (int16_t) imm));
#if defined UNLIKELY_SHORT_FORMS
		if (s == 0)  // Effectively a nop with a branch delay slot
			strcpy(shrt, "nop");
		else
#endif
			strcpy(shrt, canon);
		return true;
	case 0x01:  // BGEZ RS, PC+IMM+4
		siprintf(canon, "bgez $%" PRIu32 ", 0x%08" PRIX32, s, rel_jump(loc, (int16_t) imm));
		if (s == 0)  // Effectively an unconditional immediate jump
			siprintf(shrt, "b %" PRIX32, rel_jump(loc, (int16_t) imm));
		else
			strcpy(shrt, canon);
		return true;
	case 0x02:  // BLTZL RS, PC+IMM+4
		siprintf(canon, "bltzl $%" PRIu32 ", 0x%08" PRIX32, s, rel_jump(loc, (int16_t) imm));
		strcpy(shrt, canon);
		return true;
	case 0x03:  // BGEZL RS, PC+IMM+4
		siprintf(canon, "bgezl $%" PRIu32 ", 0x%08" PRIX32, s, rel_jump(loc, (int16_t) imm));
#if defined UNLIKELY_SHORT_FORMS
		if (s == 0)  // Effectively an unconditional immediate jump
			siprintf(shrt, "b %" PRIX32, rel_jump(loc, (int16_t) imm));
		else
#endif
			strcpy(shrt, canon);
		return true;
	case 0x08:  // TGEI RS, IMM
		siprintf(canon, "tgei $%" PRIu32 ", 0x%" PRIX16, s, imm);
		siprintf(shrt, "tgei $%" PRIu32 ", %" PRIi16, s, (int16_t) imm);
		return true;
	case 0x09:  // TGEIU RS, IMM
		siprintf(canon, "tgeiu $%" PRIu32 ", 0x%" PRIX16, s, imm);
		siprintf(shrt, "tgeiu $%" PRIu32 ", %" PRIu16, s, imm);
		return true;
	case 0x0A:  // TLTI RS, IMM
		siprintf(canon, "tlti $%" PRIu32 ", 0x%" PRIX16, s, imm);
		siprintf(shrt, "tlti $%" PRIu32 ", %" PRIi16, s, (int16_t) imm);
		return true;
	case 0x0B:  // TLTIU RS, IMM
		siprintf(canon, "tltiu $%" PRIu32 ", 0x%" PRIX16, s, imm);
		siprintf(shrt, "tltiu $%" PRIu32 ", %" PRIu16, s, imm);
		return true;
	case 0x0C:  // TEQI RS, IMM
		siprintf(canon, "teqi $%" PRIu32 ", 0x%" PRIX16, s, imm);
		siprintf(shrt, "teqi $%" PRIu32 ", %" PRIi16, s, (int16_t) imm);
		return true;
	case 0x0E:  // TNEI RS, IMM
		siprintf(canon, "tnei $%" PRIu32 ", 0x%" PRIX16, s, imm);
		siprintf(shrt, "tnei $%" PRIu32 ", %" PRIi16, s, (int16_t) imm);
		return true;
	case 0x10:  // BLTZAL RS, PC+IMM+4
		siprintf(canon, "bltzal $%" PRIu32 ", 0x%08" PRIX32, s, rel_jump(loc, (int16_t) imm));
		strcpy(shrt, canon);
		return true;
	case 0x11:  // BGEZAL RS, PC+IMM+4
		siprintf(canon, "bgezal $%" PRIu32 ", 0x%08" PRIX32, s, rel_jump(loc, (int16_t) imm));
		if (s == 0)  // Effectively an unconditional immediate call
			siprintf(shrt, "bal 0x%08" PRIX32, rel_jump(loc, (int16_t) imm));
		else
			strcpy(shrt, canon);
	case 0x12:  // BLTZALL RS, PC+IMM+4
		siprintf(canon, "bltzall $%" PRIu32 ", 0x%08" PRIX32, s, rel_jump(loc, (int16_t) imm));
		strcpy(shrt, canon);
		return true;
	case 0x13:  // BGEZALL RS, PC+IMM+4
		siprintf(canon, "bgezall $%" PRIu32 ", 0x%08" PRIX32, s, rel_jump(loc, (int16_t) imm));
#if defined UNLIKELY_SHORT_FORMS
		if (s == 0)  // Effectively an unconditional immediate call
			siprintf(shrt, "bal 0x%08" PRIX32, rel_jump(loc, (int16_t) imm));
		else
#endif
			strcpy(shrt, canon);
		return true;
	default:
		return false;
	}
}

/*
 * Disassembles a MIPS32 Coprocessor 0 prefix instruction.
 * Returns true and fills both buffers if the instruction is defined.
 * Returns false if the instruction is undefined.
 */
static bool disassemble_mips32_cop0(uint32_t op, uint32_t loc, char* shrt, char* canon)
{
	if (op & UINT32_C(0x02000000)) { /* Bit 25, CO, is set */
		uint32_t op_min = op & 0x3F;
		switch (op_min) {
		case 0x01:  // TLBR
			strcpy(canon, "tlbr");
			strcpy(shrt, canon);
			return true;
		case 0x02:  // TLBWI
			strcpy(canon, "tlbwi");
			strcpy(shrt, canon);
			return true;
		case 0x06:  // TLBWR
			strcpy(canon, "tlbwr");
			strcpy(shrt, canon);
			return true;
		case 0x08:  // TLBP
			strcpy(canon, "tlbp");
			strcpy(shrt, canon);
			return true;
		case 0x18:  // ERET
			strcpy(canon, "eret");
			strcpy(shrt, canon);
			return true;
		case 0x1F:  // DERET
			strcpy(canon, "deret");
			strcpy(shrt, canon);
			return true;
		case 0x20:  // WAIT
			strcpy(canon, "wait");
			strcpy(shrt, canon);
			return true;
		default:
			return false;
		}
	} else {
		uint32_t cop0_min = (op >> 21) & 0x1F;
		uint32_t t = (op >> 16) & 0x1F,
			d = (op >> 11) & 0x1F,
			sel = op & 0x7;
		switch (cop0_min) {
		case 0x00:  // MFC0 rt, rd, sel
			siprintf(canon, "mfc0 $%" PRIu32 ", $%" PRIu32 ", %" PRIu32, t, d, sel);
			strcpy(shrt, canon);
			return true;
		case 0x04:  // MTC0 rt, rd, sel
			siprintf(canon, "mtc0 $%" PRIu32 ", $%" PRIu32 ", %" PRIu32, t, d, sel);
			strcpy(shrt, canon);
			return true;
		default:
			return false;
		}
	}
}

/*
 * Disassembles a MIPS32 PREF instruction.
 * Returns true and fills both buffers.
 */
static bool disassemble_mips32_pref(uint32_t op, uint32_t loc, char* shrt, char* canon)
{
	uint32_t hint = (op >> 16) & 0x1F,
		base = (op >> 21) & 0x1F;
	uint16_t imm = op;
	siprintf(canon, "pref 0x%" PRIX32 ", 0x%" PRIX16 "($%" PRIu32 ")", hint, imm, base);
	siprintf(shrt, "pref 0x%" PRIX32 ", %" PRIi16 "($%" PRIu32 ")", hint, (int16_t) imm, base);
	return true;
}

/*
 * Disassembles a MIPS32 CACHE instruction.
 * Returns true and fills both buffers.
 */
static bool disassemble_mips32_cache(uint32_t op, uint32_t loc, char* shrt, char* canon)
{
	uint32_t cache_op = (op >> 16) & 0x1F,
		base = (op >> 21) & 0x1F;
	uint16_t imm = op;
	siprintf(canon, "cache 0x%" PRIX32 ", 0x%" PRIX16 "($%" PRIu32 ")", cache_op, imm, base);
	siprintf(shrt, "cache 0x%" PRIX32 ", %" PRIi16 "($%" PRIu32 ")", cache_op, (int16_t) imm, base);
	return true;
}

/*
 * Disassembles a MIPS32 I-type instruction.
 * Returns true and fills both buffers if the instruction is defined.
 * Returns false if the instruction is undefined.
 */
static bool disassemble_mips32_i(uint32_t op, uint32_t loc, char* shrt, char* canon)
{
	uint32_t op_maj = (op >> 26) & 0x3F;
	uint32_t s = (op >> 21) & 0x1F,
		t = (op >> 16) & 0x1F;
	uint16_t imm = op;
	switch (op_maj) {
	case 0x04:  // BEQ RS, RT, PC+IMM+4
		siprintf(canon, "beq $%" PRIu32 ", $%" PRIu32 ", 0x%08" PRIX32, s, t, rel_jump(loc, (int16_t) imm));
		if (s == t)  // Effectively an unconditional immediate jump
			siprintf(shrt, "b 0x%08" PRIX32, rel_jump(loc, (int16_t) imm));
		else
			strcpy(shrt, canon);
		return true;
	case 0x05:  // BNE RS, RT, PC+IMM+4
		siprintf(canon, "bne $%" PRIu32 ", $%" PRIu32 ", 0x%08" PRIX32, s, t, rel_jump(loc, (int16_t) imm));
#if defined UNLIKELY_SHORT_FORMS
		if (s == t)  // Effectively a nop with a branch delay slot
			strcpy(shrt, "nop");
		else
#endif
			strcpy(shrt, canon);
		return true;
	case 0x06:  // BLEZ RS, PC+IMM+4
		siprintf(canon, "blez $%" PRIu32 ", 0x%08" PRIX32, s, rel_jump(loc, (int16_t) imm));
		if (s == 0)  // Effectively an unconditional immediate jump
			siprintf(shrt, "b 0x%08" PRIX32, rel_jump(loc, (int16_t) imm));
		else
			strcpy(shrt, canon);
		return true;
	case 0x07:  // BGTZ RS, PC+IMM+4
		siprintf(canon, "bgtz $%" PRIu32 ", 0x%08" PRIX32, s, rel_jump(loc, (int16_t) imm));
#if defined UNLIKELY_SHORT_FORMS
		if (s == 0)  // Effectively a nop with a branch delay slot
			strcpy(shrt, "nop");
		else
#endif
			strcpy(shrt, canon);
		return true;
	case 0x08:  // ADDI RT, RS, IMM
		siprintf(canon, "addi $%" PRIu32 ", $%" PRIu32 ", 0x%" PRIX16, t, s, imm);
#if defined UNLIKELY_SHORT_FORMS
		if (t == 0)  // Effectively a nop
			strcpy(shrt, "nop");
		else if (s == 0)  // Loading [-32768, 32767] into RT
			siprintf(shrt, "li $%" PRIu32 ", %" PRIi16, t, (int16_t) imm);
		else if (imm == 0)  // Moving RS into RT
			siprintf(shrt, "move $%" PRIu32 ", $%" PRIu32, t, s);
		else
#endif
			siprintf(shrt, "addi $%" PRIu32 ", $%" PRIu32 ", %" PRIi16, t, s, (int16_t) imm);
		return true;
	case 0x09:  // ADDIU RT, RS, IMM
		siprintf(canon, "addiu $%" PRIu32 ", $%" PRIu32 ", 0x%" PRIX16, t, s, imm);
#if defined UNLIKELY_SHORT_FORMS
		if (t == 0)  // Effectively a nop
			strcpy(shrt, "nop");
		else
#endif
		if (s == 0)  // Loading [-32768, 32767] into RT
			siprintf(shrt, "li $%" PRIu32 ", %" PRIi16, t, (int16_t) imm);
#if defined UNLIKELY_SHORT_FORMS
		else if (imm == 0)  // Moving RS into RT
			siprintf(shrt, "move $%" PRIu32 ", $%" PRIu32, t, s);
#endif
		else
			siprintf(shrt, "addiu $%" PRIu32 ", $%" PRIu32 ", %" PRIi16, t, s, (int16_t) imm);
		return true;
	case 0x0A:  // SLTI RT, RS, IMM
		siprintf(canon, "slti $%" PRIu32 ", $%" PRIu32 ", 0x%" PRIX16, t, s, imm);
#if defined UNLIKELY_SHORT_FORMS
		if (t == 0)  // Effectively a nop
			strcpy(shrt, "nop");
		else if (s == 0 && imm == 0)  // Moving zero into RT
			siprintf(shrt, "li $%" PRIu32 ", 0", t);
		else
#endif
			siprintf(shrt, "slti $%" PRIu32 ", $%" PRIu32 ", %" PRIi16, t, s, (int16_t) imm);
		return true;
	case 0x0B:  // SLTIU RT, RS, IMM
		siprintf(canon, "sltiu $%" PRIu32 ", $%" PRIu32 ", 0x%" PRIX16, t, s, imm);
#if defined UNLIKELY_SHORT_FORMS
		if (t == 0)  // Effectively a nop
			strcpy(shrt, "nop");
		else if (s == 0 && imm == 0)  // Moving zero into RT
			siprintf(shrt, "li $%" PRIu32 ", 0", t);
		else
#endif
			siprintf(shrt, "sltiu $%" PRIu32 ", $%" PRIu32 ", %" PRIi16, t, s, (int16_t) imm);
		return true;
	case 0x0C:  // ANDI RT, RS, IMM
		siprintf(canon, "andi $%" PRIu32 ", $%" PRIu32 ", 0x%" PRIX16, t, s, imm);
#if defined UNLIKELY_SHORT_FORMS
		if (t == 0)  // Effectively a nop
			strcpy(shrt, "nop");
		else if (imm == 0)  // Moving zero into RT
			siprintf(shrt, "li $%" PRIu32 ", 0", t);
		else
#endif
			strcpy(shrt, canon);
		return true;
	case 0x0D:  // ORI RT, RS, IMM
		siprintf(canon, "ori $%" PRIu32 ", $%" PRIu32 ", 0x%" PRIX16, t, s, imm);
#if defined UNLIKELY_SHORT_FORMS
		if (t == 0 || (s == t && imm == 0))  // Effectively a nop
			strcpy(shrt, "nop");
		else
#endif
		if (s == 0 && imm > 0)  // Loading [0, 32767] into RT
			siprintf(shrt, "li $%" PRIu32 ", %" PRIi16, t, (int16_t) imm);
#if defined UNLIKELY_SHORT_FORMS
		else if (imm == 0)  // Moving RS into RT
			siprintf(shrt, "move $%" PRIu32 ", $%" PRIu32, t, s);
		else
#endif
			strcpy(shrt, canon);
		return true;
	case 0x0E:  // XORI RT, RS, IMM
		siprintf(canon, "xori $%" PRIu32 ", $%" PRIu32 ", 0x%" PRIX16, t, s, imm);
#if defined UNLIKELY_SHORT_FORMS
		if (t == 0 || (s == t && imm == 0))  // Effectively a nop
			strcpy(shrt, "nop");
		else if (s == 0 && imm >= 0)  // Loading [0, 32767] into RT
			siprintf(shrt, "li $%" PRIu32 ", %" PRIi16, t, (int16_t) imm);
		else if (imm == 0)  // Moving RS into RT
			siprintf(shrt, "move $%" PRIu32 ", $%" PRIu32, t, s);
		else
#endif
			strcpy(shrt, canon);
		return true;
	case 0x0F:  // LUI RT, IMM
		siprintf(canon, "lui $%" PRIu32 ", 0x%" PRIX16, t, imm);
#if defined UNLIKELY_SHORT_FORMS
		if (t == 0)  // Effectively a nop
			strcpy(shrt, "nop");
		else
#endif
			siprintf(shrt, "lui $%" PRIu32 ", %" PRIi16, t, imm);
		return true;
	case 0x14:  // BEQL RS, RT, PC+IMM+4
		siprintf(canon, "beql $%" PRIu32 ", $%" PRIu32 ", 0x%08" PRIX32, s, t, rel_jump(loc, (int16_t) imm));
#if defined UNLIKELY_SHORT_FORMS
		if (s == t)  // Effectively an unconditional immediate jump
			siprintf(shrt, "b 0x%08" PRIX32, rel_jump(loc, (int16_t) imm));
		else
#endif
			strcpy(shrt, canon);
		return true;
	case 0x15:  // BNEL RS, RT, PC+IMM+4
		siprintf(canon, "bnel $%" PRIu32 ", $%" PRIu32 ", 0x%08" PRIX32, s, t, rel_jump(loc, (int16_t) imm));
		strcpy(shrt, canon);
		return true;
	case 0x16:  // BLEZL RS, PC+IMM+4
		siprintf(canon, "blezl $%" PRIu32 ", 0x%08" PRIX32, s, rel_jump(loc, (int16_t) imm));
#if defined UNLIKELY_SHORT_FORMS
		if (s == 0)  // Effectively an unconditional immediate jump
			siprintf(shrt, "b 0x%08" PRIX32, rel_jump(loc, (int16_t) imm));
		else
#endif
			strcpy(shrt, canon);
		return true;
	case 0x17:  // BGTZL RS, PC+IMM+4
		siprintf(canon, "bgtzl $%" PRIu32 ", 0x%08" PRIX32, s, rel_jump(loc, (int16_t) imm));
		strcpy(shrt, canon);
		return true;
	case 0x20:  // LB RT, IMM(RS)
		siprintf(canon, "lb $%" PRIu32 ", 0x%" PRIX16 "($%" PRIu32 ")", t, imm, s);
		siprintf(shrt, "lb $%" PRIu32 ", %" PRIi16 "($%" PRIu32 ")", t, (int16_t) imm, s);
		return true;
	case 0x21:  // LH RT, IMM(RS)
		siprintf(canon, "lh $%" PRIu32 ", 0x%" PRIX16 "($%" PRIu32 ")", t, imm, s);
		siprintf(shrt, "lh $%" PRIu32 ", %" PRIi16 "($%" PRIu32 ")", t, (int16_t) imm, s);
		return true;
	case 0x22:  // LWL RT, IMM(RS)
		siprintf(canon, "lwl $%" PRIu32 ", 0x%" PRIX16 "($%" PRIu32 ")", t, imm, s);
		siprintf(shrt, "lwl $%" PRIu32 ", %" PRIi16 "($%" PRIu32 ")", t, (int16_t) imm, s);
		return true;
	case 0x23:  // LW RT, IMM(RS)
		siprintf(canon, "lw $%" PRIu32 ", 0x%" PRIX16 "($%" PRIu32 ")", t, imm, s);
		siprintf(shrt, "lw $%" PRIu32 ", %" PRIi16 "($%" PRIu32 ")", t, (int16_t) imm, s);
		return true;
	case 0x24:  // LBU RT, IMM(RS)
		siprintf(canon, "lbu $%" PRIu32 ", 0x%" PRIX16 "($%" PRIu32 ")", t, imm, s);
		siprintf(shrt, "lbu $%" PRIu32 ", %" PRIi16 "($%" PRIu32 ")", t, (int16_t) imm, s);
		return true;
	case 0x25:  // LHU RT, IMM(RS)
		siprintf(canon, "lhu $%" PRIu32 ", 0x%" PRIX16 "($%" PRIu32 ")", t, imm, s);
		siprintf(shrt, "lhu $%" PRIu32 ", %" PRIi16 "($%" PRIu32 ")", t, (int16_t) imm, s);
		return true;
	case 0x26:  // LWR RT, IMM(RS)
		siprintf(canon, "lwr $%" PRIu32 ", 0x%" PRIX16 "($%" PRIu32 ")", t, imm, s);
		siprintf(shrt, "lwr $%" PRIu32 ", %" PRIi16 "($%" PRIu32 ")", t, (int16_t) imm, s);
		return true;
	case 0x28:  // SB RT, IMM(RS)
		siprintf(canon, "sb $%" PRIu32 ", 0x%" PRIX16 "($%" PRIu32 ")", t, imm, s);
		siprintf(shrt, "sb $%" PRIu32 ", %" PRIi16 "($%" PRIu32 ")", t, (int16_t) imm, s);
		return true;
	case 0x29:  // SH RT, IMM(RS)
		siprintf(canon, "sh $%" PRIu32 ", 0x%" PRIX16 "($%" PRIu32 ")", t, imm, s);
		siprintf(shrt, "sh $%" PRIu32 ", %" PRIi16 "($%" PRIu32 ")", t, (int16_t) imm, s);
		return true;
	case 0x2A:  // SWL RT, IMM(RS)
		siprintf(canon, "swl $%" PRIu32 ", 0x%" PRIX16 "($%" PRIu32 ")", t, imm, s);
		siprintf(shrt, "swl $%" PRIu32 ", %" PRIi16 "($%" PRIu32 ")", t, (int16_t) imm, s);
		return true;
	case 0x2B:  // SW RT, IMM(RS)
		siprintf(canon, "sw $%" PRIu32 ", 0x%" PRIX16 "($%" PRIu32 ")", t, imm, s);
		siprintf(shrt, "sw $%" PRIu32 ", %" PRIi16 "($%" PRIu32 ")", t, (int16_t) imm, s);
		return true;
	case 0x2E:  // SWR RT, IMM(RS)
		siprintf(canon, "swr $%" PRIu32 ", 0x%" PRIX16 "($%" PRIu32 ")", t, imm, s);
		siprintf(shrt, "swr $%" PRIu32 ", %" PRIi16 "($%" PRIu32 ")", t, (int16_t) imm, s);
		return true;
	case 0x30:  // LL RT, IMM(RS)
		siprintf(canon, "ll $%" PRIu32 ", 0x%" PRIX16 "($%" PRIu32 ")", t, imm, s);
		siprintf(shrt, "ll $%" PRIu32 ", %" PRIi16 "($%" PRIu32 ")", t, (int16_t) imm, s);
		return true;
	case 0x38:  // SC RT, IMM(RS)
		siprintf(canon, "sc $%" PRIu32 ", 0x%" PRIX16 "($%" PRIu32 ")", t, imm, s);
		siprintf(shrt, "sc $%" PRIu32 ", %" PRIi16 "($%" PRIu32 ")", t, (int16_t) imm, s);
		return true;
	default:
		return false;
	}
}

/*
 * Disassembles a MIPS32 SPECIAL2 prefix instruction.
 * Returns true and fills both buffers if the instruction is defined.
 * Returns false if the instruction is undefined.
 */
static bool disassemble_mips32_special2(uint32_t op, uint32_t loc, char* shrt, char* canon)
{
	uint32_t s = (op >> 21) & 0x1F,
		t = (op >> 16) & 0x1F,
		d = (op >> 11) & 0x1F,
		op_min = op & 0x3F;
	switch (op_min) {
	case 0x00:  // MADD RS, RT
		siprintf(canon, "madd $%" PRIu32 ", $%" PRIu32, s, t);
#if defined UNLIKELY_SHORT_FORMS
		if (s == 0 || t == 0)  // Effectively a nop
			strcpy(shrt, "nop");
		else
#endif
			strcpy(shrt, canon);
		return true;
	case 0x01:  // MADDU RS, RT
		siprintf(canon, "maddu $%" PRIu32 ", $%" PRIu32, s, t);
#if defined UNLIKELY_SHORT_FORMS
		if (s == 0 || t == 0)  // Effectively a nop
			strcpy(shrt, "nop");
		else
#endif
			strcpy(shrt, canon);
		return true;
	case 0x02:  // MUL RD, RS, RT
		siprintf(canon, "mul $%" PRIu32 ", $%" PRIu32 ", $%" PRIu32, d, s, t);
#if defined UNLIKELY_SHORT_FORMS
		if (d == 0)  // Effectively a nop
			strcpy(shrt, "nop");
		else if (s == 0 || t == 0)  // Moving zero into RD
			siprintf(shrt, "li $%" PRIu32 ", 0", d);
		else
#endif
			strcpy(shrt, canon);
		return true;
	case 0x04:  // MSUB RS, RT
		siprintf(canon, "msub $%" PRIu32 ", $%" PRIu32, s, t);
#if defined UNLIKELY_SHORT_FORMS
		if (s == 0 || t == 0)  // Effectively a nop
			strcpy(shrt, "nop");
		else
#endif
			strcpy(shrt, canon);
		return true;
	case 0x05:  // MSUBU RS, RT
		siprintf(canon, "msubu $%" PRIu32 ", $%" PRIu32, s, t);
#if defined UNLIKELY_SHORT_FORMS
		if (s == 0 || t == 0)  // Effectively a nop
			strcpy(shrt, "nop");
		else
#endif
			strcpy(shrt, canon);
		return true;
	case 0x20:  // CLZ RD, RS
		siprintf(canon, "clz $%" PRIu32 ", $%" PRIu32, d, s);
#if defined UNLIKELY_SHORT_FORMS
		if (d == 0)  // Effectively a nop
			strcpy(shrt, "nop");
		else
#endif
			strcpy(shrt, canon);
		return true;
	case 0x21:  // CLO RD, RS
		siprintf(canon, "clo $%" PRIu32 ", $%" PRIu32, d, s);
#if defined UNLIKELY_SHORT_FORMS
		if (d == 0)  // Effectively a nop
			strcpy(shrt, "nop");
		else
#endif
			strcpy(shrt, canon);
		return true;
	case 0x3F:  // SDBBP imm20
		siprintf(canon, "sdbbp 0x%" PRIX32, (op >> 6) & UINT32_C(0xFFFFF));
		strcpy(shrt, canon);
		return true;
	default:
		return false;
	}
}

/*
 * Disassembles a MIPS32 J-type instruction.
 * Returns true and fills both buffers if the instruction is defined.
 * Returns false if the instruction is undefined.
 */
static bool disassemble_mips32_j(uint32_t op, uint32_t loc, char* shrt, char* canon)
{
	uint32_t op_maj = (op >> 26) & 0x3F;
	uint32_t imm = (op >> 6) & UINT32_C(0x03FFFFFF);
	switch (op_maj) {
	case 0x02:  // J PCSeg4.IMM26.00
		siprintf(canon, "j 0x%08" PRIX32, abs_jump(loc, imm));
		strcpy(shrt, canon);
		return true;
	case 0x03:  // JAL PCSeg4.IMM26.00
		siprintf(canon, "jal 0x%08" PRIX32, abs_jump(loc, imm));
		strcpy(shrt, canon);
		return true;
	default:
		return false;
	}
}

void process_exception()
{
	struct card_reply_mips_exception exception;
	size_t i;

	REG_IME = IME_ENABLE;
	card_read_data(sizeof(exception), &exception, true);
	REG_IME = IME_DISABLE;

	set_sub_text();
	consoleClear();
	iprintf("    - Supercard exception -\n%02" PRIX32 " %s\nat address %08" PRIX32 "\n",
	       exception.excode, excode_desc(exception.excode), exception.c0_epc);

	if (exception.mapped) {
		char shrt[64], canon[64];

		disassemble_mips32(exception.op, exception.c0_epc, shrt, canon);
		iprintf("->   %s\n", shrt);
		if (strcmp(shrt, canon) != 0) {
			iprintf("   = %s\n", canon);
		} else {
			iprintf("\n");
		}

		disassemble_mips32(exception.next_op, exception.c0_epc + 4, shrt, canon);
		iprintf("     %s\n", shrt);
		if (strcmp(shrt, canon) != 0) {
			iprintf("   = %s\n", canon);
		} else {
			iprintf("\n");
		}
		iprintf("\n");
	} else if ((exception.c0_epc & 3) != 0) {
		iprintf("\n     (%08" PRIX32 " is unaligned)\n\n\n", exception.c0_epc);
	} else {
		iprintf("\n     (%08" PRIX32 " is unmapped)\n\n\n", exception.c0_epc);
	}

	for (i = 0; i < REG_COUNT; i++) {
		iprintf("  %2" PRIu8 " = %08" PRIX32, reg_nums[i],
		       *(uint32_t*) ((uint8_t*) &exception + reg_offs[i]));
		if (i & 1) {
			iprintf("\n");
		}
	}

	if (REG_COUNT & 1) {
		iprintf("\n");
	}

	iprintf("  HI = %08" PRIX32 "  LO = %08" PRIX32, exception.hi, exception.lo);

	link_status = LINK_STATUS_ERROR;
}
