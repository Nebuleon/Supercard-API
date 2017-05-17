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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../common/filter_streams.h"
#include "../common/filter_ops.h"

#define ROUND_UP_CONST_PTR(p, a) ((const void*) ( ((uintptr_t) (p) + (a) - 1) & ~((uintptr_t) (a) - 1) ))

/* Set by start.S, which is in turn written by the filterer in the first four
 * instructions of the unfilterer */
uint32_t filtered_size __attribute__((section(".noinit")));
uint32_t instruction_count __attribute__((section(".noinit")));

/* Symbol provided by the linker, at the end of all data sections. Despite it
 * being declared as a byte array, it's 4-byte-aligned. */
extern uint8_t filtered_data[];

/* The type of functions that decode data from the streams as required by an
 * instruction.
 *
 * In:
 *   argument 1: Array of stream pointers.
 * Out:
 *   Zero or more of the 'streams' have had data read from them.
 * Returns:
 *   Bits to be ORed into the opcode template for the instruction.
 */
typedef uint32_t (*decode_fn) (const void**);

/* The functions that decode data from streams. */
static uint32_t decode_null(const void**);
static uint32_t decode_rs(const void**);
static uint32_t decode_rd(const void**);
static uint32_t decode_sa(const void**);
static uint32_t decode_j26a(const void**);
static uint32_t decode_imm20(const void**);
static uint32_t decode_rest_0_25(const void**);
static uint32_t decode_rd_rt_sa(const void**);
static uint32_t decode_rd_rs_rt(const void**);
static uint32_t decode_rs_rt(const void**);
static uint32_t decode_rd_rs(const void**);
static uint32_t decode_rs_j16r(const void**);
static uint32_t decode_rs_rt_mem(const void**);
static uint32_t decode_rs_rt_j16r(const void**);
static uint32_t decode_rs_rt_imm16(const void**);
static uint32_t decode_rt_rs_imm16(const void**);
static uint32_t decode_rt_imm16(const void**);

struct op_desc {
	decode_fn decode;
	uint32_t opcode;
};

/* These tables map flat opcodes to the functions that read the things they
 * need from the appropriate streams. */
