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
#include "misc-macros.h"

#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <sys/ptrace.h>
/******************************************************************************
*                                   DEFINES                                   *
******************************************************************************/
#ifndef DEBUG_MODE_NO_PTRACE
#define DEBUG_MODE_NO_PTRACE 0
#endif
/******************************************************************************
*                                  CONSTANTS                                  *
******************************************************************************/
static const int SIGNALS_TO_FORWARD[] = {
	SIGHUP, SIGINT, SIGQUIT, SIGKILL, SIGPIPE, SIGALRM,
	SIGUSR1, SIGUSR2, SIGTSTP, SIGTTIN, SIGTTOU
};
/******************************************************************************
*                                    DATA                                     *
******************************************************************************/
static struct thread_jump tj_main;
static struct thread_jump tj_thread;

static volatile pid_t parent_pid;
static volatile pid_t child_pid;

static volatile int wait_flag;
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
static int monitor_thread(void* arg);
static NEVER_INLINE int monitor(void);
static void setup_signal_handling(void);
static void signal_forwarder_handler(
	int signo, siginfo_t *info, void *ucontext
);
static NEVER_INLINE int only_wait_for_exit(void);
/******************************************************************************
*                              STATIC FUNCTIONS                               *
******************************************************************************/
static void signal_forwarder_handler(
	int signo, siginfo_t *info, void *ucontext
) {
	kill(child_pid, signo);
}
/*****************************************************************************/
static int monitor_thread(void* arg)
{
	child_pid = syscall_getpid();

	tj_swap(&tj_thread, &tj_main, 1);
	assert(arch_prctl_get_fs_nocheck() == tj_thread.fs);

	setup_signal_handling();

	if(DEBUG_MODE_NO_PTRACE) {
		syscall_exit(only_wait_for_exit());
	} else {
		syscall_exit(monitor());
	}


	return -1;
}
/*****************************************************************************/
static void setup_signal_handling(void)
{
	struct sigaction fwd_action;

	fwd_action.sa_sigaction = signal_forwarder_handler;
	fwd_action.sa_flags = SA_RESTART | SA_SIGINFO;
	sigemptyset(&fwd_action.sa_mask);

	for(int i = 0; i < ARR_SIZE(SIGNALS_TO_FORWARD); i++) {
		sigaction(SIGNALS_TO_FORWARD[i], &fwd_action, NULL);
	}
}
/*****************************************************************************/
static NEVER_INLINE int only_wait_for_exit(void)
{
	int status;

	wait_flag = 1;

	while(1) {

		if(waitpid(child_pid, &status, 0) == -1) {
			break;
		}

		if(WIFEXITED(status)) {
			return WEXITSTATUS(status);
		}
	}

	return -1;
}
/*****************************************************************************/
static NEVER_INLINE int monitor(void)
{
	int status;

	waitpid(child_pid, &status, 0);
	ptrace(PTRACE_SETOPTIONS, child_pid, 0, PTRACE_O_TRACESYSGOOD);

	wait_flag = 1;

	while(1) {
		ptrace(PTRACE_SYSCALL, child_pid, 0, 0);
		waitpid(child_pid, &status, 0);

		if(WIFSTOPPED(status)) {
			ptrace(PTRACE_SYSCALL, child_pid, 0, WSTOPSIG(status));
		} else if(WIFEXITED(status)) {
			return WEXITSTATUS(status);
		}
	}

	return -1;
}
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
int start_trace(void)
{
	parent_pid = syscall_getpid();

	if(fake_pthread(monitor_thread, NULL)) {
		return 1;
	}

	tj_swap(&tj_main, &tj_thread, 1);
	assert(arch_prctl_get_fs_nocheck() == tj_main.fs);

	if(DEBUG_MODE_NO_PTRACE == 0) {
		ptrace(PTRACE_TRACEME, 0, 0, 0);
		kill(child_pid, SIGSTOP);
	}

	while(wait_flag == 0);

	return 0;
}
/*****************************************************************************/