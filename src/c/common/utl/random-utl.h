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
#ifndef RANDOM_UTL_H
#define RANDOM_UTL_H
/******************************************************************************
*                                  INCLUDES                                   *
******************************************************************************/
#include <stdlib.h>
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
int random_utl_seed_from_urandom(struct drand48_data *data);
int random_utl_seed_from_clock(struct drand48_data *data);
void random_utl_rand_alpha_num(
	struct drand48_data *restrict data,
	char *restrict str,
	size_t len
);
/*****************************************************************************/
#endif /* RANDOM_UTL_H */
