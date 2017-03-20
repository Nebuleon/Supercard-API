/*
 * This file is part of the C standard library for the Supercard DSTwo.
 *
 * Copyright 2006 Ingenic Semiconductor Inc.
 *                author: Seeger Chin <seeger.chin@gmail.com>
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

#include <asm/cachectl.h>
#include <archdefs.h>
#include "sysdefs.h"
#include <mipsregs.h>
#include <stdint.h>
#include <errno.h>

#define CACHE_SIZE       16384
#define CACHE_LINE_SIZE     32

#define INVALIDATE_BTB()			\
do {						\
	unsigned long tmp;			\
	__asm__ __volatile__(			\
	".set mips32\n\t"			\
	"mfc0 %0, $16, 7\n\t"			\
	"nop\n\t"				\
	"ori %0, 2\n\t"				\
	"mtc0 %0, $16, 7\n\t"			\
	"nop\n\t"				\
	".set mips0\n\t"			\
	: "=&r" (tmp));				\
} while (0)

#define SYNC_WB() __asm__ __volatile__ ("sync")

#define cache_op(op,addr)						\
	__asm__ __volatile__(						\
	"	.set	push			\n"			\
	"	.set	noreorder		\n"			\
	"	.set	mips32\n\t		\n"			\
	"	cache	%0, %1			\n"			\
	"	.set	pop"						\
	:										\
	: "i" (op), "m" (*(unsigned char *)(addr)))

static bool addr_valid(const void* addr) {
	return (uintptr_t) addr >= 0x80000000 && (uintptr_t) addr < 0x82000000;
}

void dcache_invalidate_all(void)
{
	uint8_t* line;

	write_c0_taghi(0);
	write_c0_taglo(0);

	for (line = (uint8_t*) A_K0BASE; line < (uint8_t*) A_K0BASE + CACHE_SIZE; line += CACHE_LINE_SIZE)
		cache_op(DCIndexStTag, line);

	SYNC_WB();
}

void dcache_writeback_invalidate_all(void)
{
	uint8_t* line;

	for (line = (uint8_t*) A_K0BASE; line < (uint8_t*) A_K0BASE + CACHE_SIZE; line += CACHE_LINE_SIZE)
		cache_op(DCIndexWBInv, line);

	SYNC_WB();
}


void icache_invalidate_all(void)
{
	uint8_t* line;

	write_c0_taghi(0);
	write_c0_taglo(0);

	for (line = (uint8_t*) A_K0BASE; line < (uint8_t*) A_K0BASE + CACHE_SIZE; line += CACHE_LINE_SIZE)
		cache_op(ICIndexStTag, line);

	SYNC_WB();
	/* Invalidating some or all of the instruction cache means we should
	 * also invalidate the branch target buffer for performance. Some
	 * instructions were rewritten and the branch target prediction is
	 * better redone from scratch for these new instructions. */
	INVALIDATE_BTB();
}

int dcache_writeback_range(const void* addr, size_t bytes)
{
	const uint8_t* start = addr;
	const uint8_t* end = start + bytes;
	const uint8_t* line;

	if (!addr_valid(start) || !addr_valid(start + (bytes - 1)))
		return EFAULT;

	start -= (uintptr_t) start & (CACHE_LINE_SIZE - 1);

	for (line = start; line < end; line += CACHE_LINE_SIZE)
		cache_op(DCHitWB, line);

	SYNC_WB();
	return 0;
}

int dcache_writeback_invalidate_range(const void* addr, size_t bytes)
{
	const uint8_t* start = addr;
	const uint8_t* end = start + bytes;
	const uint8_t* line;

	if (!addr_valid(start) || !addr_valid(start + (bytes - 1)))
		return EFAULT;

	start -= (uintptr_t) start & (CACHE_LINE_SIZE - 1);

	for (line = start; line < end; line += CACHE_LINE_SIZE)
		cache_op(DCHitWBInv, line);

	SYNC_WB();
	return 0;
}

int dcache_invalidate_range(const void* addr, size_t bytes)
{
	const uint8_t* start = addr;
	const uint8_t* end = start + bytes;
	const uint8_t* line;

	if (!addr_valid(start) || !addr_valid(start + (bytes - 1)))
		return EFAULT;

	start -= (uintptr_t) start & (CACHE_LINE_SIZE - 1);

	for (line = start; line < end; line += CACHE_LINE_SIZE)
		cache_op(DCHitInv, line);

	SYNC_WB();
	return 0;
}

int icache_invalidate_range(const void* addr, size_t bytes)
{
	const uint8_t* start = addr;
	const uint8_t* end = start + bytes;
	const uint8_t* line;

	if (!addr_valid(start) || !addr_valid(start + (bytes - 1)))
		return EFAULT;

	start -= (uintptr_t) start & (CACHE_LINE_SIZE - 1);

	for (line = start; line < end; line += CACHE_LINE_SIZE)
		cache_op(ICHitInv, line);

	SYNC_WB();
	/* Invalidating some or all of the instruction cache means we should
	 * also invalidate the branch target buffer for performance. Some
	 * instructions were rewritten and the branch target prediction is
	 * better redone from scratch for these new instructions. */
	INVALIDATE_BTB();

	return 0;
}

int _flush_cache(const void* addr, int bytes, int caches)
{
	if ((caches & ~BCACHE) != 0 || bytes < 0)
		return EINVAL;

	if (bytes > 0) {
		int result;
		if (caches) {
			if ((result = dcache_writeback_invalidate_range(addr, bytes)) != 0) {
				return result;
			}
		}
		if (caches & ICACHE) {
			if ((result = icache_invalidate_range(addr, bytes)) != 0) {
				return result;
			}
		}
	}
	return 0;
}
