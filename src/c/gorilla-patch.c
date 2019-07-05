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
/******************************************************************************
*                                  INCLUDES                                   *
******************************************************************************/
#include "proc-utl.h"
#include "str-utl.h"
#include "debug-modes.h"
#include "trace.h"
#include "pseudo-strace.h"
#include "options.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>
#include <assert.h>
/******************************************************************************
*                                  CONSTANTS                                  *
******************************************************************************/
static const struct option GETOPT_OPTIONS[] = {
	{"real-pid", no_argument, NULL, 'p'},
	{"help", no_argument, NULL, 'h'},
	{NULL, 0, 0, 0}
};
static const char OPT_STRING[] = "+hp";
static const char HELP_TEXT[] =
	"Start a thread in the target program to ptrace the target.\n"
	"\n"
	"Options:\n"
	"-h,  --help     Display this help text\n"
	"-p, --real-pid  Don't fake the process ID of the target process.\n"
	"                This programs runs the target in a child process\n"
	"                means that the output of the getpid() system call\n"
	"                will appear to be inconsistent with the pid given\n"
	"                by the parent process' call to clone() or fork().\n"
	"                To get around this, by default this program\n"
	"                intercepts the getpid() system call in order to\n"
	"                make it appear that the target process has the pid\n"
	"                of it's parent. This option disables this\n"
	"                behaviour.\n";
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
static void setup_ld_preload(void);
static int target_args_pos(int argc, char **argv);
static int parse_arguments(int argc, char **argv, struct prog_opts *aptr);
/******************************************************************************
*                              STATIC FUNCTIONS                               *
******************************************************************************/
static int parse_arguments(int argc, char **argv, struct prog_opts *aptr)
{
	struct prog_opts defaults = DEFAULT_PROG_ARGS;
	int opt_ind = 0;
	bool flag = true;

	memcpy(aptr, &defaults, sizeof(*aptr));

	while(flag) {
		int c = getopt_long(
			argc, argv, OPT_STRING, GETOPT_OPTIONS, &opt_ind
		);
		switch(c) {
		case -1:
			flag = false;
			break;
		case 'h':
			printf("%s", HELP_TEXT);
			exit(0);
			break;
		case 'p':
			aptr->fake_pid = false;
			break;
		case '?':
			flag = false;
			return -1;
		default:
			assert(false);
			flag = false;
			return -1;
		}
	}

	return 0;
}
/*****************************************************************************/
static int target_args_pos(int argc, char **argv)
{

	for(int i = 1; i < argc; i++) {
		if(argv[i][0] != '-') {
			return i;
		} else if(strcmp(argv[i], "--") == 0) {
			return i != argc ? i + 1 : -1;
		}
	}

	return -1;
}
/*****************************************************************************/
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
	struct prog_opts parsed_args;
	int targ_arg_index = target_args_pos(argc, argv);

	if(parse_arguments(argc, argv, &parsed_args)) {
		return -1;
	}

	if(targ_arg_index < 0) {
		return 0;
	}

	if(set_options(&parsed_args)) {
		perror(NULL);
		return -1;
	}

	if(DEBUG_MODE_NO_THREAD) {
		struct trace_descriptor descr = pseudo_strace_descriptor();

		start_trace(&descr, NULL);
	} else {
		setup_ld_preload();
	}


	if(execvp(argv[targ_arg_index], argv + targ_arg_index)) {
		perror(NULL);
	}

	return -1;
}
/*****************************************************************************/