/*
 * This file is part of the MIPS executable filter for the Supercard DSTwo.
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

#ifndef FILTER_OPS_H
#define FILTER_OPS_H

/* This is shared between the filterer and unfilterer, because they must agree
 * on the order of opcodes in order to work correctly. Opcode compositions are
 * described after each enum value. Refer to filter_streams.h for stream names
 * and information.
 */

enum flat_op {
	OP_NONE = -1,  /* Used in tables for invalid opcodes/need dispatch */

	/* Very frequent in MIPS executables */
	OP_ADDIU,      /* rt -> REGS, rs -> REGS, imm16 -> IMM16S */
	OP_LW,         /* rs -> REGS, rt -> REGS, imm16 -> MEMOFFS */
	OP_SW,         /* rs -> REGS, rt -> REGS, imm16 -> MEMOFFS */
	OP_ADDU,       /* rd -> REGS, rs -> REGS, rt -> REGS */
	OP_BEQ,        /* rs -> REGS, rt -> REGS, imm16 -> J16RS */
	OP_ANDI,       /* rt -> REGS, rs -> REGS, imm16 -> IMM16S */
	OP_OR,         /* rd -> REGS, rs -> REGS, rt -> REGS */
	OP_SLL,        /* rd -> REGS, rt -> REGS, sa -> SAS */
	OP_SRL,        /* rd -> REGS, rt -> REGS, sa -> SAS */
	OP_LUI,        /* rt -> REGS, imm16 -> IMM16S */
	OP_BNE,        /* rs -> REGS, rt -> REGS, imm16 -> J16RS */
	OP_NOP,        /* Nothing */
	OP_JAL,        /* imm26 -> J26AS */
	OP_LHU,        /* rs -> REGS, rt -> REGS, imm16 -> MEMOFFS */
	OP_SH,         /* rs -> REGS, rt -> REGS, imm16 -> MEMOFFS */
	OP_ORI,        /* rt -> REGS, rs -> REGS, imm16 -> IMM16S */
	OP_SUBU,       /* rd -> REGS, rs -> REGS, rt -> REGS */
	OP_JR,         /* rs -> REGS */
	OP_AND,        /* rd -> REGS, rs -> REGS, rt -> REGS */
	OP_LBU,        /* rs -> REGS, rt -> REGS, imm16 -> MEMOFFS */
	OP_SB,         /* rs -> REGS, rt -> REGS, imm16 -> MEMOFFS */
	OP_SLTU,       /* rd -> REGS, rs -> REGS, rt -> REGS */
	OP_SLTIU,      /* rt -> REGS, rs -> REGS, imm16 -> IMM16S */
	OP_SLT,        /* rd -> REGS, rs -> REGS, rt -> REGS */
	OP_SLTI,       /* rt -> REGS, rs -> REGS, imm16 -> IMM16S */

	OP_BGEZ,       /* rs -> REGS, imm16 -> J16RS */
	OP_BGEZAL,     /* rs -> REGS, imm16 -> J16RS */
	OP_BGTZ,       /* rs -> REGS, imm16 -> J16RS */
	OP_BLEZ,       /* rs -> REGS, imm16 -> J16RS */
	OP_BLTZ,       /* rs -> REGS, imm16 -> J16RS */
	OP_DIV,        /* rs -> REGS, rt -> REGS */
	OP_DIVU,       /* rs -> REGS, rt -> REGS */
	OP_J,          /* imm26 -> J26AS */
	OP_JALR,       /* rd -> REGS, rs -> REGS */
	OP_LB,         /* rs -> REGS, rt -> REGS, imm16 -> MEMOFFS */
	OP_LH,         /* rs -> REGS, rt -> REGS, imm16 -> MEMOFFS */
	OP_MFHI,       /* rd -> REGS */
	OP_MFLO,       /* rd -> REGS */
	OP_MOVN,       /* rd -> REGS, rs -> REGS, rt -> REGS */
	OP_MOVZ,       /* rd -> REGS, rs -> REGS, rt -> REGS */
	OP_MUL,        /* rd -> REGS, rs -> REGS, rt -> REGS */
	OP_MULT,       /* rs -> REGS, rt -> REGS */
	OP_MULTU,      /* rs -> REGS, rt -> REGS */
	OP_NOR,        /* rd -> REGS, rs -> REGS, rt -> REGS */
	OP_PREF,       /* rs -> REGS, #20..16 (hint) -> REGS, imm16 -> MEMOFFS */
	OP_SLLV,       /* rd -> REGS, rs -> REGS, rt -> REGS */
	OP_SRA,        /* rd -> REGS, rt -> REGS, sa -> SAS */
	OP_SRAV,       /* rd -> REGS, rs -> REGS, rt -> REGS */
	OP_SRLV,       /* rd -> REGS, rs -> REGS, rt -> REGS */
	OP_XOR,        /* rd -> REGS, rs -> REGS, rt -> REGS */
	OP_XORI,       /* rt -> REGS, rs -> REGS, imm16 -> IMM16S */

