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
#ifndef GMALLOC_CHUNK_TYPES_H
#define GMALLOC_CHUNK_TYPES_H
/******************************************************************************
*                                  INCLUDES                                   *
******************************************************************************/
#include <utl/math-utl.h>

#include <stdlib.h>
#include <stdint.h>
/******************************************************************************
*                                   DEFINES                                   *
******************************************************************************/
/* min size is size of the payload union, plus a sizeof(size_t) so we can
 * write the footer size */
#define MIN_CHUNK_DATA_SIZE ((\
	sizeof(size_t) > sizeof(struct link) ? \
	sizeof(size_t) : \
	sizeof(struct link) \
) + sizeof(size_t))

#define MIN_CHUNK_DATA_WORDS DIV_ROUND_UP(MIN_CHUNK_DATA_SIZE, sizeof(size_t))
/******************************************************************************
*                                    TYPES                                    *
******************************************************************************/
struct link {
	struct link *fwd;
	struct link *bck;
};

struct chunk {
	size_t flags;

	union {
		struct link link;
		size_t data[MIN_CHUNK_DATA_WORDS];
		uint8_t bytes[MIN_CHUNK_DATA_WORDS * sizeof(size_t)];
	} payload;
};

/*****************************************************************************/
#endif /* GMALLOC_CHUNK_TYPES_H */
