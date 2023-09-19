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
#include <circ_buffer.h>

#include <string.h>
/******************************************************************************
*                            FUNCTION DEFINITIONS                             *
******************************************************************************/
int circ_buffer_grow(
	struct circ_buffer *cb,
	size_t new_size,
	void *(*realloc_f)(void *, void*, size_t),
	void *realloc_arg
) {
	if(new_size < cb->buf_size) {
		return 0;
	}

	uint8_t *new_buf = realloc_f(realloc_arg, cb->buf, new_size);

	if(new_buf == NULL) {
		return -1;
	}

	size_t widx = cb->p - cb->buf;
	size_t growth = new_size - cb->buf_size;
	size_t wrap = widx - cb->used;

	cb->buf = new_buf;
	cb->p = new_buf + widx;

	if(widx > cb->used) {
		uint8_t *old_end = cb->buf + cb->buf_size;
		if(growth >= wrap) {
			memmove(old_end, cb->buf, wrap);
			cb->p = old_end + wrap;
		} else {
			size_t extra = wrap - growth;
			memmove(old_end, cb->buf, growth);
			memmove(cb->buf, cb->buf + growth, extra);
			cb->p = cb->buf + extra;
		}
	}

	cb->buf_size = new_size;

	return 0;
}
/*****************************************************************************/
int circ_buffer_init(struct circ_buffer *cb, uint8_t *space, size_t size)
{
	cb->buf_size = size;
	cb->buf = space;
	cb->p = space;
	cb->used = 0;

	return 0;
}
/*****************************************************************************/
