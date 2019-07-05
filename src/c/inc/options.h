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
#ifndef OPTIONS_H
#define OPTIONS_H
/******************************************************************************
*                                  INCLUDES                                   *
******************************************************************************/
#include <stdbool.h>
/******************************************************************************
*                                    TYPES                                    *
******************************************************************************/
struct prog_opts {
	bool fake_pid;
};
/******************************************************************************
*                                   DEFINES                                   *
******************************************************************************/
#define DEFAULT_PROG_ARGS {true};
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
int set_options(const struct prog_opts *opts);
int get_options(struct prog_opts *opts);
/*****************************************************************************/
#endif /* OPTIONS_H */
