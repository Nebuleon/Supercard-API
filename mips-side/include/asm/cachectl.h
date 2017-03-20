/*
 * This file is part of the C standard library for the Supercard DSTwo.
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

#ifndef __ASM_CACHECTL_H__
#define __ASM_CACHECTL_H__

#include <stddef.h>

/* Writes the contents of the data cache to RAM for an address range.
 *
 * The intent is that the data cache is still considered to contain the most
 * recent version of memory in the address range for future accesses by the
 * CPU, but RAM must contain the same data because it will be read by DMA or
 * be used to fill the instruction cache.
 *
 * When writing the data cache to RAM after filling it with new instructions,
 * use this function first, then icache_invalidate_range.
 */
extern int dcache_writeback_range(const void* start, size_t bytes);

/* Writes the contents of the data cache to RAM for an address range and
 * considers it invalid.
 *
 * The intent is that the data cache contains the most recent version of
 * memory but must no longer be considered to contain it, because the current
 * contents will be read by DMA, then possibly overwritten by another DMA.
 */
extern int dcache_writeback_invalidate_range(const void* start, size_t bytes);

/* Considers the contents of the data cache for an address range invalid.
 *
 * The intent is that the current contents of the data cache, if any, no
 * longer matter because they are about to be overwritten by DMA.
 */
extern int dcache_invalidate_range(const void* start, size_t bytes);

/* Considers the contents of the instruction cache for an address range
 * invalid.
 *
 * The intent is that the current contents of the instruction cache, if any,
 * are stale code that has been rewritten since being previously executed.
 */
extern int icache_invalidate_range(const void* start, size_t bytes);

/* Writes the contents of the data cache to RAM for an address range and
 * considers it invalid.
 *
 * Call this function instead of dcache_writeback_invalidate_range when so
 * much data has been modified that writing back and invalidating the range
 * would take more time than iterating through the data cache itself.
 */
extern void dcache_writeback_invalidate_all(void);

/* Considers the entire data cache to be invalid.
 *
 * The intent is that the contents of any volatile RAM no longer matter, e.g.
 * before a reset, and the contents of the data cache MUST NOT be written when
 * the reset completes (and new data would be overwritten by stale data that
 * the processor considers to be recent).
 */
extern void dcache_invalidate_all(void);

/* Considers the entire instruction cache to be invalid.
 *
 * The intent is that none of the currently loaded code matters anymore
 * because a new executable is about to be loaded.
 */
extern void icache_invalidate_all(void);

#define ICACHE 1
#define DCACHE 2
#define BCACHE 3

/* Flushes the contents of the specified caches for an address range.
 *
 * The intent of this function is that the caches contain the most recent
 * version of memory, but it must be written to RAM and the caches must no
 * longer be considered to contain it.
 *
 * In:
 *   addr: The starting address of the range.
 *   bytes: The length of the range.
 *   caches: The caches to be flushed. This is either of:
 *     - ICACHE: The instruction cache.
 *     - DCACHE: The data cache.
 *     - BCACHE: Both the instruction and data caches.
 * Returns:
 *   0: Success.
 *   EFAULT: Some or all of the address range is invalid.
 *   EINVAL: 'bytes' is negative or 'caches' contains bits that are not
 *     ICACHE, DCACHE or BCACHE.
 * Specified somewhat by Linux, which has this header on MIPS, and gcc, which
 * requires a cache-flushing function having these parameters for internal
 * usage.
 */
extern int _flush_cache(const void* addr, int bytes, int caches);

#define cacheflush _flush_cache

#endif /* !__ASM_CACHECTL_H__ */
