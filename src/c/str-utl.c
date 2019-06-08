/******************************************************************************
* Copyright (C) 2019  Billy Kozak                                             *
*                                                                             *
* This file is part of the py-trace program                                   *
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
#include "str-utl.h"

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
char *int_to_string(int i)
{
	size_t length = snprintf(NULL, 0, "%d", i);
	char *buffer = calloc(length + 1, sizeof(*buffer));

	if(buffer == NULL) {
		return NULL;
	}

	snprintf(buffer, length + 1, "%d", i);

	return buffer;
}
/*****************************************************************************/
char *concatenate_n_strings(size_t count, ...)
{
	va_list argp;

	char *ret = NULL;
	char *writeptr = NULL;
	size_t len = 1;

	char **strings = calloc(count, sizeof(*strings));
	if(strings == NULL) {
		goto exit;
	}

	va_start(argp, count);

	for(size_t i = 0; i < count; i++) {
		strings[i] = va_arg(argp, char*);
		len += strlen(strings[i]);
	}

	va_end(argp);

	ret = calloc(len, sizeof(*ret));
	if(ret == NULL) {
		goto exit;
	}
	writeptr = ret;

	for(size_t i = 0; i < count; i++) {
		size_t len = strlen(strings[i]);

		memcpy(writeptr, strings[i], len);
		writeptr += len;
	}

exit:
	free(strings);
	return ret;
}
/*****************************************************************************/