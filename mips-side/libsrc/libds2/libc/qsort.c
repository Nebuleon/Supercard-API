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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*
 * This is an implementation of Donald Shell's Shell sort, an improvement over
 * insertion sort and bubble sort. The algorithm works by quickly bringing low
 * values near the beginning of the array and high values near its end. Values
 * are first compared and swapped across very large strides, which get smaller
 * as the algorithm proceeds. The last step is an insertion sort (stride = 1),
 * which has less work to do because all elements have been brought near their
 * final destination.
 *
 * The chosen stride sequence is ... 127, 31, 7, 1, which are powers of 2 with
 * 1 subtracted from them, to utilise the cache efficiently.
 *
 * Shell sort's access pattern tends to be sequential. Its cache behavior is a
 * bit tricky, but can be boiled down to sequential access among many streams.
 * Thus, a processor with high cache associativity (= many ways per cache set)
 * will be able to handle more of those streams. The stride has 1 removed from
 * the power of 2 in order to help processors with low cache associativity; in
 * those processors, the accesses being 1 element away from a power of 2 would
 * sometimes allow the cache to hold elements from different strides in 2 sets
 * instead of 2 ways, which the processor may not have.
 *
 * During each pass through the array with a new stride, with sufficient cache
 * associativity:
 *
 * - Reads from two strides are initially interleaved with each other, and, if
 *   the processor reads whole cache lines surrounding any element accessed, a
 *   certain number of accesses in both of those strides are essentially free.
 * - More and more strides get added as the algorithm progresses. At a certain
 *   point, the cache begins to replace lines that are actively being read but
 *   not all of them are immediately lost due to the cache associativity.
 *
 * The final pass reads elements up to 7 away, so it benefits from prefetching
 * of whole cache lines and no longer requires cache associativity.
 */

static inline uint8_t* el_ptr(void* base, size_t size, size_t i)
{
	return (uint8_t*) base + i * size;
}

static void shellsort(void* a, size_t n, size_t stride, size_t s,
	int (*cmp) (const void*, const void*))
{
	uint8_t el_b[s];
	size_t i, r;

	for (i = stride; i < n; i++) {
		r = i;

		memcpy(el_b, el_ptr(a, s, r), s);

		while (r >= stride && cmp(el_ptr(a, s, r - stride), el_b) > 0) {
			/* A sequence point occurs immediately before and immediately
			 * after each call to the comparison function [as above], and
			 * also between any call to the comparison function and any
			 * movement of the objects passed as arguments to that call
			 * [as below].
			 * - C99 */
			memcpy(el_ptr(a, s, r), el_ptr(a, s, r - stride), s);
			r -= stride;
		}

		if (r != i) {
			memcpy(el_ptr(a, s, r), el_b, s);
		}
	}
}

void qsort(void* a, size_t n, size_t s, int (*cmp) (const void*, const void*))
{
	/* Select the initial stride of the Shell sort such that it's 1 less than
	 * a power of 4 multiplied by 2 (..., 127, 31, 7, 1). If this sequence is
	 * changed, make sure the final pass has a stride of 1 to perform a final
	 * insertion sort. Currently, it does, because 7 / 4 == 1. */
	size_t guess = 2, stride;
	do {
		stride = guess;
		guess *= 4;
	} while (guess > stride /* prevent overflow */ && guess < n);
	stride--;

	while (stride != 0) {
		shellsort(a, n, stride, s, cmp);
		stride /= 4;
	}
}
