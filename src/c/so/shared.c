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
#define _GNU_SOURCE
/******************************************************************************
*                                  INCLUDES                                   *
******************************************************************************/
#include "shared.h"

#include "trace.h"
#include "syscall-utl.h"
#include "pseudo-strace.h"
#include "lua-trace.h"
#include "application.h"
#include "options.h"
#include "secret-heap.h"
#include "ghost-signals.h"
#include <gio/ghost-stdio.h>

#include <dlfcn.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <setjmp.h>
#include <sys/types.h>
/******************************************************************************
*                                    DATA                                     *
******************************************************************************/
static pid_t parent_pid;
static pid_t child_pid;

static struct prog_opts cached_opts;
/******************************************************************************
*                              STATIC FUNCTIONS                               *
******************************************************************************/
static bool am_ghost_patch(const char *progname);
/*****************************************************************************/
static void do_special_setup(void)
{
	struct trace_entities ents;
	struct trace_descriptor descr;

	get_options(&cached_opts);
	ghost_signals_init();

	if(cached_opts.lua_ent == NULL) {
		descr = pseudo_strace_descriptor();
	} else {
		descr = lua_trace_descriptor(cached_opts.lua_ent);
	}

	if(start_trace(&descr, &ents)) {
		perror("Unable to start trace");
	}

	parent_pid = ents.parent;
	child_pid = ents.child;
}
/*****************************************************************************/
static bool am_ghost_patch(const char *progname)
{
	const char *fname = basename(progname);

	/* avoid using strcmp in the pre-main execution environment
	 * In some targets, use of certain library functions causes
	* sigfaults */
	int i = 0;
	while((fname[i] != '\0') && (APPLICATION_NAME[i] != '\0')) {
		if(fname[i] != APPLICATION_NAME[i]) {
			return false;
		}
		i += 1;
	}

	return (fname[i] == '\0') && (APPLICATION_NAME[i] == '\0');
}
/*****************************************************************************/
static int fake_main(int argc, char **argv, char **envp)
{
	if(!am_ghost_patch(argv[0])) {
		do_special_setup();
	}
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
	secret_heap_init();
	ghost_stdio_init();

	int (*real_libc_start_main)
		(
			int (*main)(int, char **, char **),
			int argc,
			char **ubp_av,
			void (*init)(void),
			void (*fini)(void),
			void (*rtld_fini) (void),
			void (* stack_end)
		);

	char **argv = ubp_av;
	char **envp = ubp_av + argc + 1;

	int fake_ret = fake_main(argc, argv, envp);

	if(fake_ret < 0)  {
		return fake_ret;
	}

	real_libc_start_main = dlsym(RTLD_NEXT, "__libc_start_main");
	return real_libc_start_main(
		main,
		argc,
		ubp_av,
		init,
		fini,
		rtld_fini,
		stack_end
	);
}
/*****************************************************************************/
EXPORT pid_t getpid(void)
{
	pid_t result = syscall_getpid();

	if(cached_opts.fake_pid && (result == child_pid)) {
		return parent_pid;
	} else {
		return result;
	}
}
/*****************************************************************************/
