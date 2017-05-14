/*
 * This file is part of the MIPS packer for the Supercard DSTwo.
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

/* filter.c converts MIPS instructions into multiple "streams" to be sent to
 * the compressor. Each "stream" contains one, and only one, sort of data to
 * be stuffed inside instructions. The first contains the opcodes, which are
 * needed by the unpacker to know which of the next ones to read from; other
 * streams have different uses. See filter_streams.h for rationales for each
 * stream. (In general, the streams are expected to increase redundancy, and
 * compressors exploit redundancy very well.)
 */

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../common/filter_streams.h"
#include "../common/filter_ops.h"

#define ROUND_UP_4(n) (((n) + 3) & ~3)

struct stream {
	uint8_t* data;
	size_t size;
	size_t capacity;
};

struct stream streams[STRM_COUNT];

/* The type of functions that output data to the streams as required by an
 * instruction.
 *
 * In:
 *   argument 1: The instruction to output data for.
 * Out:
 *   Zero or more of the 'streams' have had data written into them.
 * Returns:
 *   true if the instruction is valid.
 *   false if the instruction would raise a Reserved Instruction exception or
 *   an internal error occurred during processing.
 */
typedef bool (*output_fn) (uint32_t);

/* The functions that dispatch to the proper minor opcode. */
static bool output_special(uint32_t);
static bool output_regimm(uint32_t);
static bool output_special2(uint32_t);
static bool output_sll(uint32_t);

/* The functions that output data to streams. */
static bool output_rd_rt_sa(uint32_t);
static bool output_rd_rs_rt(uint32_t);
static bool output_rs_rt(uint32_t);
static bool output_rs(uint32_t);
static bool output_rd(uint32_t);
static bool output_sa(uint32_t);
static bool output_rd_rs(uint32_t);
static bool output_j26a(uint32_t);
static bool output_rs_j16r(uint32_t);
static bool output_rs_rt_mem(uint32_t);
static bool output_rs_rt_j16r(uint32_t);
static bool output_rs_rt_imm16(uint32_t);
static bool output_rt_rs_imm16(uint32_t);
static bool output_rt_imm16(uint32_t);
static bool output_imm20(uint32_t);
static bool output_rest_0_25(uint32_t);

struct op_desc {
	output_fn output;
	/* To ensure that the transformations implemented by this filter are fully
	 * reversible, ensure that the unused fields of opcodes are all-zeroes. */
	uint32_t zeroes;
	enum flat_op flat_op;
};

/* These tables map MIPS opcodes to the functions that write the things they
 * need to the appropriate streams. */
