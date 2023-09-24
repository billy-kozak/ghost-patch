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
#ifndef STR_UTL_LIBC_H
#define STR_UTL_LIBC_H
/******************************************************************************
*                                  INCLUDES                                   *
******************************************************************************/
#include <misc-macros.h>

#include <stdlib.h>
/******************************************************************************
*                                   MACROS                                    *
******************************************************************************/
#define concatenate_strings(...) \
	concatenate_n_strings(NUM_ARGS(const char*, __VA_ARGS__), __VA_ARGS__)
#define copy_string(s) concatenate_n_strings(1, s);
#define append_to_dyn_str(lenptr, dst, ...) \
	append_n_to_dyn_str( \
		NUM_ARGS(const char *, __VA_ARGS__), lenptr, dst, __VA_ARGS__ \
	)
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
char *concatenate_n_strings(size_t count, ...);
char *append_n_to_dyn_str(size_t count, size_t *lenptr, char *dst, ...);
/*****************************************************************************/
#endif /* STR_UTL_LIBC_H */