struct op_desc ops[OP_COUNT] = {
	[OP_ADD]     = { decode_rd_rs_rt,    UINT32_C(0x00000020) },
	[OP_ADDI]    = { decode_rt_rs_imm16, UINT32_C(0x20000000) },
	[OP_ADDIU]   = { decode_rt_rs_imm16, UINT32_C(0x24000000) },
	[OP_ADDU]    = { decode_rd_rs_rt,    UINT32_C(0x00000021) },
	[OP_AND]     = { decode_rd_rs_rt,    UINT32_C(0x00000024) },
	[OP_ANDI]    = { decode_rt_rs_imm16, UINT32_C(0x30000000) },
	[OP_BEQ]     = { decode_rs_rt_j16r,  UINT32_C(0x10000000) },
	[OP_BEQL]    = { decode_rs_rt_j16r,  UINT32_C(0x50000000) },
	[OP_BGEZ]    = { decode_rs_j16r,     UINT32_C(0x04010000) },
	[OP_BGEZAL]  = { decode_rs_j16r,     UINT32_C(0x04110000) },
	[OP_BGEZALL] = { decode_rs_j16r,     UINT32_C(0x04130000) },
	[OP_BGEZL]   = { decode_rs_j16r,     UINT32_C(0x04030000) },
	[OP_BGTZ]    = { decode_rs_j16r,     UINT32_C(0x1C000000) },
	[OP_BGTZL]   = { decode_rs_j16r,     UINT32_C(0x5C000000) },
	[OP_BLEZ]    = { decode_rs_j16r,     UINT32_C(0x18000000) },
	[OP_BLEZL]   = { decode_rs_j16r,     UINT32_C(0x58000000) },
	[OP_BLTZ]    = { decode_rs_j16r,     UINT32_C(0x04000000) },
	[OP_BLTZAL]  = { decode_rs_j16r,     UINT32_C(0x04100000) },
	[OP_BLTZALL] = { decode_rs_j16r,     UINT32_C(0x04120000) },
	[OP_BLTZL]   = { decode_rs_j16r,     UINT32_C(0x04020000) },
	[OP_BNE]     = { decode_rs_rt_j16r,  UINT32_C(0x14000000) },
	[OP_BNEL]    = { decode_rs_rt_j16r,  UINT32_C(0x54000000) },
	[OP_BREAK]   = { decode_imm20,       UINT32_C(0x0000000D) },
	[OP_CACHE]   = { decode_rs_rt_mem,   UINT32_C(0xBC000000) },
	[OP_CLO]     = { decode_rd_rs_rt,    UINT32_C(0x70000021) },
	[OP_CLZ]     = { decode_rd_rs_rt,    UINT32_C(0x70000020) },
	[OP_COP0]    = { decode_rest_0_25,   UINT32_C(0x40000000) },
	[OP_DIV]     = { decode_rs_rt,       UINT32_C(0x0000001A) },
	[OP_DIVU]    = { decode_rs_rt,       UINT32_C(0x0000001B) },
	[OP_J]       = { decode_j26a,        UINT32_C(0x08000000) },
	[OP_JAL]     = { decode_j26a,        UINT32_C(0x0C000000) },
	[OP_JALR]    = { decode_rd_rs,       UINT32_C(0x00000009) },
	[OP_JR]      = { decode_rs,          UINT32_C(0x00000008) },
	[OP_LB]      = { decode_rs_rt_mem,   UINT32_C(0x80000000) },
	[OP_LBU]     = { decode_rs_rt_mem,   UINT32_C(0x90000000) },
	[OP_LH]      = { decode_rs_rt_mem,   UINT32_C(0x84000000) },
	[OP_LHU]     = { decode_rs_rt_mem,   UINT32_C(0x94000000) },
	[OP_LL]      = { decode_rs_rt_mem,   UINT32_C(0xC0000000) },
	[OP_LUI]     = { decode_rt_imm16,    UINT32_C(0x3C000000) },
	[OP_LW]      = { decode_rs_rt_mem,   UINT32_C(0x8C000000) },
	[OP_LWL]     = { decode_rs_rt_mem,   UINT32_C(0x88000000) },
	[OP_LWR]     = { decode_rs_rt_mem,   UINT32_C(0x98000000) },
	[OP_MADD]    = { decode_rs_rt,       UINT32_C(0x70000000) },
	[OP_MADDU]   = { decode_rs_rt,       UINT32_C(0x70000001) },
	[OP_MFHI]    = { decode_rd,          UINT32_C(0x00000010) },
	[OP_MFLO]    = { decode_rd,          UINT32_C(0x00000012) },
	[OP_MOVN]    = { decode_rd_rs_rt,    UINT32_C(0x0000000B) },
	[OP_MOVZ]    = { decode_rd_rs_rt,    UINT32_C(0x0000000A) },
	[OP_MSUB]    = { decode_rs_rt,       UINT32_C(0x70000004) },
	[OP_MSUBU]   = { decode_rs_rt,       UINT32_C(0x70000005) },
	[OP_MTHI]    = { decode_rs,          UINT32_C(0x00000011) },
	[OP_MTLO]    = { decode_rs,          UINT32_C(0x00000013) },
	[OP_MUL]     = { decode_rd_rs_rt,    UINT32_C(0x70000002) },
	[OP_MULT]    = { decode_rs_rt,       UINT32_C(0x00000018) },
	[OP_MULTU]   = { decode_rs_rt,       UINT32_C(0x00000019) },
	[OP_NOP]     = { decode_null,        UINT32_C(0x00000000) },
	[OP_NOR]     = { decode_rd_rs_rt,    UINT32_C(0x00000027) },
	[OP_OR]      = { decode_rd_rs_rt,    UINT32_C(0x00000025) },
	[OP_ORI]     = { decode_rt_rs_imm16, UINT32_C(0x34000000) },
	[OP_PREF]    = { decode_rs_rt_mem,   UINT32_C(0xCC000000) },
	[OP_SB]      = { decode_rs_rt_mem,   UINT32_C(0xA0000000) },
	[OP_SC]      = { decode_rs_rt_mem,   UINT32_C(0xE0000000) },
	[OP_SDBBP]   = { decode_imm20,       UINT32_C(0x7000003F) },
	[OP_SH]      = { decode_rs_rt_mem,   UINT32_C(0xA4000000) },
	[OP_SLL]     = { decode_rd_rt_sa,    UINT32_C(0x00000000) },
	[OP_SLLV]    = { decode_rd_rs_rt,    UINT32_C(0x00000004) },
	[OP_SLT]     = { decode_rd_rs_rt,    UINT32_C(0x0000002A) },
	[OP_SLTI]    = { decode_rt_rs_imm16, UINT32_C(0x28000000) },
	[OP_SLTIU]   = { decode_rt_rs_imm16, UINT32_C(0x2C000000) },
	[OP_SLTU]    = { decode_rd_rs_rt,    UINT32_C(0x0000002B) },
	[OP_SRA]     = { decode_rd_rt_sa,    UINT32_C(0x00000003) },
	[OP_SRAV]    = { decode_rd_rs_rt,    UINT32_C(0x00000007) },
	[OP_SRL]     = { decode_rd_rt_sa,    UINT32_C(0x00000002) },
	[OP_SRLV]    = { decode_rd_rs_rt,    UINT32_C(0x00000006) },
	[OP_SUB]     = { decode_rd_rs_rt,    UINT32_C(0x00000022) },
	[OP_SUBU]    = { decode_rd_rs_rt,    UINT32_C(0x00000023) },
	[OP_SYNC]    = { decode_sa,          UINT32_C(0x0000000F) },
	[OP_SYSCALL] = { decode_imm20,       UINT32_C(0x0000000C) },
	[OP_SW]      = { decode_rs_rt_mem,   UINT32_C(0xAC000000) },
	[OP_SWL]     = { decode_rs_rt_mem,   UINT32_C(0xA8000000) },
	[OP_SWR]     = { decode_rs_rt_mem,   UINT32_C(0xB8000000) },
	[OP_T]       = { decode_rs_rt_imm16, UINT32_C(0x00000000) },
	[OP_TI]      = { decode_rs_rt_imm16, UINT32_C(0x04000000) },
	[OP_XOR]     = { decode_rd_rs_rt,    UINT32_C(0x00000026) },
	[OP_XORI]    = { decode_rt_rs_imm16, UINT32_C(0x38000000) },
};

