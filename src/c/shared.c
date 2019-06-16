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
#define _GNU_SOURCE
/******************************************************************************
*                                  INCLUDES                                   *
******************************************************************************/
#include "shared.h"

#include "trace.h"

#include <dlfcn.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <setjmp.h>
/******************************************************************************
*                              STATIC FUNCTIONS                               *
******************************************************************************/
static bool am_py_trace(const char *progname);
static sigjmp_buf jump_buffer;
/*****************************************************************************/
static void do_special_setup(void)
{
	if(start_trace()) {
		perror("Unable to start trace");
	}
}
/*****************************************************************************/
static bool am_py_trace(const char *progname)
{
	return strcmp(basename(progname), "py-trace") == 0;
}
/*****************************************************************************/
static int fake_main(int argc, char **argv, char **envp)
{
	if(!am_py_trace(argv[0])) {
		do_special_setup();
	}

	siglongjmp(jump_buffer, 1);
	return 0;
}
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
EXPORT int __libc_start_main(
	int (*main)(int, char **, char **),
	int argc,
	char **ubp_av,
	void (*init)(void),
	void (*fini)(void),
	void (*rtld_fini) (void),
	void (* stack_end)
) {
	int (*real)
		(
			int (*main)(int, char **, char **),
			int argc,
			char **ubp_av, void (*init)(void),
			void (*fini)(void),
			void (*rtld_fini) (void),
			void (* stack_end)
		) = dlsym(RTLD_NEXT, "__libc_start_main");

	if(sigsetjmp(jump_buffer, 0) == 0) {
		return real(
			fake_main,
			argc,
			ubp_av,
			init,
			fini,
			rtld_fini,
			stack_end
		);
	} else {
		return real(
			main,
			argc,
			ubp_av,
			init,
			fini,
			rtld_fini,
			stack_end
		);
	}


}
/*****************************************************************************/