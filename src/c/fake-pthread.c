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
#include "fake-pthread.h"

#include "thread-jump.h"

#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>
#include <stdio.h>
#include <signal.h>
#include <sched.h>
#include <string.h>
/******************************************************************************
*                                    TYPES                                    *
******************************************************************************/
struct thread_arg {
	volatile long flag;

	int(*target)(void*);
	int(*target_arg)(void*);

	struct thread_jump tj;
};
/******************************************************************************
*                                  CONSTANTS                                  *
******************************************************************************/
static const size_t TEMP_STACK_SIZE = 2 * 1024 * 1024;
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
static void* pthread_target(void *arg);
static int clone_target(void *arg);
/******************************************************************************
*                            FUNCTION DEFINITIONS                             *
******************************************************************************/
static int clone_target(void *arg)
{
	volatile struct thread_arg *aptr = arg;
	struct thread_jump *tj;

	tj = (struct thread_jump *)&(aptr->tj);

	tj_spinwait(tj);
	tj_jump(tj, 1);

	return 0;
}
/*****************************************************************************/
static void* pthread_target(void *arg)
{
	long targ_ret;

	volatile struct thread_arg *aptr = arg;
	volatile struct thread_arg t_arg;
	struct thread_jump *tj;

	memcpy((void*)&t_arg, (void*)aptr, sizeof(struct thread_arg));

	tj = (struct thread_jump *)&(aptr->tj);

	tj_set_and_exit(tj);
	aptr->flag = 1;

	/* aptr is now inavlid, but we can still access the copy in t_arg */

	targ_ret = t_arg.target(t_arg.target_arg);
	return (void*) targ_ret;
}
/******************************************************************************
*                           FUNCTION DEFINITIONS                              *
******************************************************************************/
int fake_pthread(int(*target)(void* arg), void *arg)
{
	int ret = 0;

	pthread_t thread;
	pthread_attr_t attr;
	uint8_t *stack;
	uint8_t *stack_end;
	int clone_flags;

	volatile struct thread_arg t_arg = {
		0,
		target,
		arg,
		TJ_BUFFER_INITIAL
	};

	stack_end = mmap(
		NULL,
		TEMP_STACK_SIZE,
		PROT_READ | PROT_WRITE,
		MAP_STACK | MAP_PRIVATE | MAP_ANONYMOUS,
		-1,
		0
	);
	if(stack_end == MAP_FAILED) {
		goto cleanup_1;
	}

	if(pthread_attr_init(&attr)) {
		ret = 1;
		goto exit;
	}

	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	if(pthread_create(&thread, &attr, pthread_target, (void*)&t_arg)) {
		ret = 1;
		goto cleanup_2;
	}

	stack = stack_end + TEMP_STACK_SIZE;
	clone_flags =
		CLONE_VM |
		SIGCHLD;

	if(clone(clone_target, stack, clone_flags, (void*)&t_arg) == -1) {
		ret = 1;
		goto cleanup_2;
	}

	while(t_arg.flag == 0);

cleanup_2:
	pthread_attr_destroy(&attr);
cleanup_1:
	munmap(stack_end, TEMP_STACK_SIZE);
exit:
	return ret;
}
/*****************************************************************************/