static uint32_t decode_null(const void** streams)
{
	return 0;
}

static uint32_t decode_rs(const void** streams)
{
	const uint8_t* regs = streams[STRM_REGS];
	uint32_t ret = (uint32_t) regs[0] << 21;
	streams[STRM_REGS] = regs + 1;
	return ret;
}

static uint32_t decode_rd(const void** streams)
{
	const uint8_t* regs = streams[STRM_REGS];
	uint32_t ret = (uint32_t) regs[0] << 11;
	streams[STRM_REGS] = regs + 1;
	return ret;
}

static uint32_t decode_sa(const void** streams)
{
	const uint8_t* sas = streams[STRM_SAS];
	uint32_t ret = (uint32_t) sas[0] << 6;
	streams[STRM_SAS] = sas + 1;
	return ret;
}

static uint32_t decode_j26a(const void** streams)
{
	const uint32_t* j26as = streams[STRM_J26AS];
	uint32_t ret = j26as[0];
	streams[STRM_J26AS] = j26as + 1;
	return ret;
}

static uint32_t decode_imm20(const void** streams)
{
	const uint32_t* imm20s = streams[STRM_IMM20S];
	uint32_t ret = imm20s[0];
	streams[STRM_IMM20S] = imm20s + 1;
	return ret;
}

static uint32_t decode_rest_0_25(const void** streams)
{
	const uint32_t* rest = streams[STRM_REST];
	uint32_t ret = rest[0];
	streams[STRM_REST] = rest + 1;
	return ret;
}

static uint32_t decode_rd_rt_sa(const void** streams)
{
	const uint8_t* regs = streams[STRM_REGS];
	const uint8_t* sas = streams[STRM_SAS];
	uint32_t ret = ((uint32_t) regs[0] << 11)
	             | ((uint32_t) regs[1] << 16)
	             | ((uint32_t) sas[0] << 6);
	streams[STRM_REGS] = regs + 2;
	streams[STRM_SAS] = sas + 1;
	return ret;
}

static uint32_t decode_rd_rs_rt(const void** streams)
{
	const uint8_t* regs = streams[STRM_REGS];
	uint32_t ret = ((uint32_t) regs[0] << 11)
	             | ((uint32_t) regs[1] << 21)
	             | ((uint32_t) regs[2] << 16);
	streams[STRM_REGS] = regs + 3;
	return ret;
}

static uint32_t decode_rs_rt(const void** streams)
{
	const uint8_t* regs = streams[STRM_REGS];
	uint32_t ret = ((uint32_t) regs[0] << 21)
	             | ((uint32_t) regs[1] << 16);
	streams[STRM_REGS] = regs + 2;
	return ret;
}

static uint32_t decode_rd_rs(const void** streams)
{
	const uint8_t* regs = streams[STRM_REGS];
	uint32_t ret = ((uint32_t) regs[0] << 11)
	             | ((uint32_t) regs[1] << 21);
	streams[STRM_REGS] = regs + 2;
	return ret;
}

