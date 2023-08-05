/******************************************************************************
* Copyright (C) 2023  Billy Kozak                                             *
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
#ifndef MATH_UTL_H
#define MATH_UTL_H
/******************************************************************************
*                                  INCLUDES                                   *
******************************************************************************/
#include <stdint.h>
/******************************************************************************
*                                   MACROS                                    *
******************************************************************************/
#define DIV_ROUND_UP(n, d) (((n) / (d)) + (((n) % (d)) ? 1 : 0))
/******************************************************************************
*                              INLINE_FUNCTIONS                               *
******************************************************************************/
static inline uint64_t align_up_unsigned(uint64_t size, uint64_t align)
{
	if((size % align) == 0) {
		return (size / align) * align;
	} else {
		return ((size / align) * (align)) + align;
	}
}
/*****************************************************************************/
static inline uint64_t align_down_unsigned(uint64_t size, uint64_t align)
{
	return (size / align) * align;
}
/*****************************************************************************/
static inline int64_t math_utl_round(double d)
{
	double floor = (double)((int64_t)d);
	double frac = d - floor;

	if(frac >= 0.5) {
		return ((int64_t)(floor)) + 1;
	} else {
		return (int64_t)(floor);
	}
}
/*****************************************************************************/
#endif /* MATH_UTL_H */

