/******************************************************************************
* Copyright (C) 2023  Billy Kozak                                             *
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
#ifndef FILE_UTL_H
#define FILE_UTL_H
/******************************************************************************
*                                  INCLUDES                                   *
******************************************************************************/
#include <stdlib.h>
/******************************************************************************
*                                    TYPES                                    *
******************************************************************************/
struct file_utl_reader_state {
	char *data;
	size_t len;

	size_t buf_size;
	size_t buf_used;
	char *buf;
	int fd;
};
/******************************************************************************
*                                   DEFINES                                   *
******************************************************************************/
#define FILE_UTL_READER_EOF -1
#define FILE_UTL_ERR_IO_ERR -2
#define FILE_UTL_ERR_TOO_SMALL -3
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
/**
 * Initialize reader state
 *
 * @param fd - file descriptor to read from
 * @param line_buffer - buffer to store lines
 * @param buf_size - size of the line buffer
**/
void file_utl_reader_init(
	struct file_utl_reader_state *state,
	int fd,
	char *line_buffer,
	size_t buf_size
);
/**
 * Read lines from file without using stdio
 *
 * Lines can be retrieved from state->data and have a size of state->len.
 *
 * @param state - represents the state of the reader and also contains results.
 * 	Must have been initialized by file_util_reader_init.
 *
 * @return int - The number of bytes consumed, FILE_UTL_READER_EOF or an error
 * 	code.
**/
int file_utl_read_line(struct file_utl_reader_state *state);
/*****************************************************************************/
#endif /* FILE_UTL_H */
