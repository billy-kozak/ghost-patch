/******************************************************************************
* Copyright (C) 2019  Billy Kozak                                             *
*                                                                             *
* This file is part of the ghost-patch program                                *
*                                                                             *
* This program is free software: you can redistribute it and/or modify        *
* it under the terms of the GNU Lesser General Public License as published by *
* the Free Software Foundation, either version 3 of the License, or           *
* (at your option) any later version.                                         *
*                                                                             *
* This program is distributed in the hope that it will be useful,             *
* but WITHOUT ANY WARRANTY; without even the implied warranty of              *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
* GNU Lesser General Public License for more details.                         *
*                                                                             *
* You should have received a copy of the GNU Lesser General Public License    *
* along with this program.  If not, see <http://www.gnu.org/licenses/>.       *
******************************************************************************/
/******************************************************************************
*                                  INCLUDES                                   *
******************************************************************************/
#include "ghost-malloc.h"

#include "gmalloc-chunk.h"
#include "gmalloc-chunk-list.h"
#include "gmalloc/gmalloc-chunk-types.h"
#include "gmalloc/gmalloc-maps.h"

#include <safe_syscalls.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
/******************************************************************************
*                                   MACROS                                    *
******************************************************************************/
#define NUM_SMALL_BINS 61
#define NUM_LARGE_BINS 63

#define SMALL_BINS_SEPERATION 8
#define SMALL_BINS_MIN_SIZE MIN_CHUNK_DATA_SIZE
#define SMALL_BINS_MAX_SIZE ( \
	(NUM_SMALL_BINS * SMALL_BINS_SEPERATION) + SMALL_BINS_MIN_SIZE \
)

#define HEAP_OVERHEAD_SIZE ( \
	sizeof(struct ghost_heap) - \
	sizeof(((struct chunk*)(NULL))->payload) \
)

#define CHUNK_SPLIT_THRESHOLD 64

#define MIN_PAGES_FOR_MALLOC_ALLOC 4
/******************************************************************************
*                                    DATA                                     *
******************************************************************************/
static int page_size;
/******************************************************************************
*                                    TYPES                                    *
******************************************************************************/
struct ghost_heap {
	struct chunk *top_chunk;
	size_t top_flags;

	struct link* unsorted_bin;
	struct link* small_bins[NUM_SMALL_BINS];
	struct link* large_bins[NUM_LARGE_BINS];

	struct chunk first_chunk;
};
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
static size_t min_to_map(size_t target);
static void heap_maintenance(struct ghost_heap *heap);
static void *pure_mmap_alloc(struct ghost_heap *heap, size_t size);
static void *normal_malloc_alloc(struct ghost_heap *heap, size_t size);
static struct chunk *bin_pop(struct ghost_heap *heap, size_t size);
static struct chunk *bin_search(
	struct ghost_heap *heap, size_t size, struct link ***bin_ptr
);
static int small_bin_index(size_t size);
static int large_bin_index(size_t size);
static struct chunk *alloc_on_top(struct ghost_heap * heap, size_t size);
static bool should_split(struct chunk *chunk, size_t desired);
static void split_chunk(
	struct ghost_heap *heap, struct chunk *chunk, size_t desired
);
static void merge_chunks(struct ghost_heap *heap);
static void sort_chunks(struct ghost_heap * heap);
static void insert_small(struct ghost_heap *heap, struct chunk *chunk);
static void insert_large(struct ghost_heap *heap, struct chunk *chunk);
static int extend_mmaped_chunk(struct chunk *chunk, size_t desired_size);
static void bin_append(struct link **bin, struct link *new);
/******************************************************************************
*                              STATIC FUNCTIONS                               *
******************************************************************************/
static bool is_chunk_small(const struct chunk *chunk)
{
	return chunk_read_size(chunk) <= SMALL_BINS_MAX_SIZE;
}
/*****************************************************************************/
static bool is_chunk_free(
	const struct ghost_heap *heap,
	const struct chunk *chunk
) {
	struct chunk *next = chunk_next_after(chunk);

	if(next == NULL) {
		if(chunk_read_flag(chunk, MMAPED_CHUNK)) {
			return false;
		} else if((heap->top_flags & PREV_IN_USE) != 0) {
			return false;
		} else {
			return true;
		}
	} else if(chunk_read_flag(next, PREV_IN_USE)) {
		return false;
	} else {
		return true;
	}
}
/*****************************************************************************/
static size_t min_to_map(size_t target)

