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
#include "debug-modes.h"

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
static NEVER_INLINE int monitor(pid_t target_pid);
static void setup_signal_handling(void);
static void signal_forwarder_handler(
	int signo, siginfo_t *info, void *ucontext
);
static int only_wait_for_exit(pid_t target_pid);
static int start_monitor(void);
static int trace_target(pid_t target_pid);
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

	syscall_exit(monitor(child_pid));

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
static NEVER_INLINE int monitor(pid_t target_pid)
{
	if(DEBUG_MODE_NO_PTRACE) {
		return only_wait_for_exit(target_pid);
	} else {
		return trace_target(target_pid);
	}
}
/*****************************************************************************/
static int only_wait_for_exit(pid_t target_pid)
{
	int status;

	wait_flag = 1;

	while(1) {

		if(waitpid(target_pid, &status, 0) == -1) {
			break;
		}

		if(WIFEXITED(status)) {
			return WEXITSTATUS(status);
		}
	}

	return -1;
}
/*****************************************************************************/
static int trace_target(pid_t target_pid)
{
	int status;

	waitpid(target_pid, &status, 0);
	ptrace(PTRACE_SETOPTIONS, target_pid, 0, PTRACE_O_TRACESYSGOOD);

	wait_flag = 1;

	ptrace(PTRACE_CONT, target_pid, 0, 0);

	while(1) {

		if(waitpid(target_pid, &status, 0) == -1) {
			perror(NULL);
			break;
		}

		if(WIFSTOPPED(status)) {
			ptrace(PTRACE_CONT, target_pid, 0, 0);
		} else if(WIFEXITED(status)) {
			return WEXITSTATUS(status);
		}
	}

	return -1;
}
/*****************************************************************************/
static int start_monitor(void)
{
	if(DEBUG_MODE_NO_THREAD) {
		if((child_pid = fork()) != 0) {
			syscall_exit(monitor(child_pid));
		} else {
			child_pid = syscall_getpid();
		}
		return 0;
	} else {
		if(fake_pthread(monitor_thread, NULL)) {
			return 1;
		}

		tj_swap(&tj_main, &tj_thread, 1);
		assert(arch_prctl_get_fs_nocheck() == tj_main.fs);

		return 0;
	}
}
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
int start_trace(void)
{
	parent_pid = syscall_getpid();

	if(start_monitor()) {
		return 1;
	}

	if(DEBUG_MODE_NO_PTRACE == 0) {
		ptrace(PTRACE_TRACEME, 0, 0, 0);
		kill(child_pid, SIGSTOP);
	}

	if(DEBUG_MODE_NO_THREAD == 0) {
		while(wait_flag == 0);
	}

	return 0;
}
/*****************************************************************************/