static struct op_desc special[64] = {
	/* 0x00 */ { output_sll,         UINT32_C(0x03E00000), OP_NONE },
	/* 0x01 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x02 */ { output_rd_rt_sa,    UINT32_C(0x03E00000), OP_SRL },
	/* 0x03 */ { output_rd_rt_sa,    UINT32_C(0x03E00000), OP_SRA },
	/* 0x04 */ { output_rd_rs_rt,    UINT32_C(0x000007C0), OP_SLLV },
	/* 0x05 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x06 */ { output_rd_rs_rt,    UINT32_C(0x000007C0), OP_SRLV },
	/* 0x07 */ { output_rd_rs_rt,    UINT32_C(0x000007C0), OP_SRAV },
	/* 0x08 */ { output_rs,          UINT32_C(0x001FFFC0), OP_JR },
	/* 0x09 */ { output_rd_rs,       UINT32_C(0x001F07C0), OP_JALR },
	/* 0x0A */ { output_rd_rs_rt,    UINT32_C(0x000007C0), OP_MOVZ },
	/* 0x0B */ { output_rd_rs_rt,    UINT32_C(0x000007C0), OP_MOVN },
	/* 0x0C */ { output_imm20,       UINT32_C(0x00000000), OP_SYSCALL },
	/* 0x0D */ { output_imm20,       UINT32_C(0x00000000), OP_BREAK },
	/* 0x0E */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x0F */ { output_sa,          UINT32_C(0x03FFF800), OP_SYNC },
	/* 0x10 */ { output_rd,          UINT32_C(0x03FF07C0), OP_MFHI },
	/* 0x11 */ { output_rs,          UINT32_C(0x001FFFC0), OP_MTHI },
	/* 0x12 */ { output_rd,          UINT32_C(0x03FF07C0), OP_MFLO },
	/* 0x13 */ { output_rs,          UINT32_C(0x001FFFC0), OP_MTLO },
	/* 0x14 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x15 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x16 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x17 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x18 */ { output_rs_rt,       UINT32_C(0x0000FFC0), OP_MULT },
	/* 0x19 */ { output_rs_rt,       UINT32_C(0x0000FFC0), OP_MULTU },
	/* 0x1A */ { output_rs_rt,       UINT32_C(0x0000FFC0), OP_DIV },
	/* 0x1B */ { output_rs_rt,       UINT32_C(0x0000FFC0), OP_DIVU },
	/* 0x1C */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x1D */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x1E */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x1F */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x20 */ { output_rd_rs_rt,    UINT32_C(0x000007C0), OP_ADD },
	/* 0x21 */ { output_rd_rs_rt,    UINT32_C(0x000007C0), OP_ADDU },
	/* 0x22 */ { output_rd_rs_rt,    UINT32_C(0x000007C0), OP_SUB },
	/* 0x23 */ { output_rd_rs_rt,    UINT32_C(0x000007C0), OP_SUBU },
	/* 0x24 */ { output_rd_rs_rt,    UINT32_C(0x000007C0), OP_AND },
	/* 0x25 */ { output_rd_rs_rt,    UINT32_C(0x000007C0), OP_OR },
	/* 0x26 */ { output_rd_rs_rt,    UINT32_C(0x000007C0), OP_XOR },
	/* 0x27 */ { output_rd_rs_rt,    UINT32_C(0x000007C0), OP_NOR },
	/* 0x28 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x29 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x2A */ { output_rd_rs_rt,    UINT32_C(0x000007C0), OP_SLT },
	/* 0x2B */ { output_rd_rs_rt,    UINT32_C(0x000007C0), OP_SLTU },
	/* 0x2C */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x2D */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x2E */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x2F */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x30 */ { output_rs_rt_imm16, UINT32_C(0x00000000), OP_T },
	/* 0x31 */ { output_rs_rt_imm16, UINT32_C(0x00000000), OP_T },
	/* 0x32 */ { output_rs_rt_imm16, UINT32_C(0x00000000), OP_T },
	/* 0x33 */ { output_rs_rt_imm16, UINT32_C(0x00000000), OP_T },
	/* 0x34 */ { output_rs_rt_imm16, UINT32_C(0x00000000), OP_T },
	/* 0x35 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x36 */ { output_rs_rt_imm16, UINT32_C(0x00000000), OP_T },
	/* 0x37 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x38 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x39 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x3A */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x3B */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x3C */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x3D */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x3E */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x3F */ { NULL,               UINT32_C(0x00000000), OP_NONE },
};

static struct op_desc regimm[32] = {
	/* 0x00 */ { output_rs_j16r,     UINT32_C(0x00000000), OP_BLTZ },
	/* 0x01 */ { output_rs_j16r,     UINT32_C(0x00000000), OP_BGEZ },
	/* 0x02 */ { output_rs_j16r,     UINT32_C(0x00000000), OP_BLTZL },
	/* 0x03 */ { output_rs_j16r,     UINT32_C(0x00000000), OP_BGEZL },
	/* 0x04 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x05 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x06 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x07 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x08 */ { output_rs_rt_imm16, UINT32_C(0x00000000), OP_TI },
	/* 0x09 */ { output_rs_rt_imm16, UINT32_C(0x00000000), OP_TI },
	/* 0x0A */ { output_rs_rt_imm16, UINT32_C(0x00000000), OP_TI },
	/* 0x0B */ { output_rs_rt_imm16, UINT32_C(0x00000000), OP_TI },
	/* 0x0C */ { output_rs_rt_imm16, UINT32_C(0x00000000), OP_TI },
	/* 0x0D */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x0E */ { output_rs_rt_imm16, UINT32_C(0x00000000), OP_TI },
	/* 0x0F */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x10 */ { output_rs_j16r,     UINT32_C(0x00000000), OP_BLTZAL },
	/* 0x11 */ { output_rs_j16r,     UINT32_C(0x00000000), OP_BGEZAL },
	/* 0x12 */ { output_rs_j16r,     UINT32_C(0x00000000), OP_BLTZALL },
	/* 0x13 */ { output_rs_j16r,     UINT32_C(0x00000000), OP_BGEZALL },
	/* 0x14 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x15 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x16 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x17 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x18 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x19 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x1A */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x1B */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x1C */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x1D */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x1E */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x1F */ { NULL,               UINT32_C(0x00000000), OP_NONE },
};

