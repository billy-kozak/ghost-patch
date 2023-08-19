/******************************************************************************
* Copyright (C) 2019  Billy Kozak                                             *
*                                                                             *
* This file is part of the ghost-patch program                                   *
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
#include "path-utl.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/******************************************************************************
*                                  CONSTANTS                                  *
******************************************************************************/
static const size_t DEFAULT_BUFFER_SIZE = 128;
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
char *safe_resolve_symlink(const char *path)
{
	ssize_t buf_size = DEFAULT_BUFFER_SIZE;
	char *buf = calloc(buf_size, sizeof(*buf));
	size_t readlink_ret = 0;

	if(buf == NULL) {
		goto fail;
	}

	while((readlink_ret = readlink(path, buf, buf_size)) == buf_size) {
		char *temp = realloc(buf, buf_size * 2);
		if(temp == NULL) {
			goto fail;
		}
		buf_size *= 2;
	}

	if(readlink_ret == -1) {
		goto fail;
	}

	buf[readlink_ret] = '\0';
	return buf;
fail:
	free(buf);
	return NULL;
}
/*****************************************************************************/
const char *basename(const char *path)
{
	size_t size = strlen(path);

	for(size_t i = size - 1; i >= 0; i--) {
		if(path[i] == '/') {
			return path + i + 1;
		}
	}
	return path;
}
/*****************************************************************************/