#ifndef __EXCEPT_H__
#define __EXCEPT_H__

#include <stdint.h>

/* TODO Make this a part of the public API if exposing exception handlers */
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

#endif /* !__EXCEPT_H__ */