static struct op_desc special2[64] = {
	/* 0x00 */ { output_rs_rt,       UINT32_C(0x0000FFC0), OP_MADD },
	/* 0x01 */ { output_rs_rt,       UINT32_C(0x0000FFC0), OP_MADDU },
	/* 0x02 */ { output_rd_rs_rt,    UINT32_C(0x000007C0), OP_MUL },
	/* 0x03 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x04 */ { output_rs_rt,       UINT32_C(0x0000FFC0), OP_MSUB },
	/* 0x05 */ { output_rs_rt,       UINT32_C(0x0000FFC0), OP_MSUBU },
	/* 0x06 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x07 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x08 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x09 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x0A */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x0B */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x0C */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x0D */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x0E */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x0F */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x10 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x11 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x12 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x13 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x14 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x15 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x16 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x17 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x18 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x19 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x1A */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x1B */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x1C */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x1D */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x1E */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x1F */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x20 */ { output_rd_rs_rt,    UINT32_C(0x000007C0), OP_CLZ },
	/* 0x21 */ { output_rd_rs_rt,    UINT32_C(0x000007C0), OP_CLO },
	/* 0x22 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x23 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x24 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x25 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x26 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x27 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x28 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x29 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x2A */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x2B */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x2C */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x2D */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x2E */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x2F */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x30 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x31 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x32 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x33 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x34 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x35 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x36 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x37 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x38 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x39 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x3A */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x3B */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x3C */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x3D */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x3E */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x3F */ { output_imm20,       UINT32_C(0x00000000), OP_SDBBP },
};

static struct op_desc major[64] = {
	/* 0x00 */ { output_special,     UINT32_C(0x00000000), OP_NONE },
	/* 0x01 */ { output_regimm,      UINT32_C(0x00000000), OP_NONE },
	/* 0x02 */ { output_j26a,        UINT32_C(0x00000000), OP_J },
	/* 0x03 */ { output_j26a,        UINT32_C(0x00000000), OP_JAL },
	/* 0x04 */ { output_rs_rt_j16r,  UINT32_C(0x00000000), OP_BEQ },
	/* 0x05 */ { output_rs_rt_j16r,  UINT32_C(0x00000000), OP_BNE },
	/* 0x06 */ { output_rs_j16r,     UINT32_C(0x001F0000), OP_BLEZ },
	/* 0x07 */ { output_rs_j16r,     UINT32_C(0x001F0000), OP_BGTZ },
	/* 0x08 */ { output_rt_rs_imm16, UINT32_C(0x00000000), OP_ADDI },
	/* 0x09 */ { output_rt_rs_imm16, UINT32_C(0x00000000), OP_ADDIU },
	/* 0x0A */ { output_rt_rs_imm16, UINT32_C(0x00000000), OP_SLTI },
	/* 0x0B */ { output_rt_rs_imm16, UINT32_C(0x00000000), OP_SLTIU },
	/* 0x0C */ { output_rt_rs_imm16, UINT32_C(0x00000000), OP_ANDI },
	/* 0x0D */ { output_rt_rs_imm16, UINT32_C(0x00000000), OP_ORI },
	/* 0x0E */ { output_rt_rs_imm16, UINT32_C(0x00000000), OP_XORI },
	/* 0x0F */ { output_rt_imm16,    UINT32_C(0x03E00000), OP_LUI },
	/* 0x10 */ { output_rest_0_25,   UINT32_C(0x00000000), OP_COP0 },
	/* 0x11 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x12 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x13 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x14 */ { output_rs_rt_j16r,  UINT32_C(0x00000000), OP_BEQL },
	/* 0x15 */ { output_rs_rt_j16r,  UINT32_C(0x00000000), OP_BNEL },
	/* 0x16 */ { output_rs_j16r,     UINT32_C(0x001F0000), OP_BLEZL },
	/* 0x17 */ { output_rs_j16r,     UINT32_C(0x001F0000), OP_BGTZL },
	/* 0x18 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x19 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x1A */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x1B */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x1C */ { output_special2,    UINT32_C(0x00000000), OP_NONE },
	/* 0x1D */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x1E */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x1F */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x20 */ { output_rs_rt_mem,   UINT32_C(0x00000000), OP_LB },
	/* 0x21 */ { output_rs_rt_mem,   UINT32_C(0x00000000), OP_LH },
	/* 0x22 */ { output_rs_rt_mem,   UINT32_C(0x00000000), OP_LWL },
	/* 0x23 */ { output_rs_rt_mem,   UINT32_C(0x00000000), OP_LW },
	/* 0x24 */ { output_rs_rt_mem,   UINT32_C(0x00000000), OP_LBU },
	/* 0x25 */ { output_rs_rt_mem,   UINT32_C(0x00000000), OP_LHU },
	/* 0x26 */ { output_rs_rt_mem,   UINT32_C(0x00000000), OP_LWR },
	/* 0x27 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x28 */ { output_rs_rt_mem,   UINT32_C(0x00000000), OP_SB },
	/* 0x29 */ { output_rs_rt_mem,   UINT32_C(0x00000000), OP_SH },
	/* 0x2A */ { output_rs_rt_mem,   UINT32_C(0x00000000), OP_SWL },
	/* 0x2B */ { output_rs_rt_mem,   UINT32_C(0x00000000), OP_SW },
	/* 0x2C */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x2D */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x2E */ { output_rs_rt_mem,   UINT32_C(0x00000000), OP_SWR },
	/* 0x2F */ { output_rs_rt_mem,   UINT32_C(0x00000000), OP_CACHE },
	/* 0x30 */ { output_rs_rt_mem,   UINT32_C(0x00000000), OP_LL },
	/* 0x31 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x32 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x33 */ { output_rs_rt_mem,   UINT32_C(0x00000000), OP_PREF },
	/* 0x34 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x35 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x36 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x37 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x38 */ { output_rs_rt_mem,   UINT32_C(0x00000000), OP_SC },
	/* 0x39 */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x3A */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x3B */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x3C */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x3D */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x3E */ { NULL,               UINT32_C(0x00000000), OP_NONE },
	/* 0x3F */ { NULL,               UINT32_C(0x00000000), OP_NONE },
};

