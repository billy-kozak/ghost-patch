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
#ifndef STR_UTL_H
#define STR_UTL_H
/******************************************************************************
*                                  INCLUDES                                   *
******************************************************************************/
#include <stdlib.h>
#include <stdbool.h>

#include "misc-macros.h"
/******************************************************************************
*                                    TYPES                                    *
******************************************************************************/
struct lstring {
	size_t len;
	char *str;
};
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
char *int_to_string(int i);
const char *bool_to_string(bool val);
int strdcmp(const char *s1, const char *s2, char delim);
int strdcpy(char *dst, const char *src, char delim, size_t size);
size_t strdlen(const char *s, char delim);
struct lstring str_utl_tok_and_sqz(
	const char *s,
	size_t len,
	char delim,
	const char **saveptr
);
int lstring_cmp(const struct lstring *ls, const char *s);
/*****************************************************************************/
#endif /* STR_UTL_H */
