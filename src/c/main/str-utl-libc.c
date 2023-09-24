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
#include "str-utl-libc.h"

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
/******************************************************************************
*                            FUNCTION DEFINITIONS                             *
******************************************************************************/
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
char *append_n_to_dyn_str(size_t count, size_t *lenptr, char *dst, ...)
{
	if(count == 0) {
		return dst;
	}

	const char *strings[count];
	size_t lens[count];

	size_t len_dst = strlen(dst);
	size_t len_total = len_dst;
	va_list argp;

	va_start(argp, dst);

	for(int i = 0; i < count; i++) {
		strings[i] = va_arg(argp, const char*);
		lens[i] = strlen(strings[i]);

		len_total += lens[i];
	}
	len_total += 1;

	va_end(argp);

	char *new_dst = realloc(dst, len_total);
	if(new_dst == NULL) {
		return NULL;
	}

	char *wptr = new_dst + len_dst;

	for(int i = 0; i < count; i++) {
		memcpy(wptr, strings[i], lens[i]);
		wptr += lens[i];
	}
	*wptr = '\0';

	if(lenptr != NULL) {
		*lenptr = len_total;
	}

	return new_dst;
}

/*****************************************************************************/
