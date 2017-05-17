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

#ifndef __DS2_DS_EXCEPT_H__
#define __DS2_DS_EXCEPT_H__

struct _register_block {
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
	unsigned long ra;    /* Return address of the interrupted function */
	unsigned long hi;    /* Multiplier unit HI */
	unsigned long lo;    /* Multiplier unit LO */
	uint32_t c0_status;
	uint32_t c0_epc;     /* Address of the instruction causing the exception */
};

extern void _send_exception(void);

#endif /* !__DS2_DS_EXCEPT_H__ */