{
	size_t pages = DIV_ROUND_UP(target, page_size);

	return pages * page_size;
}
/*****************************************************************************/
static int small_bin_index(size_t size)
{
	if(size <= SMALL_BINS_MIN_SIZE) {
		return 0;
	} else {
		return (
			(size + SMALL_BINS_MIN_SIZE) / SMALL_BINS_SEPERATION
		) + ( (size % SMALL_BINS_SEPERATION) ? 1 : 0 );
	}
}
/*****************************************************************************/
static int large_bin_index(size_t size)
{
	const int spacings[] = {
		64, 512, 4096, 32768, 262144
	};
	const int thresholds[] = {
		32, 16, 8, 4, 2
	};

	size_t count_spacings = sizeof(spacings) / sizeof(*spacings);

	size_t last_threshold_point = 0;
	size_t threshold_point = SMALL_BINS_MAX_SIZE;

	int cumulative_threshold = 0;

	for(int i = 0; i < count_spacings; i++) {
		last_threshold_point = threshold_point;
		threshold_point += spacings[i] * thresholds[i];

		if(size < threshold_point) {
			size_t diff = size - last_threshold_point;
			return (diff / spacings[i]) + cumulative_threshold;
		}
		cumulative_threshold += thresholds[i];
	}
	return NUM_LARGE_BINS - 1;
}
/*****************************************************************************/
static struct link** get_bin(
	struct ghost_heap *heap, const struct chunk *chunk
) {
	if(is_chunk_small(chunk)) {
		int idx = small_bin_index(chunk_read_size(chunk));
		return &heap->small_bins[idx];
	} else {
		int idx = large_bin_index(chunk_read_size(chunk));
		return &heap->large_bins[idx];
	}
}
/*****************************************************************************/
static struct link** get_confirmed_bin(
	struct ghost_heap *heap, const struct chunk *chunk
) {
	struct link **sorted_bin = get_bin(heap, chunk);
	struct link **candidates[] = {&heap->unsorted_bin, sorted_bin};

	for(int i = 0; i < 2; i++) {
		struct link **bin = candidates[i];

		if(*bin == NULL) {
			continue;
		}

		for(
			struct chunk *c = chunk_ll_next_chunk(*bin, NULL);
			c != NULL;
			c = chunk_ll_next_chunk(*bin, c)
		) {
			if(c == chunk) {
				return bin;
			}
		}
	}

	return NULL;
}
/*****************************************************************************/
static void pop_from_ll_and_bin(struct ghost_heap *heap, struct chunk *chunk)
{
	struct link **bin = get_bin(heap, chunk);

	struct link *after = chunk_ll_pop(&chunk->payload.link);
	if(heap->unsorted_bin == &chunk->payload.link) {
		heap->unsorted_bin = after;
	}
	else if(*bin == &chunk->payload.link) {
		*bin = after;
	}
}
/*****************************************************************************/
static bool should_split(struct chunk *chunk, size_t desired)
{
	assert(chunk_read_size(chunk) >= desired);
	return (chunk_read_size(chunk) - desired) > CHUNK_SPLIT_THRESHOLD;
}
/*****************************************************************************/
static void bin_append(struct link **bin, struct link *new)
{
	if(*bin == NULL) {
		*bin = new;
		new->fwd = new;
		new->bck = new;
	} else {
		chunk_ll_append(*bin, new);
	}
}
/*****************************************************************************/
static void split_chunk(
	struct ghost_heap *heap, struct chunk *chunk, size_t desired
) {
	size_t s1 = DIV_ROUND_UP(
		desired, MIN_CHUNK_DATA_SIZE
	) * MIN_CHUNK_DATA_SIZE;
	size_t s2 = chunk_read_size(chunk) - s1;
	size_t top_flag = chunk_read_flag(chunk, TOP_CHUNK) ? TOP_CHUNK : 0;
	union {
		uint8_t *ptr;
		struct chunk *new;
	} u;

	u.ptr = chunk->payload.bytes + s1;

	chunk_set_size(chunk, s1);
	chunk_clear_flags(chunk, TOP_CHUNK);

	chunk_set_size(u.new, s2 - CHUNK_OVERHEAD_SIZE);
	/* this chunk is free so we also write the size at end of data */
	chunk_clear_flags(u.new, ALL_FLAGS);
	chunk_set_footer_size(u.new);
	chunk_set_flags(u.new, top_flag | PREV_IN_USE);

	if(top_flag) {
		heap->top_chunk = u.new;
		heap->top_flags &= ~PREV_IN_USE;
	} else {
		struct chunk *next = chunk_next_after(u.new);
		assert(next != NULL);
		chunk_clear_flags(next, PREV_IN_USE);
	}

	bin_append(&heap->unsorted_bin, &u.new->payload.link);
}
/*****************************************************************************/
static void insert_small(struct ghost_heap *heap, struct chunk *chunk)
{
	int index = small_bin_index(chunk_read_size(chunk));

	bin_append(&heap->small_bins[index], &chunk->payload.link);
}
/*****************************************************************************/
static void insert_large(struct ghost_heap *heap, struct chunk *chunk)
{
	size_t chunk_size = chunk_read_size(chunk);
	int index = large_bin_index(chunk_size);
	struct link *list = heap->large_bins[index];

	if(list == NULL) {
		heap->large_bins[index] = &chunk->payload.link;
		chunk->payload.link.fwd = &chunk->payload.link;
		chunk->payload.link.bck = &chunk->payload.link;
		return;
	}

	struct chunk *c0 = chunk_ll_chunk_ptr(list);
	struct chunk *c = c0;

	do {
		if(chunk_size >= chunk_read_size(c)) {
			chunk_ll_insert_before(
				&c->payload.link,
				&chunk->payload.link
			);
			if(c == c0) {
				heap->large_bins[index] = &chunk->payload.link;
			}
			return;
		}
		c = chunk_ll_chunk_ptr(c->payload.link.fwd);
	} while(c != c0);

	chunk_ll_insert_before(&c0->payload.link, &chunk->payload.link);
}
/*****************************************************************************/
static int extend_mmaped_chunk(struct chunk *chunk, size_t desired_size)
{
	size_t chunk_size = chunk_read_size(chunk);
	size_t map_size = min_to_map(desired_size - chunk_size);
	uint8_t *chunk_end = ((uint8_t*)(chunk->payload.data)) + chunk_size;

	void *new_mem = NULL;

	assert(desired_size > chunk_size);
	assert(chunk_read_flag(chunk, MMAPED_CHUNK | TOP_CHUNK));

	new_mem = safe_mmap(
		chunk_end,
		map_size,
		PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE,
		-1,
		0
	);

	if(new_mem == MAP_FAILED) {
		return 1;
	} else if(new_mem != chunk_end) {
		safe_munmap(new_mem, map_size);
		return 1;
	} else {
		chunk_set_size(chunk, chunk_size + map_size);
		return 0;
	}
}
/*****************************************************************************/
static int shrink_mmaped_chunk(struct chunk *chunk, size_t desired_size)
{
	size_t size = chunk_read_size(chunk) + CHUNK_OVERHEAD_SIZE;
	size_t new_size = align_up_unsigned(
		desired_size + CHUNK_OVERHEAD_SIZE, page_size
	);

	assert(new_size <= size);

	if(new_size >= size) {
		return 0;
	}

	union {
		uint64_t addr;
		uint8_t *mem;
		struct chunk *chunk;
	} u;

	u.chunk = chunk;

	assert((u.addr % page_size) == 0);

	uint8_t *end_of_chunk = u.mem + new_size;
	size_t size_to_free = size - new_size;

	assert((size_to_free % page_size) == 0);

	int r = safe_munmap(
		end_of_chunk, size_to_free
	);

	/* There is no valid reason for munmap to fail */
	assert(r == 0);
	if(r == 0) {
		chunk_set_size(chunk, new_size - CHUNK_OVERHEAD_SIZE);
	}

	return r;
}
/*****************************************************************************/
static void merge_chunks(struct ghost_heap *heap)
{
	struct chunk *c = chunk_ll_next_chunk(heap->unsorted_bin, NULL);

	while(c != NULL) {
		struct chunk *prev = chunk_prev_before(c);
		size_t this_size = chunk_read_size(c);

		if(prev == NULL) {
			c = chunk_ll_next_chunk(heap->unsorted_bin, c);
			continue;
		}

		struct link *next = chunk_ll_pop(&c->payload.link);
		if(heap->unsorted_bin == &c->payload.link) {
			heap->unsorted_bin = next;
		}
		int is_top = chunk_read_flag(c, TOP_CHUNK);

		pop_from_ll_and_bin(heap, prev);
		bin_append(&heap->unsorted_bin, &prev->payload.link);
		chunk_set_size(
			prev,
			chunk_read_size(prev) + this_size + CHUNK_OVERHEAD_SIZE
		);
		chunk_set_footer_size(prev);

		if(is_top) {
			heap->top_chunk = prev;
			chunk_set_flags(prev, TOP_CHUNK);
		}

		if(next != NULL) {
			c = chunk_ll_chunk_ptr(next);
		} else {
			c = NULL;
		}
	}
}
/*****************************************************************************/
static void sort_chunks(struct ghost_heap * heap)
{
	if(heap->unsorted_bin == NULL) {
		return;
	}

	struct chunk *c = chunk_ll_chunk_ptr(heap->unsorted_bin);
	do {
		struct chunk *next = chunk_ll_pop_chunk(&c->payload.link);

		if(is_chunk_small(c)) {
			insert_small(heap, c);
		} else {
			insert_large(heap, c);
		}

		c = next;
	} while(c != NULL);

	heap->unsorted_bin = NULL;
}
/*****************************************************************************/
static void heap_maintenance(struct ghost_heap *heap)
{
	merge_chunks(heap);
	sort_chunks(heap);
}
/*****************************************************************************/
static struct chunk *alloc_on_top(struct ghost_heap * heap, size_t size)
{
	struct chunk *top = heap->top_chunk;
	size_t extra_size = page_size + min_to_map(size);

