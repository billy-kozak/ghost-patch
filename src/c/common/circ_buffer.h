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
#ifndef CIRC_BUFFER_H
#define CIRC_BUFFER_H
/******************************************************************************
*                                  INCLUDES                                   *
******************************************************************************/
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <utl/math-utl.h>
/******************************************************************************
*                                    TYPES                                    *
******************************************************************************/
struct circ_buffer {
	size_t used;
	size_t buf_size;
	uint8_t *buf;
	uint8_t *p;
};
/******************************************************************************
*                            FUNCTION DEFINITIONS                             *
******************************************************************************/
static inline uint8_t *circ_buffer_rptr(struct circ_buffer *cb)
{
	size_t widx = cb->p - cb->buf;
	size_t ridx = circ_sub_u64(widx, cb->used, cb->buf_size);

	return cb->buf + ridx;
}
/*****************************************************************************/
static inline uint8_t *circ_buffer_wptr(struct circ_buffer *cb)
{
	return cb->p;
}
/*****************************************************************************/
static inline size_t circ_buffer_used(struct circ_buffer *cb)
{
	return cb->used;
}
/*****************************************************************************/
static inline size_t circ_buffer_capacity(struct circ_buffer *cb)
{
	return cb->buf_size - cb->used;
}
/*****************************************************************************/
static inline size_t circ_buffer_contig_wsize(struct circ_buffer *cb)
{
	if(circ_buffer_capacity(cb) == 0) {
		return 0;
	}

	uint8_t *rptr = circ_buffer_rptr(cb);

	if(rptr > cb->p) {
		return rptr - cb->p;
	} else {
		return (cb->buf + cb->buf_size) - cb->p;
	}
}
/*****************************************************************************/
static inline size_t circ_buffer_contig_rsize(struct circ_buffer *cb)
{
	uint8_t *rptr = circ_buffer_rptr(cb);

	size_t ridx = rptr - cb->buf;

	if((ridx + cb->used) > cb->buf_size) {
		return cb->buf_size - ridx;
	} else {
		return cb->used;
	}
}
/*****************************************************************************/
static inline int circ_buffer_increment_used(
	struct circ_buffer *cb,
	size_t used
) {
	if(used > circ_buffer_capacity(cb)) {
		return -1;
	}

	size_t widx_new = circ_add_u64(cb->p - cb->buf, used, cb->buf_size);

	cb->p = cb->buf + widx_new;
	cb->used += used;

	return 0;
}
/*****************************************************************************/
static inline int circ_buffer_decrement_used(
	struct circ_buffer *cb,
	size_t used
) {
	if(used > cb->used) {
		return -1;
	}

	cb->used -= used;

	return 0;
}
/*****************************************************************************/
static inline size_t circ_buffer_write(
	struct circ_buffer *cb,
	void *restrict src,
	size_t size
) {
	if(circ_buffer_capacity(cb) == 0) {
		return 0;
	}

	size_t contig = circ_buffer_contig_wsize(cb);

	if(contig >= size) {
		memcpy(cb->p, src, size);
		cb->p += size;
		cb->used += size;

		return size;
	} else {
		memcpy(cb->p, src, contig);
		cb->used += contig;
		cb->p = cb->buf;
		return contig + circ_buffer_write(
			cb,
			src + contig,
			size - contig
		);
	}
}
/*****************************************************************************/
static inline int circ_buffer_prepend(
	struct circ_buffer *cb,
	char c
) {
	if(circ_buffer_capacity(cb) == 0) {
		return -1;
	}

	size_t ridx = circ_sub_u64(
		circ_buffer_rptr(cb) - cb->buf,
		1,
		cb->buf_size
	);

	cb->buf[ridx] = c;
	cb->used += 1;

	return 0;
}
/*****************************************************************************/
static inline size_t circ_buffer_read(
	struct circ_buffer *cb,
	void *restrict dest,
	size_t size
) {
	if(cb->used == 0) {
		return 0;
	}

	size_t contig = circ_buffer_contig_rsize(cb);
	uint8_t *rptr = circ_buffer_rptr(cb);

	if(contig >= size) {
		memcpy(dest, rptr, size);
		cb->used -= size;
		return size;
	} else {
		memcpy(dest, rptr, contig);
		cb->used -= contig;
		return contig + circ_buffer_read(
			cb, dest + contig, size - contig
		);
	}
}
/*****************************************************************************/
static inline int circ_buffer_pop(struct circ_buffer *cb)
{
	if(circ_buffer_used(cb) == 0) {
		return -1;
	}

	int c = circ_buffer_rptr(cb)[0];
	cb->used -= 1;

	return c;
}
/*****************************************************************************/
static inline int circ_buffer_get(struct circ_buffer *cb, int i)
{
	if(circ_buffer_used(cb) <= i) {
		return -1;
	}

	size_t ridx = circ_buffer_rptr(cb) - cb->p;
	return cb->buf[circ_add_u64(ridx, i, cb->buf_size)];
}
/*****************************************************************************/
static inline void circ_buffer_clear(struct circ_buffer *cb)
{
	cb->used = 0;
	cb->p = cb->buf;
}
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
int circ_buffer_grow(
	struct circ_buffer *cb,
	size_t new_size,
	void *(*realloc_f)(void *, void*, size_t),
	void *realloc_arg
);
int circ_buffer_init(struct circ_buffer *cb, uint8_t *space, size_t size);
/*****************************************************************************/
#endif /* CIRC_BUFFER_H */
