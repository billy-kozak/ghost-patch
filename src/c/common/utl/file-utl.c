/******************************************************************************
* Copyright (C) 2023  Billy Kozak                                             *
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
#include "file-utl.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
/******************************************************************************
*                              STATIC FUNCTIONS                               *
******************************************************************************/
static char* find_eol(char *str, size_t len)
{
	for(int i = 0; i < len; i++) {
		if(str[i] == '\n') {
			return str + i;
		}
	}
	return NULL;
}
/******************************************************************************
*                            FUNCTION DEFINITIONS                             *
******************************************************************************/
void file_utl_reader_init(
	struct file_utl_reader_state *state,
	int fd,
	char *line_buffer,
	size_t buf_size
) {
	state->fd = fd;
	state->buf = line_buffer;
	state->buf_size = buf_size;
	state->len = 0;
	state->buf_used = 0;
	state->data = line_buffer;
}
/*****************************************************************************/
int file_utl_read_line(struct file_utl_reader_state *state)
{
	char *start_of_used = state->data + state->len;
	size_t len_of_used = state->buf_used - (start_of_used - state->buf);

	assert(len_of_used >= 0);

	char *eol = find_eol(start_of_used, len_of_used);

	if(eol == NULL) {
		if(state->buf_used != 0) {
			memmove(state->buf, state->data, len_of_used);
		}
		char *start_of_unused = state->buf + len_of_used;
		size_t remaining = state->buf_size - len_of_used;

		assert(remaining >= 0);

		if(remaining == 0) {
			state->buf_used = len_of_used;
			state->len = len_of_used;
			state->data = state->buf;
			return FILE_UTL_ERR_TOO_SMALL;
		}

		ssize_t ret = read(state->fd, start_of_unused, remaining);

		if( ret < 0 ) {
			return FILE_UTL_ERR_IO_ERR;
		}

		state->buf_used = len_of_used + ret;
		state->data = state->buf;

		if(ret == 0 && len_of_used == 0) {
			return FILE_UTL_READER_EOF;
		} else if(ret == 0 && len_of_used != 0) {
			state->len = len_of_used;
			return len_of_used;
		} else {
			eol = find_eol(state->data, state->buf_used);
			if(eol == NULL) {
				state->len = state->buf_used;
				return FILE_UTL_ERR_TOO_SMALL;
			}
		}
	} else {
		state->data = start_of_used;
	}

	state->len = eol + 1 - state->data;
	assert(state->len != 0);

	return state->len;
}
/*****************************************************************************/
