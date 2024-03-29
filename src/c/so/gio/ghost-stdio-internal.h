/**************************************
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
#ifndef GHOST_STDIO_INTERNAL_H
#define GHOST_STDIO_INTERNAL_H
/******************************************************************************
*                                  INCLUDES                                   *
******************************************************************************/
#include <stdlib.h>

#include <circ_buffer.h>
/******************************************************************************
*                                   DEFINES                                   *
******************************************************************************/
#define GIO_FLAG_BUF   (1 << 1)
#define GIO_FLAG_LF    (1 << 2)
#define GIO_FLAG_SBUF  (1 << 3)
#define GIO_FLAG_READ  (1 << 4)
#define GIO_FLAG_WRITE (1 << 5)
#define GIO_FLAG_OPEN  (1 << 6)

#define GIO_ERR_EOF      (1 << 1)
#define GIO_ERR_BUFSIZ   (1 << 2)
#define GIO_ERR_IOERR    (1 << 3)
#define GIO_ERR_BAD_MODE (1 << 4)
/******************************************************************************
*                                    TYPES                                    *
******************************************************************************/
struct ghost_file {
	int fd;
	int flags;
	int err;

	struct circ_buffer wb;
	struct circ_buffer rb;

	char sys_buffer[];
};

struct fmode {
	int flags;
	mode_t mode;
};
/*****************************************************************************/
#endif /* GHOST_STDIO_INTERNAL_H */