	uint8_t *end_of_heap = chunk_mem_after(top);

	assert((extra_size % page_size) == 0);

	void *new_mem = safe_mmap(
		end_of_heap,
		extra_size,
		PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE,
		-1,
		0
	);

	if(new_mem == MAP_FAILED) {
		return NULL;
	}
	assert(new_mem == end_of_heap);

	struct chunk *new = new_mem;

	chunk_clear_flags(top, TOP_CHUNK);

	size_t top_in_use = heap->top_flags & PREV_IN_USE;
	chunk_set_flags(new, top_in_use | TOP_CHUNK);
	chunk_set_size(new, extra_size - CHUNK_OVERHEAD_SIZE);

	heap->top_flags |= PREV_IN_USE;

	return new;
}
/*****************************************************************************/
static void *pure_mmap_alloc(struct ghost_heap *heap, size_t size)
{
	size_t real_size = min_to_map(size + CHUNK_OVERHEAD_SIZE);
	struct chunk *chunk = safe_mmap(
		NULL,
		real_size,
		PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS,
		-1,
		0
	);

	if(chunk == MAP_FAILED) {
		return NULL;
	}

	chunk_set_flags(chunk, PREV_IN_USE | MMAPED_CHUNK);
	chunk_set_size(chunk, real_size - CHUNK_OVERHEAD_SIZE);

	return &chunk->payload;
}
/*****************************************************************************/
static void *normal_malloc_alloc(struct ghost_heap *heap, size_t size)
{
	struct chunk *chunk = bin_pop(heap, size);

	if(chunk == NULL) {
		chunk = alloc_on_top(heap, size);
	}

	if (should_split(chunk, size)) {
		split_chunk(heap, chunk, size);
	}

	return &chunk->payload.data;
}
/*****************************************************************************/
static struct chunk *bin_pop(struct ghost_heap *heap, size_t size)
{
	struct link **bin_ptr;
	struct chunk *found = bin_search(heap, size, &bin_ptr);