	/* Very infrequent in MIPS executables */
	OP_ADD,        /* rd -> REGS, rs -> REGS, rt -> REGS */
	OP_ADDI,       /* rt -> REGS, rs -> REGS, imm16 -> IMM16S */
	OP_BLTZAL,     /* rs -> REGS, imm16 -> J16RS */
	OP_BREAK,      /* #25..6 (brkcode) -> IMM20S */
	OP_CACHE,      /* rs -> REGS, #20..16 (op) -> REGS, imm16 -> MEMOFFS */
	OP_CLO,        /* rd -> REGS, rs -> REGS, rt -> REGS */
	OP_CLZ,        /* rd -> REGS, rs -> REGS, rt -> REGS */
	OP_COP0,       /* #25..0 -> REST */
	OP_LL,         /* rs -> REGS, rt -> REGS, imm16 -> MEMOFFS */
	OP_LWL,        /* rs -> REGS, rt -> REGS, imm16 -> MEMOFFS */
	OP_LWR,        /* rs -> REGS, rt -> REGS, imm16 -> MEMOFFS */
	OP_MADD,       /* rs -> REGS, rt -> REGS */
	OP_MADDU,      /* rs -> REGS, rt -> REGS */
	OP_MSUB,       /* rs -> REGS, rt -> REGS */
	OP_MSUBU,      /* rs -> REGS, rt -> REGS */
	OP_MTHI,       /* rs -> REGS */
	OP_MTLO,       /* rs -> REGS */
	OP_SC,         /* rs -> REGS, rt -> REGS, imm16 -> MEMOFFS */
	OP_SDBBP,      /* #25..6 (brkcode) -> IMM20S */
	OP_SUB,        /* rd -> REGS, rs -> REGS, rt -> REGS */
	OP_SWL,        /* rs -> REGS, rt -> REGS, imm16 -> MEMOFFS */
	OP_SWR,        /* rs -> REGS, rt -> REGS, imm16 -> MEMOFFS */
	OP_SYNC,       /* #10..6 (stype) -> SAS */
	OP_SYSCALL,    /* #25..6 (code) -> IMM20S */
	OP_T,          /* rs -> REGS, rt -> REGS, #15..0 (code, op) -> IMM16S */
	OP_TI,         /* rs -> REGS, #20..16 (op) -> REGS, imm16 -> IMM16S */

	/* Deprecated in MIPS32 */
	OP_BEQL,       /* rs -> REGS, rt -> REGS, imm16 -> J16RS */
	OP_BGEZALL,    /* rs -> REGS, imm16 -> J16RS */
	OP_BGEZL,      /* rs -> REGS, imm16 -> J16RS */
	OP_BGTZL,      /* rs -> REGS, imm16 -> J16RS */
	OP_BLEZL,      /* rs -> REGS, imm16 -> J16RS */
	OP_BLTZALL,    /* rs -> REGS, imm16 -> J16RS */
	OP_BLTZL,      /* rs -> REGS, imm16 -> J16RS */
	OP_BNEL,       /* rs -> REGS, rt -> REGS, imm16 -> J16RS */

	OP_COUNT,      /* Determines the number of bytes needed to encode an op */
};

#endif
