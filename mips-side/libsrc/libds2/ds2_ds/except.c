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

void _unhandled_exception(uint32_t excode, const struct _register_block* registers)
{
	_ds2_ds.exception.excode = excode;
	_ds2_ds.exception.at = registers->at;
	_ds2_ds.exception.v0 = registers->v0;
	_ds2_ds.exception.v1 = registers->v1;
	_ds2_ds.exception.a0 = registers->a0;
	_ds2_ds.exception.a1 = registers->a1;
	_ds2_ds.exception.a2 = registers->a2;
	_ds2_ds.exception.a3 = registers->a3;
	_ds2_ds.exception.t0 = registers->t0;
	_ds2_ds.exception.t1 = registers->t1;
	_ds2_ds.exception.t2 = registers->t2;
	_ds2_ds.exception.t3 = registers->t3;
	_ds2_ds.exception.t4 = registers->t4;
	_ds2_ds.exception.t5 = registers->t5;
	_ds2_ds.exception.t6 = registers->t6;
	_ds2_ds.exception.t7 = registers->t7;
	_ds2_ds.exception.s0 = registers->s0;
	_ds2_ds.exception.s1 = registers->s1;
	_ds2_ds.exception.s2 = registers->s2;
	_ds2_ds.exception.s3 = registers->s3;
	_ds2_ds.exception.s4 = registers->s4;
	_ds2_ds.exception.s5 = registers->s5;
	_ds2_ds.exception.s6 = registers->s6;
	_ds2_ds.exception.s7 = registers->s7;
	_ds2_ds.exception.t8 = registers->t8;
	_ds2_ds.exception.t9 = registers->t9;
	_ds2_ds.exception.gp = registers->gp;
	_ds2_ds.exception.sp = registers->sp;
	_ds2_ds.exception.fp = registers->fp;
	_ds2_ds.exception.ra = registers->ra;
	_ds2_ds.exception.hi = registers->hi;
	_ds2_ds.exception.lo = registers->lo;
	_ds2_ds.exception.c0_epc = registers->c0_epc;
	if (registers->c0_epc >= 0x80000000 && registers->c0_epc <= 0x81FFFFF8
	 && (registers->c0_epc & 3) == 0) {
		_ds2_ds.exception.mapped = 1;
		_ds2_ds.exception.op = *(uint32_t*) registers->c0_epc;
		_ds2_ds.exception.next_op = *((uint32_t*) registers->c0_epc + 1);
	} else {
		_ds2_ds.exception.mapped = 0;
		_ds2_ds.exception.op = 0;
		_ds2_ds.exception.next_op = 0;
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
	_send_reply_4(DATA_KIND_MIPS_EXCEPTION | DATA_ENCODING(0) | DATA_BYTE_COUNT(sizeof(_ds2_ds.exception)) | DATA_END);
	_send_reply(&_ds2_ds.exception, sizeof(_ds2_ds.exception));

	_ds2_ds.pending_sends = 0;
}
