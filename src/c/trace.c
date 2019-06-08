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
#include "trace.h"

#include <stdint.h>
#include <sched.h>
#include <sys/mman.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
/******************************************************************************
*                                  CONSTANTS                                  *
******************************************************************************/
static const size_t MONITOR_THREAD_TARGET_STACK_SIZE = 8 * 1024 * 1024;
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
static int monitor_thread(void* arg);
/******************************************************************************
*                              STATIC FUNCTIONS                               *
******************************************************************************/
static size_t thread_stack_size(void)
{
	long page_size = sysconf(_SC_PAGE_SIZE);

	if(MONITOR_THREAD_TARGET_STACK_SIZE % page_size) {
		return
			page_size *
			((MONITOR_THREAD_TARGET_STACK_SIZE / page_size) + 1);
	} else {
		return MONITOR_THREAD_TARGET_STACK_SIZE;
	}
}
/*****************************************************************************/
static int monitor_thread(void* arg)
{
	printf("Monitor thread online\n");
	return 0;
}
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
int start_trace(void)
{
	size_t stack_size = thread_stack_size();
	uint8_t* stack = mmap(
		NULL,
		stack_size,
		PROT_READ | PROT_WRITE,
		MAP_STACK | MAP_PRIVATE | MAP_ANONYMOUS,
		-1,
		0
	);
	uint8_t * stack_end = stack + stack_size;

	if(stack == MAP_FAILED) {
		goto fail_0;
	}

	if(clone(monitor_thread, stack_end, CLONE_VM | SIGCHLD, NULL) == -1) {
		goto fail_1;
	}

	return 0;
fail_1:
	munmap(stack, stack_size);
fail_0:
	return -1;
}
/*****************************************************************************/