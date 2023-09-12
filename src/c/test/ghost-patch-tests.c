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
/******************************************************************************
*                                  INCLUDES                                   *
******************************************************************************/
#include <suites/test-suites.h>

#include <picounit/picounit.h>

#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
/******************************************************************************
*                                  CONSTANTS                                  *
******************************************************************************/
static const struct option GETOPT_OPTIONS[] = {
	{"help", no_argument, NULL, 'h'},
	{"test", required_argument, NULL, 't'},
	{"ls", no_argument, NULL, 'l'},
	{NULL, 0, 0, 0}
};

static const char OPT_STRING[] = "+hlt:";

static const char HELP_TEXT[] =
	"Run ghost-patch unit tests"
	"\n"
	"Options:\n"
	"-h,  --help     Display this help text\n"
	"--test=<NAME>   Run the given named test only. If not given then\n"
	"                all tests are run\n"
	"-l, --ls        List all namd tests and exit\n";

static const char* NAMED_TEST[] = {
	"stdio",
	"malloc"
};

#define NUM_TESTS (sizeof(NAMED_TEST) / sizeof(NAMED_TEST[0]))
/******************************************************************************
*                              STATIC FUNCTIONS                               *
******************************************************************************/
static void print_named_test_err(const char *name)
{
	fprintf(stderr, "Error: no such test '%s'\n", name);
}
/*****************************************************************************/
static void print_named_tests(void)
{
	for(int i = 0; i < NUM_TESTS; i++) {
		printf("%s\n", NAMED_TEST[i]);
	}
}
/*****************************************************************************/
static void run_test(int idx)
{
	switch(idx) {
	case 0:
		PUNIT_RUN_SUITE(test_suite_ghost_stdio);
		break;
	case 1:
		PUNIT_RUN_SUITE(test_suite_ghost_malloc);
		break;
	default:
		fprintf(stderr, "Error: no such text number %d\n", idx);
	}
}
/*****************************************************************************/
static void run_tests(int idx)
{
	if(idx < 0) {
		for(int i = 0; i < NUM_TESTS; i++) {
			run_test(i);
		}
	} else {
		run_test(idx);
	}

	punit_print_stats();
}
/*****************************************************************************/
static int test_name_to_idx(const char *name)
{
	for(int i = 0; i < NUM_TESTS; i++) {
		if(strcmp(NAMED_TEST[i], name) == 0) {
			return i;
		}
	}

	return -1;
}
/******************************************************************************
*                            FUNCTION DEFINITIONS                             *
******************************************************************************/
int main(int argc, char **argv)
{
	int opt_ind = 0;
	bool flag = true;

	int test_idx = -1;

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
			return 0;
		case 't':
			test_idx = test_name_to_idx(optarg);
			if(test_idx < 0) {
				print_named_test_err(optarg);
				return -1;
			}
			break;
		case 'l':
			print_named_tests();
			return 0;
		default:
			return -1;
		}
	}

	run_tests(test_idx);

	return 0;
}
/*****************************************************************************/
