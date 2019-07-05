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
#include "tracee-state-table.h"
#include "application.h"
#include "options.h"

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
#include <linux/ptrace.h>
#include <stdbool.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
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

static struct trace_descriptor descriptor;
static void *state_tab;
static struct prog_opts cached_opts;
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
static void call_descriptor(const struct tracee_state *state);
static int load_regs(struct tracee_state *state);
static bool is_syscall_stop(int status);
static bool is_group_stop(int status);
static bool is_event_stop(int status);
static bool is_signal_stop(int status);
static int extract_ptrace_event(int status);
static void modify_syscalls(struct tracee_state *state);
/******************************************************************************
*                              STATIC FUNCTIONS                               *
******************************************************************************/
static void modify_syscalls(struct tracee_state *state)
{
	struct user_regs_struct *regs = &state->data.regs;
	int syscall_no = regs->orig_rax;

	if(!cached_opts.fake_pid) {
		return;
	}

	if(state->status == SYSCALL_ENTER_STOP) {
		return;
	}

	if(regs->rax != child_pid) {
		return;
	}

	if(syscall_no != SYS_getpid) {
		return;
	}

	regs->rax = parent_pid;
	ptrace(PTRACE_SETREGS, state->pid, 0, regs);
}
/*****************************************************************************/
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

	application_set_proc_name();
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
	int exit_status;

	descriptor.arg = descriptor.init(descriptor.arg);

	if(DEBUG_MODE_NO_PTRACE) {
		exit_status = only_wait_for_exit(target_pid);
	} else {
		exit_status = trace_target(target_pid);
	}

	tracee_state_table_destroy(state_tab);

	return exit_status;
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
	struct tracee_state state;
	int status;

	int options =
		PTRACE_O_EXITKILL |
		PTRACE_O_TRACESYSGOOD |
		PTRACE_O_TRACEEXEC |
		PTRACE_O_TRACECLONE;

	waitpid(target_pid, &status, __WALL);

	ptrace(PTRACE_SEIZE, target_pid, 0, options);
	ptrace(PTRACE_SETOPTIONS, target_pid, 0, options);

	state.status = STARTED;
	state.pid = target_pid;

	call_descriptor(&state);

	wait_flag = 1;
	ptrace(PTRACE_SYSCALL, target_pid, 0, 0);

	while(1) {
		int sig = 0;

		if((state.pid = waitpid(-1, &status, __WALL)) == -1) {
			state.status = EXITED_UNEXPECTED;
			call_descriptor(&state);
			break;
		}


		if(WIFEXITED(status)) {
			state.status = EXITED_NORMAL;
			state.data.exit_status = WEXITSTATUS(status);
			call_descriptor(&state);

			if(state.pid == target_pid) {
				return state.data.exit_status;
			}
		} else if(is_syscall_stop(status)) {
			uint8_t prev_state = tracee_state_table_retrieve(
				state_tab, state.pid
			);

			if(prev_state == SYSCALL_ENTER_STOP) {
				state.status = SYSCALL_EXIT_STOP;
			} else {
				state.status = SYSCALL_ENTER_STOP;
			}

			if(load_regs(&state) == 0) {
				modify_syscalls(&state);
				call_descriptor(&state);
			} else {
				state.status = EXITED_UNEXPECTED;
				call_descriptor(&state);

				if(state.pid == target_pid) {
					break;
				}
			}
		} else if(is_group_stop(status)) {

			state.status = GROUP_STOP;
			call_descriptor(&state);

		} else if(is_event_stop(status)) {

			state.data.pt_event = extract_ptrace_event(status);

			if(state.data.pt_event == PTRACE_EVENT_EXEC) {
				state.status = PTRACE_EXEC_OCCURED;
			} else if(state.data.pt_event == PTRACE_EVENT_CLONE) {
				state.status = STARTED;

			} else {
				state.status = PTRACE_EVENT_OCCURED_STOP;

			}

			call_descriptor(&state);

		} else if(is_signal_stop(status)) {
			sig = WSTOPSIG(status);

			state.status = SIGNAL_DELIVERY_STOP;
			state.data.signo = sig;

			call_descriptor(&state);
		}

		tracee_state_table_store(state_tab, state.pid, state.status);

		if(state.status == PTRACE_EXEC_OCCURED) {
			ptrace(PTRACE_DETACH, state.pid, 0, 0);
		} else if(ptrace(PTRACE_SYSCALL, state.pid, 0, sig) == -1) {
			state.status = EXITED_UNEXPECTED;
			call_descriptor(&state);

			if(state.pid == target_pid) {
				break;
			}
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
/*****************************************************************************/
static int load_regs(struct tracee_state *state)
{
	return ptrace(PTRACE_GETREGS, state->pid, 0, &state->data.regs) == -1;
}
/*****************************************************************************/
static void call_descriptor(const struct tracee_state *state)
{
	descriptor.arg = descriptor.handle(descriptor.arg, state);
}
/*****************************************************************************/
static int extract_ptrace_event(int status)
{
	return ((status >> 8) & ~SIGTRAP) >> 8;
}
/*****************************************************************************/
static bool is_syscall_stop(int status)
{
	if(!WIFSTOPPED(status)) {
		return false;
	} else {
		return !!(WSTOPSIG(status) & 0x80);
	}
}
/*****************************************************************************/
static bool is_group_stop(int status)
{
	return (status >> 16) == PTRACE_EVENT_STOP;
}
/*****************************************************************************/
static bool is_event_stop(int status)
{
	int signal = WSTOPSIG(status);

	if(!WIFSTOPPED(status)) {
		return false;
	}

	return (signal == SIGTRAP) && !!(0xFF & (status >> 8));
}
/*****************************************************************************/
static bool is_signal_stop(int status)
{
	return
		WIFSTOPPED(status) &&
		!is_syscall_stop(status) &&
		!is_group_stop(status) &&
		!is_event_stop(status);
}
/******************************************************************************
*                            FUNCTION DEFINITIONS                             *
******************************************************************************/
int start_trace(
	const struct trace_descriptor *descr, struct trace_entities *ents
) {
	state_tab = tracee_state_table_init();

	if(state_tab == NULL) {
		return 1;
	}

	if(get_options(&cached_opts)) {
		return 1;
	}

	memcpy(&descriptor, descr, sizeof(descriptor));

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

	if(ents != NULL) {
		ents->parent = parent_pid;
		ents->child = child_pid;
	}

	return 0;
}
/*****************************************************************************/
