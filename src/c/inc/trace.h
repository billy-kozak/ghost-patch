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
#ifndef TRACE_H
#define TRACE_H
/******************************************************************************
*                                  INCLUDES                                   *
******************************************************************************/
#include <sys/types.h>
#include <sys/user.h>
/******************************************************************************
*                                    TYPES                                    *
******************************************************************************/
enum tracee_status {
	STARTED,
	EXITED_NORMAL,
	EXITED_UNEXPECTED,
	SYSCALL_ENTER_STOP,
	SYSCALL_EXIT_STOP,
	SIGNAL_DELIVERY_STOP,
	GROUP_STOP,
	PTRACE_EVENT_OCCURED_STOP
};
/*****************************************************************************/
struct tracee_state {
	enum tracee_status status;
	pid_t pid;

	union {
		int exit_status;
		int signo;
		int pt_event;
		struct user_regs_struct regs;
	} data;
};
/*****************************************************************************/
typedef void* (*trace_handler)(void *arg, const struct tracee_state *state);
typedef void* (*trace_handler_init)(void *arg);
/*****************************************************************************/
struct trace_descriptor {
	trace_handler handle;
	trace_handler_init init;
	void *arg;
};
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
int start_trace(const struct trace_descriptor *descr);
/*****************************************************************************/
#endif /* TRACE_H */