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
/******************************************************************************
*                                    TYPES                                    *
******************************************************************************/
enum buffer_type {
	BUFFER_DEFAULT,
	BUFFER_NL,
	BUFFER_NONE
};

struct ghost_file {
	int fd;
	enum buffer_type buf_type;
	size_t buf_size;
	char *wptr;
	char buffer[];
};

struct fmode {
	int flags;
	mode_t mode;
};
/*****************************************************************************/
#endif /* GHOST_STDIO_INTERNAL_H */
