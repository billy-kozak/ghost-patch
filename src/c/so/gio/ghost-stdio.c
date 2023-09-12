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
#include "ghost-stdio.h"
#include "ghost-stdio-internal.h"

#include <secret-heap.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <assert.h>
/******************************************************************************
*                                    DATA                                     *
******************************************************************************/
struct ghost_file *ghost_stdin;
struct ghost_file *ghost_stdout;
struct ghost_file *ghost_stderr;
/******************************************************************************
*                                  CONSTANTS                                  *
******************************************************************************/
static const size_t BUF_SIZE_INIT = 1024;
/******************************************************************************
*                              STATIC FUNCTIONS                               *
******************************************************************************/
static int interp_mode(const char *restrict mode, struct fmode *p)
{
	size_t len = 0;
	while(mode[len] != '\0') {
		if(len > 3) {
			return -1;
		}
		len += 1;
	}

	char ch0 = mode[0];
	char ch1 = len >= 2 ? mode[1] : '\0';
	char ch2 = len >= 3 ? mode[2] : '\0';

	if((ch1 == 'b') && (ch2 == 'b')) {
		return -1;
	}
	if(ch1 != 'b' && ch1 != '+' && ch1 != '\0') {
		return -1;
	}
	if(ch2 != 'b' && ch2 != '+' && ch2 != '\0') {
		return -1;
	}

	if(ch0 == 'r') {
		if(ch1 == '+' || ch2 == '+') {
			p->flags = O_RDWR;
			p->mode = 0;
		} else {
			p->flags = O_RDONLY;
			p->mode = 0;
		}
	} else if(ch0 == 'w') {
		if(ch1 == '+' || ch2 == '+') {
			p->flags = O_RDWR;
			p->mode = O_CREAT;
		} else {
			p->flags = O_WRONLY;
			p->mode = O_CREAT;
		}
	} else if(ch0 == 'a') {
		if(ch1 == '+' || ch2 == '+') {
			p->flags = O_RDWR;
			p->mode = O_CREAT | O_APPEND;
		} else {
			p->flags = O_WRONLY;
			p->mode = O_CREAT | O_APPEND;
		}
	} else {
		return -1;
	}

	return 0;
}
/******************************************************************************
*                            FUNCTION DEFINITIONS                             *
******************************************************************************/
struct ghost_file *ghost_fopen(
	const char *restrict pathname,
	const char *restrict mode
) {
	struct fmode fmode;

	if(interp_mode(mode, &fmode) != 0) {
		return NULL;
	}

	int fd = open(pathname, fmode.flags, fmode.mode);

	if(fd < 0) {
		return NULL;
	}

	return ghost_fdopen(fd, mode);
}
/*****************************************************************************/
struct ghost_file *ghost_fdopen(int fd, const char *restrict mode)
{
	struct ghost_file *file = ghost_malloc(
		sheap, sizeof(*file) + BUF_SIZE_INIT
	);
	if(file == NULL) {
		return file;
	}

	file->fd = fd;
	file->wptr = file->buffer;
	file->buf_size = BUF_SIZE_INIT;

	if(isatty(fd)) {
		file->buf_type = BUFFER_NL;
	} else {
		file->buf_type = BUFFER_DEFAULT;
	}

	return file;
}
/*****************************************************************************/
int ghost_fclose(struct ghost_file *file)
{
	if(file == NULL) {
		return 0;
	}

	ghost_fflush(file);

	int ret = close(file->fd);

	ghost_free(sheap, file);
	return ret;
}
/*****************************************************************************/
int ghost_fflush(struct ghost_file *file)
{
	size_t c = file->wptr - file->buffer;
	if(c == 0) {
		return 0;
	}

	int r = write(file->fd, file->buffer, c);

	if(r < 0) {
		return -1;
	}

	size_t remain = c - r;
	if(remain != 0) {
		memmove(file->buffer, file->buffer + r, remain);
	}

	file->wptr -= r;
	assert(file->wptr >= file->buffer);

	return 0;
}
/*****************************************************************************/
void ghost_stdio_cleanup(void)
{
	ghost_fflush(ghost_stdout);
	ghost_fflush(ghost_stderr);
}
/*****************************************************************************/
void ghost_stdio_init(void)
{
	ghost_stdin = ghost_fdopen(0, "r");
	ghost_stdout = ghost_fdopen(1, "w");
	ghost_stderr = ghost_fdopen(2, "w");
}
/*****************************************************************************/
