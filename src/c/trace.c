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

#include "fake-pthread.h"
#include "thread-jump.h"
#include "syscall-utl.h"

#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
/******************************************************************************
*                                    DATA                                     *
******************************************************************************/
static struct thread_jump tj_main;
static struct thread_jump tj_thread;

static volatile pid_t parent_pid;
static volatile pid_t child_pid;
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
static int monitor_thread(void* arg);
static NEVER_INLINE int monitor(void);
/******************************************************************************
*                              STATIC FUNCTIONS                               *
******************************************************************************/
static int monitor_thread(void* arg)
{
	child_pid = getpid();

	tj_swap(&tj_thread, &tj_main, 1);
	assert(arch_prctl_get_fs_nocheck() == tj_thread.fs);

	syscall_exit(monitor());

	return -1;
}
/*****************************************************************************/
static NEVER_INLINE int monitor(void)
{
	int status;

	waitpid(child_pid, &status, 0);

	return status;
}
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
int start_trace(void)
{
	parent_pid = getpid();

	if(fake_pthread(monitor_thread, NULL)) {
		return 1;
	}

	tj_swap(&tj_main, &tj_thread, 1);
	assert(arch_prctl_get_fs_nocheck() == tj_main.fs);

	return 0;
}
/*****************************************************************************/