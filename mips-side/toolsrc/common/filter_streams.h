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

#ifndef FILTER_STREAMS_H
#define FILTER_STREAMS_H

/* This is shared between the filterer and unfilterer, because they must agree
 * on the number and composition of streams in order to work correctly. Stream
 * compositions are described before each enum value.
 *
 * For speed and reduced code size in the unfilterer, streams are aligned to 4
 * bytes.
 *
 * After STRM_REST's ending alignment, the data segment follows, stored as is.
 */

enum stream_id {
	/* The first stream contains the instructions, whose number is transmitted
	 * before the streams due to the requirement for alignment. They're stored
	 * as flat opcode numbers, which are found in filter_ops.h. Each opcode is
	 * responsible for reading or writing its own things in the other streams,
	 * depending on the opcode.
	 *
	 * Helps Huffman and arithmetic coders due to the reduced symbol count.
	 *
	 * Helps dictionary compressors exploit repetition in opcodes: procedures'
	 * entry and exit code, branch tests and delay slots and other common code
	 * sequences.
	 *
	 * If the most common opcodes are assigned numbers between 0 and 31, there
	 * may be incidental matches by dictionary compressors in the OPCODES when
	 * encoding REGS and SAS, even though the streams' semantics differ.
	 */
	STRM_OPCODES,
	/* STRM_REGS contains the registers used as operands by instructions.
	 *
	 * Helps Huffman and arithmetic coders due to the reduced symbol count (32
	 * symbols) and frequent use of temporary registers in procedures.
	 *
	 * Helps dictionary compressors exploit repetition in operands. Procedures
	 * with stack accesses ([29 x 29 y 29 z]) and expressions that continually
	 * modify the same register, e.g. as an accumulator ([a a b a a c a a d]),
	 * can be coded as references.
	 */
	STRM_REGS,
	/* STRM_SAS contains the shift amounts used in SLL, SRL and SRA
	 * instructions.
	 *
	 * Helps Huffman and arithmetic coders due to the reduced symbol count (32
	 * symbols) and clustering of shift amounts to common values (2, 3, 8, 16,
	 * 24). The symbols are also not changed from STRM_REGS.
	 *
	 * Helps RLE compressors exploit repetition in shift amounts. SLL and {SRL
	 * | SRA} are often paired with the same shift amount.
	 */
	STRM_SAS,
	/* STRM_IMM16S contains the 16-bit immediates not caught by the two next
	 * streams. */
	STRM_IMM16S,
	/* STRM_J16RS contains the 16-bit relative branch offsets used in branch
	 * (B*) instructions, conditional or not.
	 *
	 * Helps Huffman and arithmetic coders because the bytes <00> and <FF> are
	 * often seen in relative branch offsets in small enough procedures.
	 */
	STRM_J16RS,
	/* STRM_MEMOFFS contains the 16-bit offsets from the base register in L*
	 * and S* instructions.
	 *
	 * Helps Huffman and arithmetic coders because the bytes <00> and <FF> are
	 * often seen in the usually small memory offsets used to access the stack
	 * and the high frequency of 0(reg) accesses.
	 *
	 * Helps dictionary compressors exploit the repeated use of relocated data
	 * addresses (low bits), on top of the often repeated stack slot offsets.
	 */
	STRM_MEMOFFS,
	/* STRM_IMM20S contains the 20-bit codes used in SYSCALL, BREAK and SDBBP
	 * instructions.
	 *
	 * Helps dictionary compressors exploit the repeated use of breakpoint and
	 * system call codes.
	 */
	STRM_IMM20S,
	/* STRM_J26AS contains the 26-bit absolute jump offsets used in J and JAL
	 * instructions.
	 *
	 * Helps Huffman and arithmetic coders because, since all code must fit in
	 * 32 MiB on the Supercard, the byte <00> is in every J or JAL instruction
	 * as the high byte.
	 *
	 * Helps dictionary compressors exploit the repetition involved in calling
	 * the (relatively) few procedures in a program many times.
	 */
	STRM_J26AS,
	/* STRM_REST currently contains the lower 26 bits of Coprocessor 0
	 * instructions. */
	STRM_REST,

	STRM_COUNT,  /* Can be used to make arrays of streams */
};

#endif /* !FILTER_STREAMS_H */
