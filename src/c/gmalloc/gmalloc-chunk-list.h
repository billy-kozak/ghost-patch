/******************************************************************************
* Copyright (C) 2023 Billy Kozak                                             *
*                                                                             *
* This file is part of the gorilla-patch program                              *
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
#ifndef GMALLOC_CHUNK_LIST
#define GMALLOC_CHUNK_LIST
/******************************************************************************
*                                  INCLUDES                                   *
******************************************************************************/
#include "gmalloc-chunk-types.h"
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
void chunk_ll_insert_before(struct link *link, struct link *new);
void chunk_ll_insert_after(struct link *link, struct link *new);
void chunk_ll_append(struct link *list, struct link *new);
struct link* chunk_ll_pop(struct link *link);
struct chunk *chunk_ll_chunk_ptr(const struct link *ptr);
struct chunk *chunk_ll_next_chunk(struct link *list, struct chunk *last);
struct chunk *chunk_ll_pop_chunk(struct link *list);
struct chunk *chunk_ll_list_pop_chunk(struct link **list);
/*****************************************************************************/
#endif /* GMALLOC_CHUNK_LIST */
