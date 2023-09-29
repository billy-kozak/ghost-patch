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
#include "trace-print-tools.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
/******************************************************************************
*                                   DEFINES                                   *
******************************************************************************/
#define CHAR_ARR_STRLEN(s) (sizeof(s) - 1)
/******************************************************************************
*                              STATIC FUNCTIONS                               *
******************************************************************************/
static char octal_char(int val, int n)
{
	return ((val >> (3 * n)) & 0x7) + '0';
}
/*****************************************************************************/
static int repr_byte(char *str, char byte, ssize_t *space_size)
{
	if((byte == '"') || (byte == '\\')) {
		if(*space_size < 2) {
			return 0;
		} else {
			str[0] = '\\';
			str[1] = byte;
			*space_size -= 2;
			return 2;
		}
	} else if(byte == '\n') {
		if(*space_size < 2) {
			return 0;
		} else {
			str[0] = '\\';
			str[1] = 'n';
			*space_size -= 2;
			return 2;
		}
	} else if(isprint(byte) || (byte == '\t')) {
		if(*space_size == 0) {
			return 0;
		} else {
			str[0] = byte;
			*space_size -= 1;
			return 1;
		}
	} else {
		if(*space_size < 4) {
			return 0;
		} else {
			str[0] = '\\';
			str[1] = octal_char(byte, 2);
			str[2] = octal_char(byte, 1);
			str[3] = octal_char(byte, 0);
			*space_size -= 4;
			return 4;
		}
	}
}
/******************************************************************************
*                            FUNCTION DEFINITIONS                             *
******************************************************************************/
char *sprint_buffer(
	const char *buffer,
	char *str,
	ssize_t buffer_size,
	ssize_t space_size
) {
	int len = 0;
	char border = '"';
	const char continuation[] = "\"...";

	if(buffer_size < 0) {
		return NULL;
	}
	if(buffer == NULL) {
		strncpy(str, "<null>", space_size);
		return str;
	}

	space_size -= sizeof(border) + CHAR_ARR_STRLEN(continuation) + 1;

	if(space_size < 0) {
		return NULL;
	}

	str[len] = border;
	len += 1;

	for(size_t i = 0; i < buffer_size; i++) {
		int s = 0;
		char c = buffer[i];

		if((s = repr_byte(str + len, c, &space_size)) == 0) {
			memcpy(str + len, continuation, sizeof(continuation));
			return str;
		}
		len += s;
	}

	str[len] = border;
	str[len + 1] = '\0';

	return str;
}
/*****************************************************************************/