static uint32_t read_u32(const uint8_t* buf)
{
	return  buf[0]        | (buf[1] << 8)
	     | (buf[2] << 16) | (buf[3] << 24);
}

static void write_u32(uint8_t* buf, uint32_t value)
{
	buf[0] = value;
	buf[1] = value >> 8;
	buf[2] = value >> 16;
	buf[3] = value >> 24;
}

static bool output_u8(enum stream_id stream_id, uint8_t value)
{
	struct stream* stream = &streams[stream_id];
	if (stream->size >= stream->capacity) {
		size_t new_capacity = ROUND_UP_4(stream->capacity * 5 / 4 + 1);
		uint8_t* new_data = realloc(stream->data, new_capacity);
		if (!new_data) {
			fprintf(stderr, "filter warning: failed to allocate %zu bytes of memory for stream #%d; some opcodes will not be filtered\n", new_capacity, stream_id);
			return false;
		}
		stream->data = new_data;
		stream->capacity = new_capacity;
	}

	stream->data[stream->size++] = value;
	return true;
}

static bool output_u16(enum stream_id stream_id, uint16_t value)
{
	if (!output_u8(stream_id, value & UINT8_C(0xFF))) return false;
	return output_u8(stream_id, (value >> 8) & UINT8_C(0xFF));
}

static bool output_u32(enum stream_id stream_id, uint32_t value)
{
	if (!output_u8(stream_id, value & UINT8_C(0xFF))) return false;
	if (!output_u8(stream_id, (value >> 8) & UINT8_C(0xFF))) return false;
	if (!output_u8(stream_id, (value >> 16) & UINT8_C(0xFF))) return false;
	return output_u8(stream_id, (value >> 24) & UINT8_C(0xFF));
}

static bool dispatch(const struct op_desc* desc, uint32_t opcode)
{
	if (desc->output) {
		if ((opcode & desc->zeroes) != 0)
			return false;
		if (!desc->output(opcode)) return false;
		/* Store the opcode last. This avoids a problem that happens if the
		 * output function runs out of memory after having written operands
		 * but the opcode was written first; the decoder would have 1 extra
		 * opcode without operands. If the opcode is written last, operands
		 * are written and waste some space, but running out of memory just
		 * asks the caller to write the instruction verbatim. */
		if (desc->flat_op != OP_NONE) {
			/* If more than one byte is needed to encode the flat_op, encode
			 * it with more bytes; the result is little-endian. */
			enum flat_op flat_op = desc->flat_op, end = OP_COUNT - 1;
			while (end != 0) {
				if (!output_u8(STRM_OPCODES, (uint8_t) flat_op)) return false;
				end >>= 8;
				flat_op >>= 8;
			}
		}
		return true;
	} else
		return false;
}

