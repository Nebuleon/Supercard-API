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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* These constants are chosen to be reasonably distinct from common values
 * (e.g. 0 and 1), distinct from each other (obviously), and easily loaded
 * by a single MIPS instruction (LUI or ORI). */
#define CHUNK_STATUS_FREE UINT32_C(0x55AA0000)
#define CHUNK_STATUS_USED UINT32_C(0x0000AA55)
#define MALLOC_ALIGNMENT 8  /* must be a power of two */

enum half {
	/* Refers to the half with the lowest address of a split chunk. */
	FIRST,
	/* Refers to the half with the highest address of a split chunk. */
	SECOND
};

struct chunk {
	struct chunk* prev;
	struct chunk* next;
	size_t size;
	uint32_t status;
};

static struct chunk* first_chunk;

static size_t heap_size;

static inline void* data_for(struct chunk* header)
{
	return (void*) (header + 1);
}

static inline struct chunk* header_for(void* data)
{
	return (struct chunk*) data - 1;
}

/*
 * Adjusts the header of the given chunk to include the size of the next one
 * inside it.
 */
static struct chunk* merge_with_next(struct chunk* one)
{
	struct chunk* two = one->next;
	struct chunk* three = two->next;

	one->size += two->size + sizeof(struct chunk);
	one->next = three;
	if (three != NULL) {
		three->prev = one;
	}
	return one;
}

/*
 * Marks the given chunk as having been freed. Neighboring free chunks are
 * brought into the given chunk as well.
 */
static struct chunk* newly_free(struct chunk* chunk)
{
	if (chunk != NULL) {
		chunk->status = CHUNK_STATUS_FREE;

		if (chunk->prev != NULL && chunk->prev->status == CHUNK_STATUS_FREE)
			chunk = merge_with_next(chunk->prev);

		if (chunk->next != NULL && chunk->next->status == CHUNK_STATUS_FREE)
			chunk = merge_with_next(chunk);
	}
	return chunk;
}

/*
 * Marks the given chunk as having been allocated.
 */
static struct chunk* newly_used(struct chunk* chunk)
{
	if (chunk != NULL) {
		chunk->status = CHUNK_STATUS_USED;
	}
	return chunk;
}

/*
 * Splits the given chunk, creating a second half having the status
 * CHUNK_STATUS_FREE. If there is not enough space for the second chunk
 * header, then none is added.
 *
 * In:
 *   chunk: The chunk to split.
 *   n: The number of bytes in the first half of the split.
 *   half: Which half of the split to be returned.
 * Returns:
 *   half == FIRST: chunk.
 *   half == SECOND: The second half of the split, or NULL if one could not be
 *     made.
 */
static struct chunk* split(struct chunk* chunk, size_t n, enum half half)
{
	if (chunk->size >= n + sizeof(struct chunk) + 4) {
		/* There is enough space left after this allocation to split the chunk
		 * into two and leave a meaningful second half marked as free. */
		struct chunk* part = (struct chunk*) ((uint8_t*) data_for(chunk) + n);
		struct chunk* next = chunk->next;

		part->status = CHUNK_STATUS_FREE;
		part->next = next;
		part->prev = chunk;
		part->size = chunk->size - n - sizeof(struct chunk);

		chunk->size = n;

		chunk->next = part;
		if (next != NULL) {
			next->prev = part;
		}
		if (half == SECOND) return part;
	} else {
		/* There is not enough space left after this allocation to make a
		 * header for the split. */
		if (half == SECOND) return NULL;
	}
	return chunk;
}

void _heap_init(uint8_t* start, uint8_t* end)
{
	/* Align 'start' to the next full word. */
	start += (4 - ((uintptr_t) start & 0x3)) & 0x3;
	/* Align 'end' to the previous full word. */
	end -= (uintptr_t) end & 0x3;
	heap_size = end - start;
	first_chunk = (struct chunk*) start;
	first_chunk->prev = first_chunk->next = NULL;
	first_chunk->size = (uint8_t*) end - (uint8_t*) start - sizeof(struct chunk);

	newly_free(first_chunk);
}

void* memalign(size_t alignment, size_t n)
{
	struct chunk* chunk;

	if (n == 0 || n > heap_size)
		return NULL;

	if ((alignment & (alignment - 1)) != 0)
		return NULL; /* alignment is not a power of two */
	if (alignment < sizeof(void*))
		alignment = sizeof(void*);

	n = (n + (MALLOC_ALIGNMENT - 1)) & ~(MALLOC_ALIGNMENT - 1);

	chunk = first_chunk;

	while (chunk != NULL) {
		if (chunk->status == CHUNK_STATUS_FREE) {
			bool misaligned = ((uintptr_t) data_for(chunk) & (alignment - 1)) != 0;
			if (misaligned) {
				/* We need to get the bookkeeping information for the aligned
				 * data right next to it, or we can't handle realloc or free
				 * for it. That means we have to see if there's enough space
				 * for a small chunk of free memory:
				 *         _____ NEW STUFF _____
				 *        /                     \
				 * [chunk]  free  [SECOND CHUNK] [new allocation]
				 *        <->| aligned          | ALIGNED */
				uint8_t* data = data_for(chunk);
				uint8_t* data_2 = data + sizeof(struct chunk);
				/* To avoid creating a free chunk of size 0, add these bytes
				 * regardless of whether the new data_2 was already aligned. */
				data_2 += alignment - (((uintptr_t) data_2) & (alignment - 1));
				if (chunk->size >= n + (data_2 - data)) {
					chunk = split(chunk, (data_2 - data) - sizeof(chunk), SECOND);
					return data_for(newly_used(split(chunk, n, FIRST)));
				}
			} else {
				/* The data is already aligned here; if the size is correct,
				 * we can use this chunk for bookkeeping information. */
				if (chunk->size >= n) {
					return data_for(newly_used(split(chunk, n, FIRST)));
				}
			}
		}

		chunk = chunk->next;
	}

	return NULL;
}

void* malloc(size_t n)
{
	return memalign(MALLOC_ALIGNMENT, n);
}

void* realloc(void* data, size_t n)
{
	struct chunk* chunk;

	if (data == NULL)
		return malloc(n);
	if (n == 0) {
		free(data);
		return NULL;
	}

	n = (n + (MALLOC_ALIGNMENT - 1)) & ~(MALLOC_ALIGNMENT - 1);

	chunk = header_for(data);

	if (n < chunk->size) {
		/* Shrinking an existing allocation. This creates a free
		 * chunk. */
		newly_free(split(chunk, n, SECOND));
		return data;
	} else if (n > chunk->size) {
		/* Growing an existing allocation. */
		struct chunk* next = chunk->next;
		if (next != NULL && next->status == CHUNK_STATUS_FREE
		 && next->size <= n - chunk->size) {
			/* If the next chunk is free and has sufficient space for the
			 * remainder of the new size, we will merge this one with some of
			 * it, making it used. */
			newly_used(split(next, n - chunk->size, FIRST));
			return data_for(merge_with_next(chunk));
		} else {
			/* We must make a new allocation (which may fail), copy the old
			 * allocation's contents into it and return it. */
			void* new_alloc = malloc(n);
			if (new_alloc != NULL) {
				memcpy(new_alloc, data, chunk->size);
			}
			return new_alloc;
		}
	} else {
		/* The allocation's size is exactly the same. Do nothing. */
		return data;
	}
}

void free(void* data)
{
	if (data != NULL) {
		struct chunk* chunk = header_for(data);

		newly_free(chunk);
	}
}
