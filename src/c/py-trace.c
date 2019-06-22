/******************************************************************************
* Copyright (C) 2019  Billy Kozak                                             *
*                                                                             *
* This file is part of the py-trace program                                   *
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
#include "str-utl.h"
#include "debug-modes.h"
#include "trace.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
static void setup_ld_preload(void);
/******************************************************************************
*                              STATIC FUNCTIONS                               *
******************************************************************************/
static void setup_ld_preload(void)
{
	char *executable = this_executable();
	char *current = getenv("LD_PRELOAD");
	char *new = NULL;

	if(current == NULL) {
		new = copy_string(executable);
	} else {
		new = concatenate_strings(executable, ":", current);
	}

	setenv("LD_PRELOAD", new, 1);

	free(executable);
	free(new);
}
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
int main(int argc, char **argv)
{
	if(argc <= 1) {
		return 0;
	}

	if(DEBUG_MODE_NO_THREAD) {
		start_trace();
	} else {
		setup_ld_preload();
	}


	if(execvp(argv[1], argv + 1)) {
		perror(NULL);
	}

	return -1;
}
/*****************************************************************************/