static uint32_t decode_rs_j16r(const void** streams)
{
	const uint8_t* regs = streams[STRM_REGS];
	const uint16_t* j16rs = streams[STRM_J16RS];
	uint32_t ret = ((uint32_t) regs[0] << 21) | j16rs[0];
	streams[STRM_REGS] = regs + 1;
	streams[STRM_J16RS] = j16rs + 1;
	return ret;
}

static uint32_t decode_rs_rt_mem(const void** streams)
{
	const uint8_t* regs = streams[STRM_REGS];
	const uint16_t* memoffs = streams[STRM_MEMOFFS];
	uint32_t ret = ((uint32_t) regs[0] << 21)
	             | ((uint32_t) regs[1] << 16)
	             | memoffs[0];
	streams[STRM_REGS] = regs + 2;
	streams[STRM_MEMOFFS] = memoffs + 1;
	return ret;
}

static uint32_t decode_rs_rt_j16r(const void** streams)
{
	const uint8_t* regs = streams[STRM_REGS];
	const uint16_t* j16rs = streams[STRM_J16RS];
	uint32_t ret = ((uint32_t) regs[0] << 21)
	             | ((uint32_t) regs[1] << 16)
	             | j16rs[0];
	streams[STRM_REGS] = regs + 2;
	streams[STRM_J16RS] = j16rs + 1;
	return ret;
}

static uint32_t decode_rs_rt_imm16(const void** streams)
{
	const uint8_t* regs = streams[STRM_REGS];
	const uint16_t* imm16s = streams[STRM_IMM16S];
	uint32_t ret = ((uint32_t) regs[0] << 21)
	             | ((uint32_t) regs[1] << 16)
	             | imm16s[0];
	streams[STRM_REGS] = regs + 2;
	streams[STRM_IMM16S] = imm16s + 1;
	return ret;
}

static uint32_t decode_rt_rs_imm16(const void** streams)
{
	const uint8_t* regs = streams[STRM_REGS];
	const uint16_t* imm16s = streams[STRM_IMM16S];
	uint32_t ret = ((uint32_t) regs[0] << 16)
	             | ((uint32_t) regs[1] << 21)
	             | imm16s[0];
	streams[STRM_REGS] = regs + 2;
	streams[STRM_IMM16S] = imm16s + 1;
	return ret;
}

static uint32_t decode_rt_imm16(const void** streams)
{
	const uint8_t* regs = streams[STRM_REGS];
	const uint16_t* imm16s = streams[STRM_IMM16S];
	uint32_t ret = ((uint32_t) regs[0] << 16) | imm16s[0];
	streams[STRM_REGS] = regs + 1;
	streams[STRM_IMM16S] = imm16s + 1;
	return ret;
}

/* Returns the address just past the last written word, or 0 on failure. */
void* entry(void)
{
	/* Pointer to the next instruction or data word to be written. */
	uint32_t* output = (uint32_t*) 0x80002000;
	const void* input = filtered_data;
	/* Pointers to the next items to be read from each of the streams coming
	 * from the filter. */
	const void* streams[STRM_COUNT];
	/* A copy of the opcode stream, which is used only in this function. */
	const uint8_t* op_bytes;
	size_t i;

	for (i = 0; i < STRM_COUNT; i++) {
		const uint32_t* input_words = (const uint32_t*) input;
		streams[i] = (const uint8_t*) input + STRM_COUNT * 4 /* past the header */
			+ input_words[i] /* stream offset, in bytes, stored in a word */;
	}

	op_bytes = streams[STRM_OPCODES];

	for (i = instruction_count; i > 0; i--) {
		unsigned int op = 0, end = OP_COUNT - 1;
		size_t byte = 0;

		/* If more than one byte was needed to encode the flat_op, decode
		 * more bytes; the result is little-endian. */
		while (end != 0) {
			op |= *op_bytes++ << (byte++ * 8);
			end >>= 8;
		}

		*output++ = ops[op].opcode | ops[op].decode(streams);
	}

	/* After instructions are done, the last stream is advanced to the start
	 * of the data segment. Make sure that's aligned in the input. */
	const uint8_t* input_bytes = ROUND_UP_CONST_PTR(streams[STRM_COUNT - 1], 4);
	memcpy(output, input_bytes, (filtered_data + filtered_size) - input_bytes);

	return (uint8_t*) output + ((filtered_data + filtered_size) - input_bytes);
}
