/******************************************************************************
* Copyright (C) 2023 Billy Kozak                                              *
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
#include "gmalloc-chunk.h"

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
void chunk_set_size(struct chunk *chunk, size_t size)
{
	assert((size & ALL_FLAGS) == 0);
	chunk->flags = size | (chunk->flags & ALL_FLAGS);
}
/*****************************************************************************/
size_t chunk_read_size(const struct chunk *chunk)
{
	return chunk->flags & ~ALL_FLAGS;
}
/*****************************************************************************/
size_t chunk_total_size(const struct chunk *chunk)
{
	return chunk_read_size(chunk) + CHUNK_OVERHEAD_SIZE;
}
/*****************************************************************************/
void chunk_clear_flags(struct chunk *chunk, size_t flags)
{
	assert((flags & ~ALL_FLAGS) == 0);
	chunk->flags &= ~flags;
}
/*****************************************************************************/
void chunk_set_flags(struct chunk *chunk, size_t flags)
{
	assert((flags & ~ALL_FLAGS) == 0);
	chunk->flags |= flags;
}
/*****************************************************************************/
int chunk_read_flag(const struct chunk *chunk, size_t flag)
{
	return !!(chunk->flags & flag);
}
/*****************************************************************************/
void chunk_set_footer_size(struct chunk *chunk)
{
	size_t size = chunk_read_size(chunk);

	assert((size % sizeof(size)) == 0);

	chunk->payload.data[(size / sizeof(size)) - 1] = size;
}
/*****************************************************************************/
size_t chunk_read_prev_size(const struct chunk *chunk)
{
	union {
		struct chunk *chunk;
		size_t *words;
	} u;

	assert(chunk_read_flag(chunk, PREV_IN_USE) == 0);

	u.chunk = (struct chunk*)chunk;

	return u.words[-1];
}
/*****************************************************************************/
struct chunk *chunk_mem_ptr(const void *ptr)
{
	union {
		uint8_t *bytes;
		struct chunk *chunk;
	} addr;

	addr.bytes = ((uint8_t*)ptr) - CHUNK_OVERHEAD_SIZE;

	return addr.chunk;
}
/*****************************************************************************/
struct chunk *chunk_next_after(const struct chunk *chunk)
{
	if(chunk_read_flag(chunk, TOP_CHUNK)) {
		return NULL;
	} else {
		union {
			uint8_t *d;
			struct chunk *c;
		} u;
		uint8_t *payload_ptr = (uint8_t*)chunk->payload.data;

		u.d = payload_ptr + chunk_read_size(chunk);
		return u.c;
	}
}
/*****************************************************************************/
void* chunk_mem_after(const struct chunk *chunk)
{
	union {
		uint8_t *d;
		struct chunk *c;
		void *p;
	} u;
	uint8_t *payload_ptr = (uint8_t*)chunk->payload.data;

	u.d = payload_ptr + chunk_read_size(chunk);
	return u.p;
}
/*****************************************************************************/
struct chunk *chunk_prev_before(const struct chunk *chunk)
{
	if(chunk_read_flag(chunk, PREV_IN_USE)) {
		return NULL;
	} else {
		union {
			uint8_t *d;
			struct chunk *c;
		} u;

		u.c = (struct chunk *)chunk;

		u.d -= chunk_read_prev_size(chunk) + CHUNK_OVERHEAD_SIZE;
		return u.c;
	}
}
/*****************************************************************************/