static bool output_special(uint32_t opcode)
{
	return dispatch(&special[opcode & UINT32_C(0x3F)], opcode);
}

static bool output_regimm(uint32_t opcode)
{
	return dispatch(&regimm[(opcode >> 16) & UINT32_C(0x1F)], opcode);
}

static bool output_special2(uint32_t opcode)
{
	return dispatch(&special2[opcode & UINT32_C(0x3F)], opcode);
}

static bool output_null(uint32_t opcode)
{
	(void) opcode;
	return true;
}

static bool output_sll(uint32_t opcode)
{
	static struct op_desc sll = { output_rd_rt_sa, UINT32_C(0x03E00000), OP_SLL };
	static struct op_desc nop = { output_null,     UINT32_C(0xFFFFFFFF), OP_NOP };
	if (opcode != 0)
		return dispatch(&sll, opcode);
	else
		return dispatch(&nop, opcode);
}

static bool output_rd_rt_sa(uint32_t opcode)
{
	if (!output_u8(STRM_REGS, (opcode >> 11) & UINT32_C(0x1F))) return false;
	if (!output_u8(STRM_REGS, (opcode >> 16) & UINT32_C(0x1F))) return false;
	return output_u8(STRM_SAS, (opcode >> 6) & UINT32_C(0x1F));
}

static bool output_rd_rs_rt(uint32_t opcode)
{
	if (!output_u8(STRM_REGS, (opcode >> 11) & UINT32_C(0x1F))) return false;
	if (!output_u8(STRM_REGS, (opcode >> 21) & UINT32_C(0x1F))) return false;
	return output_u8(STRM_REGS, (opcode >> 16) & UINT32_C(0x1F));
}

static bool output_rs_rt(uint32_t opcode)
{
	if (!output_u8(STRM_REGS, (opcode >> 21) & UINT32_C(0x1F))) return false;
	return output_u8(STRM_REGS, (opcode >> 16) & UINT32_C(0x1F));
}

static bool output_rs(uint32_t opcode)
{
	return output_u8(STRM_REGS, (opcode >> 21) & UINT32_C(0x1F));
}

static bool output_rd(uint32_t opcode)
{
	return output_u8(STRM_REGS, (opcode >> 11) & UINT32_C(0x1F));
}

static bool output_sa(uint32_t opcode)
{
	return output_u8(STRM_SAS, (opcode >> 6) & UINT32_C(0x1F));
}

static bool output_rd_rs(uint32_t opcode)
{
	if (!output_u8(STRM_REGS, (opcode >> 11) & UINT32_C(0x1F))) return false;
	return output_u8(STRM_REGS, (opcode >> 21) & UINT32_C(0x1F));
}

static bool output_j26a(uint32_t opcode)
{
	return output_u32(STRM_J26AS, opcode & UINT32_C(0x03FFFFFF));
}

static bool output_rs_j16r(uint32_t opcode)
{
	if (!output_u8(STRM_REGS, (opcode >> 21) & UINT32_C(0x1F))) return false;
	return output_u16(STRM_J16RS, opcode & UINT32_C(0xFFFF));
}

static bool output_rs_rt_mem(uint32_t opcode)
{
	if (!output_u8(STRM_REGS, (opcode >> 21) & UINT32_C(0x1F))) return false;
	if (!output_u8(STRM_REGS, (opcode >> 16) & UINT32_C(0x1F))) return false;
	return output_u16(STRM_MEMOFFS, opcode & UINT32_C(0xFFFF));
}

static bool output_rs_rt_j16r(uint32_t opcode)
{
	if (!output_u8(STRM_REGS, (opcode >> 21) & UINT32_C(0x1F))) return false;
	if (!output_u8(STRM_REGS, (opcode >> 16) & UINT32_C(0x1F))) return false;
	return output_u16(STRM_J16RS, opcode & UINT32_C(0xFFFF));
}

static bool output_rs_rt_imm16(uint32_t opcode)
{
	if (!output_u8(STRM_REGS, (opcode >> 21) & UINT32_C(0x1F))) return false;
	if (!output_u8(STRM_REGS, (opcode >> 16) & UINT32_C(0x1F))) return false;
	return output_u16(STRM_IMM16S, opcode & UINT32_C(0xFFFF));
}

