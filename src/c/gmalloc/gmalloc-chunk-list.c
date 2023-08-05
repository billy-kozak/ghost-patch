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
/******************************************************************************
*                                  INCLUDES                                   *
******************************************************************************/
#include "gmalloc-chunk-list.h"
#include "gmalloc-chunk.h"

#include <assert.h>
/******************************************************************************
*                            FUNCTION DEFINITIONS                             *
******************************************************************************/
void chunk_ll_insert_after(struct link *link, struct link *new)
{
	if(link->fwd == NULL) {
		link->fwd = new;
		link->bck = new;

		new->fwd = link;
		new->bck = link;
	} else {
		new->fwd = link->fwd;
		new->bck = link;

		link->fwd->bck = new;
		link->fwd = new;
	}
}
/*****************************************************************************/
void chunk_ll_insert_before(struct link *link, struct link *new)
{
	if(link->fwd == NULL) {
		link->fwd = new;
		link->bck = new;

		new->fwd = link;
		new->bck = link;
	} else {
		new->bck = link->bck;
		new->fwd = link;

		link->bck->fwd = new;
		link->bck = new;
	}
}
/*****************************************************************************/
void chunk_ll_append(struct link *list, struct link *new)
{
	struct link *end = list->bck;

	if(end == NULL) {
		list->fwd = new;
		list->bck = new;

		new->fwd = list;
		new->bck = list;
	} else {
		assert(end->fwd == list);

		end->fwd = new;

		new->bck = end;
		new->fwd = list;

		list->bck = new;
	}
}
/*****************************************************************************/
struct link* chunk_ll_pop(struct link *link)
{
	struct link *last = link->bck;
	struct link *next = link->fwd;

	if(link->fwd == NULL) {
		return NULL;
	}

	assert(last->fwd == link);
	assert(next->bck == link);

	last->fwd = next;
	next->bck = last;

	if(next == link) {
		return NULL;
	} else {
		return next;
	}
}
/*****************************************************************************/
struct chunk *chunk_ll_chunk_ptr(const struct link *ptr)
{
	return chunk_mem_ptr(ptr);
}
/*****************************************************************************/
struct chunk *chunk_ll_next_chunk(struct link *list, struct chunk *last)
{
	if(list == NULL) {
		return NULL;
	}
	else if(list->fwd == NULL) {
		return NULL;
	} else if(last == NULL) {
		return chunk_ll_chunk_ptr(list);
	} else if(last->payload.link.fwd == list) {
		return NULL;
	} else {
		return chunk_ll_chunk_ptr(last->payload.link.fwd);
	}
}
/*****************************************************************************/
struct chunk *chunk_ll_pop_chunk(struct link *node)
{
	struct link *next = chunk_ll_pop(node);

	if(next == NULL) {
		return NULL;
	} else {
		return chunk_ll_chunk_ptr(next);
	}
}
/*****************************************************************************/
struct chunk *chunk_ll_list_pop_chunk(struct link **list)
{
	struct chunk *next = chunk_ll_pop_chunk(*list);

	*list = &next->payload.link;

	return next;
}
/*****************************************************************************/