	if(found != NULL) {
		struct chunk *next_seq_chunk = chunk_next_after(found);
		struct link *next_in_list = chunk_ll_pop(&found->payload.link);

		if(&found->payload.link == *bin_ptr) {
			*bin_ptr = next_in_list;
		}
		if(next_seq_chunk != NULL) {
			chunk_set_flags(next_seq_chunk, PREV_IN_USE);
		} else {
			heap->top_flags = PREV_IN_USE;
		}
	}

	return found;
}
/*****************************************************************************/
static struct chunk *bin_search(
	struct ghost_heap *heap, size_t size, struct link ***bin_ptr
) {
	struct link *list = heap->unsorted_bin;
	for(
		struct chunk *c = chunk_ll_next_chunk(list, NULL);
		c != NULL;
		c = chunk_ll_next_chunk(list, c)
	) {
		if(chunk_read_size(c) >= size) {
			*bin_ptr = &heap->unsorted_bin;
			return c;
		}
	}

	if(size <= SMALL_BINS_MAX_SIZE) {
		int index = small_bin_index(size);

		if(heap->small_bins[index] != NULL) {
			*bin_ptr = &heap->small_bins[index];
			return chunk_ll_chunk_ptr(heap->small_bins[index]);
		}
	}

	for(int i = large_bin_index(size); i < NUM_LARGE_BINS; i++) {
		struct link *bin = heap->large_bins[i];
		if(bin == NULL) {
			continue;
		}
		struct chunk *c = chunk_ll_chunk_ptr(bin);
		assert(
			chunk_read_size(c) >=
			chunk_read_size(chunk_ll_chunk_ptr(bin->fwd))
      		);

		if(chunk_read_size(c) >= size) {
			*bin_ptr = &heap->large_bins[i];
			return c;
		}
	}

	return NULL;
}
/******************************************************************************
*                            FUNCTION DEFINITIONS                             *
******************************************************************************/
void *ghost_malloc(struct ghost_heap *heap, size_t size)
{
	void *ret = NULL;
	size_t min_for_mmap = page_size * MIN_PAGES_FOR_MALLOC_ALLOC;

	if(size >= min_for_mmap) {
		ret = pure_mmap_alloc(heap, size);
	} else {
		ret = normal_malloc_alloc(heap, size);
	}

	heap_maintenance(heap);
	return ret;
}
/*****************************************************************************/
void ghost_free(struct ghost_heap *heap, void *ptr)
{
	struct chunk *chunk;

	if(ptr == NULL) {
		return;
	}

	chunk = chunk_mem_ptr(ptr);

	if(chunk_read_flag(chunk, MMAPED_CHUNK)) {
		safe_munmap(
			chunk, chunk_read_size(chunk) + CHUNK_OVERHEAD_SIZE
		);
	} else {
		struct chunk *next = chunk_next_after(chunk);

		bin_append(&heap->unsorted_bin, &chunk->payload.link);

		if(next != NULL) {
			chunk_clear_flags(next, PREV_IN_USE);
		} else {
			heap->top_flags &= ~PREV_IN_USE;
		}
		chunk_set_footer_size(chunk);
	}
}
/*****************************************************************************/
void *ghost_realloc(struct ghost_heap *heap, void *ptr, size_t size)
{
	if(ptr == NULL) {
		return ghost_malloc(heap, size);
	}

	struct chunk *chunk = chunk_mem_ptr(ptr);
	size_t real_chunk_size = chunk_read_size(chunk);

	bool is_mmaped = chunk_read_flag(chunk, MMAPED_CHUNK);

	if(is_mmaped && size > real_chunk_size) {
		if(extend_mmaped_chunk(chunk, size) == 0) {
			return ptr;
		}
	} else if(is_mmaped && size < real_chunk_size) {
		shrink_mmaped_chunk(chunk, size);
		return ptr;
	}

	if(!is_mmaped && size <= real_chunk_size) {
		if(should_split(chunk, size)) {
			split_chunk(heap, chunk, size);
		}
		return ptr;
	}

	if(!is_mmaped) {
		struct chunk *next = chunk_next_after(chunk);
		while(next != NULL && is_chunk_free(heap, next)) {
			pop_from_ll_and_bin(heap, next);

			if(chunk_read_flag(next, TOP_CHUNK)) {
				heap->top_chunk = chunk;
				chunk_set_flags(chunk, TOP_CHUNK);
			}
			real_chunk_size +=
				chunk_read_size(next) +
				CHUNK_OVERHEAD_SIZE;
			chunk_set_size(chunk, real_chunk_size);

			next = chunk_next_after(chunk);
			if(next == NULL) {
				heap->top_flags |= PREV_IN_USE;
			} else {
				chunk_set_flags(next, PREV_IN_USE);
			}
		}
		if(real_chunk_size >= size) {
			if(should_split(chunk, size)) {
				split_chunk(heap, chunk, size);
			}
			return ptr;
		}
	}
	if(!is_mmaped && chunk_read_flag(chunk, TOP_CHUNK)) {
		int ret = extend_mmaped_chunk(
			chunk, size + page_size
		);
		if(ret == 0) {
			if(should_split(chunk, size)) {
				split_chunk(heap, chunk, size);
			}
			return ptr;
		}
	}

	void *new_alloc = ghost_malloc(heap, size);

	if(new_alloc != NULL) {
		memcpy(new_alloc, ptr, real_chunk_size);
		ghost_free(heap, ptr);
	}

	return new_alloc;
}
/*****************************************************************************/
void *ghost_malloc_check_leaks(struct ghost_heap *heap, void **ptr)
{
	struct chunk *c;

	if(ptr == NULL) {
		c = &heap->first_chunk;
	} else {
		c = chunk_next_after(chunk_mem_ptr(*ptr));
	}

	while(c != NULL) {
		struct chunk *next = chunk_next_after(c);

		if(!is_chunk_free(heap, c)) {
			return c->payload.data;
		}
		if(get_confirmed_bin(heap, c) == NULL) {
			return c->payload.data;
		}
		if(next == NULL && c != heap->top_chunk) {
			return c->payload.data;
		}
		c = next;
	};

	return NULL;
}
/*****************************************************************************/
int ghost_heap_destroy(struct ghost_heap *heap)
{
	size_t top_size = chunk_read_size(heap->top_chunk);
	uint8_t *end_of_heap = heap->top_chunk->payload.bytes + top_size;
	uint8_t *top_of_heap = (uint8_t*)heap;

	return safe_munmap(heap, end_of_heap - top_of_heap);
}
/*****************************************************************************/
struct ghost_heap *ghost_heap_init(void)
{
	struct ghost_heap *ret = NULL;