static bool output_rt_rs_imm16(uint32_t opcode)
{
	if (!output_u8(STRM_REGS, (opcode >> 16) & UINT32_C(0x1F))) return false;
	if (!output_u8(STRM_REGS, (opcode >> 21) & UINT32_C(0x1F))) return false;
	return output_u16(STRM_IMM16S, opcode & UINT32_C(0xFFFF));
}

static bool output_rt_imm16(uint32_t opcode)
{
	if (!output_u8(STRM_REGS, (opcode >> 16) & UINT32_C(0x1F))) return false;
	return output_u16(STRM_IMM16S, opcode & UINT32_C(0xFFFF));
}

static bool output_imm20(uint32_t opcode)
{
	return output_u32(STRM_IMM20S, (opcode >> 6) & UINT32_C(0xFFFFF));
}

static bool output_rest_0_25(uint32_t opcode)
{
	return output_u32(STRM_REST, opcode & UINT32_C(0x03FFFFFF));
}

uint32_t make_streams(const uint8_t* buf, uint32_t n)
{
	size_t i;
	uint32_t ret;
	for (i = 0; i < STRM_COUNT; i++) {
		streams[i].data = malloc(ROUND_UP_4(n));
		if (!streams[i].data) {
			fprintf(stderr, "filter warning: failed to allocate %" PRIu32 " bytes of memory for stream #%zu; some opcodes will not be filtered\n", n, i);
		}
		streams[i].size = 0;
		streams[i].capacity = ROUND_UP_4(n);
	}

	/* Process as many words as we can as instructions. Stop at the first
	 * invalid opcode or memory allocation failure. */
	for (ret = 0; ret < n; ret++) {
		uint32_t opcode = read_u32(buf + ret * 4);
		if (!dispatch(&major[(opcode >> 26) & UINT32_C(0x3F)], opcode))
			break;
	}

	for (i = 0; i < STRM_COUNT; i++) {
		/* Align the streams to 4 bytes. */
		assert(ROUND_UP_4(streams[i].size) <= streams[i].capacity);
		/* Set the padding to <00> to avoid hurting compression too much. */
		memset(streams[i].data + streams[i].size, 0,
			ROUND_UP_4(streams[i].size) - streams[i].size);
		streams[i].size = ROUND_UP_4(streams[i].size);
	}

	fprintf(stderr, "Filtered %" PRIu32 " (= 0x%" PRIX32 ") bytes in %" PRIu32 " (= 0x%" PRIX32 ") instructions\n", ret * 4, ret * 4, ret, ret);

	return ret;
}

uint32_t get_filtered_size(void)
{
	uint32_t ret = STRM_COUNT * 4;
	size_t i;

	for (i = 0; i < STRM_COUNT; i++) {
		ret += streams[i].size;
	}

	return ret;
}

bool write_streams(FILE* outfile)
{
	size_t written, total_written, i;
	uint8_t stream_offsets[STRM_COUNT * 4];
	uint32_t offset = 0;

	write_u32(&stream_offsets[0], 0);
	for (i = 1; i < STRM_COUNT; i++) {
		offset += streams[i - 1].size;
		write_u32(&stream_offsets[i * 4], offset);
	}

	total_written = 0;
	while (total_written < sizeof(stream_offsets)) {
		written = fwrite(stream_offsets + total_written, 1, sizeof(stream_offsets) - total_written, outfile);
		if (ferror(outfile)) {
			fprintf(stderr, "filter fatal error: failed to write to output file: %s\n", strerror(errno));
			return false;
		}
		total_written += written;
	}

	for (i = 0; i < STRM_COUNT; i++) {
		total_written = 0;
		while (total_written < streams[i].size) {
			written = fwrite(streams[i].data + total_written, 1, streams[i].size - total_written, outfile);
			if (ferror(outfile)) {
				fprintf(stderr, "filter fatal error: failed to write to output file: %s\n", strerror(errno));
				return false;
			}
			total_written += written;
		}
	}

	return true;
}

void free_streams(void)
{
	size_t i;

	for (i = 0; i < STRM_COUNT; i++) {
		free(streams[i].data);
		streams[i].data = NULL;
		streams[i].size = streams[i].capacity = 0;
	}
}
