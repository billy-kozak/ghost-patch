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
#include "proc-utl.h"
#include "path-utl.h"

#include <utl/str-utl.h>
#include <str-utl-libc.h>

#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
char *this_executable(void)
{
	char *str_pid = int_to_string(getpid());
	char *sym_path = concatenate_strings("/proc/", str_pid, "/exe");
	char *exe_path = safe_resolve_symlink(sym_path);

	free(str_pid);
	free(sym_path);

	return exe_path;
}
/*****************************************************************************/
