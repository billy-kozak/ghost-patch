/**************************************
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
#ifndef MUSL_FMT_DOUBLE_H
#define MUSL_FMT_DOUBLE_H
/******************************************************************************
*                                    TYPES                                    *
******************************************************************************/
struct musl_output_obj {
	void(*emit)(void*, char);
	void *emit_arg;
};
/******************************************************************************
*                                   DEFINES                                   *
******************************************************************************/
/* Convenient bit representation for modifier flags, which all fall
 * within 31 codepoints of the space character. */

#define ALT_FORM   (1U<<('#'-' '))
#define ZERO_PAD   (1U<<('0'-' '))
#define LEFT_ADJ   (1U<<('-'-' '))
#define PAD_POS    (1U<<(' '-' '))
#define MARK_POS   (1U<<('+'-' '))
#define GROUPED    (1U<<('\''-' '))
/******************************************************************************
*                            FUNCTION DEFINITIONS                             *
******************************************************************************/
int musl_fmt_fp(
	struct musl_output_obj *out_o,
	long double y,
	int w,
	int p,
	int fl,
	int t
);
/*****************************************************************************/
#endif /* MUSL_FMT_DOUBLE_H */
