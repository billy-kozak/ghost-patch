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
/******************************************************************************
*                                  INCLUDES                                   *
******************************************************************************/
#include "str-utl.h"

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
const char *bool_to_string(bool val)
{
	return val ? "true" : "false";
}
/*****************************************************************************/
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
int strdcmp(const char *s1, const char *s2, char delim)
{
	for(size_t i = 0; true; i++) {
		char c1 = s1[i] == '\0' ? delim : s1[i];
		char c2 = s2[i] == '\0' ? delim : s2[i];

		if((c1 == delim) || (c2 == delim)) {
			return c1 - c2;
		} else if(c1 != c2) {
			return c1 - c2;
		}
	}

	return -1;
}
/*****************************************************************************/
int strdcpy(char *dst, const char *src, char delim, size_t size)
{
	size_t i;
	for(i = 0; i < (size - 1); i++) {
		if(src[i] == delim || src[i] == '\0') {
			break;
		}
		dst[i] = src[i];
	}

	dst[i] = '\0';
	return i;
}
/*****************************************************************************/
size_t strdlen(const char *s, char delim)
{
	size_t i;
	for(i = 0; s[i] != '\0'; i++) {
		if(s[i] == delim) {
			return i;
		}
	}
	return i;
}
/*****************************************************************************/
struct lstring str_utl_tok_and_sqz(
	const char *s,
	size_t len,
	char delim,
	const char **saveptr
) {
	struct lstring ret = {};

	if(*saveptr == NULL) {
		*saveptr = s;
	}
	int idx = (*saveptr - s);

	for(; idx < len; idx++) {
		if(s[idx] != delim) {
			break;
		}
	}
	ret.str = (char*)(s + idx);

	for(; idx < len; idx++) {
		if(s[idx] == delim) {
			break;
		}
		ret.len += 1;
	}

	*saveptr = s + idx;

	return ret;
}
/*****************************************************************************/
int lstring_cmp(const struct lstring *ls, const char *s)
{
	for(int i = 0; i < ls->len; i++) {
		if(s[i] == '\0') {
			if(i == (ls->len - 1)) {
				return ls->str[i];
			} else {
				return -1;
			}
		} else if(s[i] != ls->str[i]) {
			return ls->str[i] - s[i];
		}
	}
	return 0;
}
/*****************************************************************************/
