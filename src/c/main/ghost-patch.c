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
#include <proc-utl.h>
#include <debug-modes.h>
#include <set-options.h>
#include <str-utl-libc.h>
#include <utl/str-utl.h>
#include <str-utl-libc.h>

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
	{"lua", required_argument, NULL, 'l'},
	{"help", no_argument, NULL, 'h'},
	{NULL, 0, 0, 0}
};
static const char OPT_STRING[] = "+hpl:";
static const char HELP_TEXT[] =
	"Start a thread in the target program to ptrace the target.\n"
	"\n"
	"Options:\n"
	"-h,  --help      Display this help text.\n"
	"--lua=<LUA_PATH> Path to lua script to run for trace.\n"
	"-p, --real-pid   Don't fake the process ID of the target process.\n"
	"                 This programs runs the target in a child process\n"
	"                 means that the output of the getpid() system call\n"
	"                 will appear to be inconsistent with the pid given\n"
	"                 by the parent process' call to clone() or fork().\n"
	"                 To get around this, by default this program\n"
	"                 intercepts the getpid() system call in order to\n"
	"                 make it appear that the target process has the pid\n"
	"                 of it's parent. This option disables this\n"
	"                 behaviour.\n";
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
		case 'l':
			aptr->lua_ent = optarg;
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
static char *shared_object(void)
{
	char *executable = this_executable();
	size_t len = strlen(executable) + 1;
	size_t new_len = len + 3;

	char *so = realloc(executable, new_len);

	assert(so != NULL);

	strcat(so, ".so");

	return so;
}
/*****************************************************************************/
static void setup_ld_preload(void)
{
	char *executable = shared_object();
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

	setup_ld_preload();

	if(execvp(argv[targ_arg_index], argv + targ_arg_index)) {
		perror(NULL);
	}

	return -1;
}
/*****************************************************************************/
