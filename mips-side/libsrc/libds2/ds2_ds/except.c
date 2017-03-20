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

#include <ds2/pm.h>
#include <stdint.h>

#include "../except.h"
#include "../intc.h"
#include "card_protocol.h"
#include "globals.h"

static struct card_reply_mips_exception exception __attribute__((aligned (32)));

void _unhandled_exception(uint32_t excode, const struct _register_block* registers)
{
	exception.excode = excode;
	exception.at = registers->at;
	exception.v0 = registers->v0;
	exception.v1 = registers->v1;
	exception.a0 = registers->a0;
	exception.a1 = registers->a1;
	exception.a2 = registers->a2;
	exception.a3 = registers->a3;
	exception.t0 = registers->t0;
	exception.t1 = registers->t1;
	exception.t2 = registers->t2;
	exception.t3 = registers->t3;
	exception.t4 = registers->t4;
	exception.t5 = registers->t5;
	exception.t6 = registers->t6;
	exception.t7 = registers->t7;
	exception.s0 = registers->s0;
	exception.s1 = registers->s1;
	exception.s2 = registers->s2;
	exception.s3 = registers->s3;
	exception.s4 = registers->s4;
	exception.s5 = registers->s5;
	exception.s6 = registers->s6;
	exception.s7 = registers->s7;
	exception.t8 = registers->t8;
	exception.t9 = registers->t9;
	exception.gp = registers->gp;
	exception.sp = registers->sp;
	exception.fp = registers->fp;
	exception.ra = registers->ra;
	exception.hi = registers->hi;
	exception.lo = registers->lo;
	exception.c0_epc = registers->c0_epc;
	if (registers->c0_epc >= 0x80000000 && registers->c0_epc <= 0x81FFFFF8
	 && (registers->c0_epc & 3) == 0) {
		exception.mapped = 1;
		exception.op = *(uint32_t*) registers->c0_epc;
		exception.next_op = *((uint32_t*) registers->c0_epc + 1);
	} else {
		exception.mapped = 0;
		exception.op = 0;
		exception.next_op = 0;
	}

	uint32_t section = DS2_EnterCriticalSection();
	_add_pending_send(PENDING_SEND_EXCEPTION);
	DS2_LeaveCriticalSection(section);

	while (1) {
		DS2_AwaitInterrupt();
	}
}

void _send_exception(void)
{
	_send_reply_4(DATA_KIND_MIPS_EXCEPTION | DATA_ENCODING(0) | DATA_BYTE_COUNT(sizeof(exception)) | DATA_END);
	_send_reply(&exception, sizeof(exception));

	_pending_sends = 0;
}
