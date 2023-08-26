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
#ifndef GMALLOC_CHUNK_H
#define GMALLOC_CHUNK_H
/******************************************************************************
*                                  INCLUDES                                   *
******************************************************************************/
#include "gmalloc-chunk-types.h"

#include <stdbool.h>
/******************************************************************************
*                                   DEFINES                                   *
******************************************************************************/
#define _CHUNK_FLAGS_WIDTH (sizeof(((struct chunk*)(NULL))->flags) * 8)

#define PREV_IN_USE (1UL << (_CHUNK_FLAGS_WIDTH - 1))
#define MMAPED_CHUNK (1UL << (_CHUNK_FLAGS_WIDTH - 2))
#define TOP_CHUNK (1UL << (_CHUNK_FLAGS_WIDTH - 3))

#define ALL_FLAGS (PREV_IN_USE | MMAPED_CHUNK | TOP_CHUNK)

#define CHUNK_MAX_SIZE (SIZE_MAX &~ ALL_FLAGS)

#define CHUNK_OVERHEAD_SIZE ( \
	sizeof(struct chunk) - sizeof(((struct chunk*)(NULL))->payload) \
)
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
void chunk_set_size(struct chunk *chunk, size_t size);
size_t chunk_read_size(const struct chunk *chunk);
size_t chunk_total_size(const struct chunk *chunk);
void chunk_clear_flags(struct chunk *chunk, size_t flags);
int chunk_read_flag(const struct chunk *chunk, size_t flag);
void chunk_set_flags(struct chunk *chunk, size_t flags);
void chunk_set_footer_size(struct chunk *chunk);
size_t chunk_read_prev_size(const struct chunk *chunk);
struct chunk *chunk_mem_ptr(const void *ptr);
struct chunk *chunk_next_after(const struct chunk *chunk);
struct chunk *chunk_prev_before(const struct chunk *chunk);
void* chunk_mem_after(const struct chunk *chunk);
/*****************************************************************************/
#endif /* GMALLOC_CHUNK_H */