	size_t size_mapped;
	int prot;
	int flags;

	if(page_size == 0) {
		page_size = getpagesize();
		assert((page_size % sizeof(size_t)) == 0);
		assert((page_size % sizeof(void*)) == 0);
	}

	size_mapped = min_to_map(sizeof(*ret));

	assert((size_mapped % page_size) == 0);

	prot = PROT_READ | PROT_WRITE;
	flags =  MAP_PRIVATE | MAP_ANONYMOUS;

	void *space = gmalloc_maps_find_suitable_heap();
	assert(space != NULL);

	ret = safe_mmap(space, size_mapped, prot, flags, -1, 0);

	if(ret == MAP_FAILED) {
		goto exit;
	}

	chunk_set_flags(&ret->first_chunk, TOP_CHUNK | PREV_IN_USE);
	chunk_set_size(&ret->first_chunk, size_mapped - HEAP_OVERHEAD_SIZE);
	chunk_set_footer_size(&ret->first_chunk);

	assert(chunk_read_flag(&ret->first_chunk, TOP_CHUNK) != 0);

	struct link *first_link = &ret->first_chunk.payload.link;

	first_link->fwd = first_link;
	first_link->bck = first_link;

	ret->unsorted_bin = first_link;

	ret->top_flags = 0;
	ret->top_chunk = &ret->first_chunk;

	assert(
		chunk_mem_ptr(&ret->first_chunk.payload.link) ==
		&ret->first_chunk
	);

exit:
	return ret;
}
/*****************************************************************************/
