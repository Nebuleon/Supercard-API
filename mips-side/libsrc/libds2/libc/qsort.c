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
 * The chosen stride sequence is either ... 63, 15, 3, 1 or ... 127, 31, 7, 1,
 * depending on the number of elements in the array. They are powers of 2 with
 * 1 subtracted from them. If they were powers of 2 outright, it would be more
 * likely that two elements to be compared, being a large stride apart, occupy
 * the same cache line and slow down the comparison and swap operations.
 *
 * This Shell sort's computational complexity tends towards O(n^1.5).
 *
 * Shell sort's access pattern tends to be sequential:
 *
 * - The initial pass interleaves reads from the second half of the array with
 *   reads from the first half of the array. Therefore, the algorithm benefits
 *   from the processor reading whole cache lines following any element and it
 *   does not swap any element pairs that were not in the cache already.
 * - The final pass benefits from the processor reading whole cache lines near
 *   the elements it needs to read and write, and only needs the cache to hold
 *   the values of elements up to 7 indices away.
 *
 * Therefore, the processor can stream large amounts of data through the cache
 * in these passes. The access pattern of the middle passes is different: they
 * move elements many strides away, and thus need to create more than 2 "cache
 * streams". Due to the stride sequence, these streams should not be accessing
 * the same cache lines, however.
 */

static void swap(void* restrict el_t,
	uint8_t* restrict el_a, uint8_t* restrict el_b,
	size_t size)
{
	if (el_t != NULL) {
		memcpy(el_t, el_a, size);
		memcpy(el_a, el_b, size);
		memcpy(el_b, el_t, size);
	} else {
		size_t i;
		uint8_t byte_t;
		for (i = 0; i < size; i++) {
			byte_t = *el_a;
			*el_a++ = *el_b;
			*el_b++ = byte_t;
		}
	}
}

static void shellsort(void* restrict el_t, void* restrict base,
	size_t n, size_t stride, size_t size,
	int (*comparison) (const void*, const void*))
{
	size_t i, j, hops;
	size_t max_hops = 1; /* Stride hops allowed from 'i' to 0 */
	size_t next_hop = stride; /* Number of elements until 'max_hops' increases */

	for (i = stride; i < n; i++) {
		next_hop--;
		if (next_hop == 0) {
			max_hops++;
			next_hop = stride;
		}

		/* Move element 'i' to 'i - n * stride', where 'n' is between 1 and
		 * 'max_hops', inclusive. 'j' tracks the value of 'n', and elements
		 * are moved one stride at a time. */
		for (hops = 0, j = i; hops < max_hops; hops++, j -= stride) {
			void* el_a = (uint8_t*) base + (j - stride) * size;
			void* el_b = (uint8_t*) base + j * size;
			if ((*comparison) (el_a, el_b) > 0) {
				/* A sequence point occurs immediately before and immediately
				 * after each call to the comparison function [as above], and
				 * also between any call to the comparison function and any
				 * movement of the objects passed as arguments to that call
				 * [as below].
				 * - C99 */
				swap(el_t, el_a, el_b, size);
			}
		}
	}
}

static void inssort(void* restrict el_t, void* restrict base,
	size_t n, size_t size,
	int (*comparison) (const void*, const void*))
{
	size_t i, j;

	for (i = 1; i < n; i++) {
		for (j = i; j >= 1; j--) {
			void* el_a = (uint8_t*) base + (j - 1) * size;
			void* el_b = (uint8_t*) base + j * size;
			if ((*comparison) (el_a, el_b) > 0) {
				/* A sequence point occurs immediately before and immediately
				 * after each call to the comparison function [as above], and
				 * also between any call to the comparison function and any
				 * movement of the objects passed as arguments to that call
				 * [as below].
				 * - C99 */
				swap(el_t, el_a, el_b, size);
			}
		}
	}
}

void qsort(void* base, size_t n, size_t size,
	int (*comparison) (const void*, const void*))
{
	if (n <= 1)
		return;
	else {
		void* el_t = malloc(size); /* Temporary element for swaps */
		/* Select the initial stride of the Shell sort such that it's 1 below
		 * the largest power of 4 that fits in half of 'n'. */
		size_t guess = 1, stride;
		do {
			stride = guess;
			guess *= 4;
		} while (guess > stride /* prevent overflow */ && guess < n / 2);
		stride--;

		while (stride > 1) {
			shellsort(el_t, base, n, stride, size, comparison);
			stride /= 4;
		}
		inssort(el_t, base, n, size, comparison);

		free(el_t);
	}
}
