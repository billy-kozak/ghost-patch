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
/******************************************************************************
*                                  INCLUDES                                   *
******************************************************************************/
#include "pseudo-strace.h"

#include "trace.h"

#include <stdlib.h>
#include <stdio.h>
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
static void* init(void *arg);
static void* handle(void *argg, const struct tracee_state *state);
/******************************************************************************
*                              STATIC FUNCTIONS                               *
******************************************************************************/
static void* init(void *arg)
{
	return fopen("/dev/stderr", "w");
}
/*****************************************************************************/
static void* handle(void *arg, const struct tracee_state *state)
{
	if(state->status == SYSCALL_ENTER_STOP) {
		fprintf(
			arg,
			"[ID %d]: Enter syscall: %llu\n",
			state->pid,
			state->data.regs.orig_rax
		);
	} else if(state->status == SYSCALL_EXIT_STOP) {
		fprintf(
			arg,
			"[ID %d]: Exit syscall: %llu\n",
			state->pid,
			state->data.regs.orig_rax
		);
	} else if(state->status == EXITED_NORMAL) {
		fprintf(
			arg,
			"[ID %d]: Exited: %d\n",
			state->pid,
			state->data.exit_status
		);
	}

	return arg;
}
/******************************************************************************
*                            FUNCTION DEFINITIONS                             *
******************************************************************************/
struct trace_descriptor pseudo_strace_descriptor(void)
{
	struct trace_descriptor descr;

	descr.handle = handle;
	descr.init = init;
	descr.arg = NULL;

	return descr;
}
/*****************************************************************************/