/******************************************************************************
* Copyright (C) 2019  Billy Kozak                                             *
*                                                                             *
* This file is part of the gorilla-patch program                              *
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
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
char *concatenate_n_strings(size_t count, ...);
char *int_to_string(int i);
const char *bool_to_string(bool val);
int strdcmp(const char *s1, const char *s2, char delim);
/******************************************************************************
*                                   MACROS                                    *
******************************************************************************/
#define concatenate_strings(...) \
	concatenate_n_strings(NUM_ARGS(const char*, __VA_ARGS__), __VA_ARGS__)
#define copy_string(s) concatenate_n_strings(1, s);
/*****************************************************************************/
#endif /* STR_UTL_H */