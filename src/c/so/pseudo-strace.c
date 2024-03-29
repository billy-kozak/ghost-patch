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
#define _GNU_SOURCE
/******************************************************************************
*                                  INCLUDES                                   *
******************************************************************************/
#include "pseudo-strace.h"

#include "trace.h"
#include <gio/ghost-stdio.h>
#include <trace-print-tools.h>

#include <stdlib.h>
#include <stdio.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <stdint.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <sys/mman.h>
/******************************************************************************
*                                    TYPES                                    *
******************************************************************************/
struct named_flag {
	const char *name;
	int flag;
};
/******************************************************************************
*                                   MACROS                                    *
******************************************************************************/
#define SYSCALL_ARG(type, n, regs) (type)syscall_arg(n, regs)
#define SYSCALL_RETVAL(type, regs) (type)syscall_retval(regs)

#define SYSCALL_BUF(str, slen, n, x, regs) \
	sprint_buffer( \
		SYSCALL_ARG(char*, n, regs), \
		str, \
		SYSCALL_ARG(ssize_t, x, regs), \
		slen \
	)
#define SYSCALL_STR(str, slen, n, regs) \
	sprint_buffer( \
		SYSCALL_ARG(char*, n, regs), \
		str, \
		strlen(SYSCALL_ARG(char*, n, regs)), \
		slen \
	)

#define SYSCALL_FLAG(str, slen, names, n, regs) \
	sprint_flags(str, slen, names, SYSCALL_ARG(int, n, regs))

/******************************************************************************
*                                  CONSTANTS                                  *
******************************************************************************/
static const ssize_t PRINT_BUFFER_SIZE = 256;

static const struct named_flag MMAP_FLAGS[] = {
	{"MAP_SHARED", MAP_SHARED},
	{"MAP_PRIVATE", MAP_PRIVATE},
	{"MAP_32BIT", MAP_32BIT},
	{"MAP_ANON", MAP_ANON},
	{"MAP_ANONYMOUS", MAP_ANONYMOUS},
	{"MAP_DENYWRITE", MAP_DENYWRITE},
	{"MAP_EXECUTABLE", MAP_EXECUTABLE},
	{"MAP_FILE", MAP_FILE},
	{"MAP_FIXED", MAP_FIXED},
	{"MAP_GROWSDOWN", MAP_GROWSDOWN},
	{"MAP_HUGETLB", MAP_HUGETLB},
	{"MAP_LOCKED", MAP_LOCKED},
	{"MAP_NONBLOCK", MAP_NONBLOCK},
	{"MAP_NORESERVE", MAP_NORESERVE},
	{"MAP_POPULATE", MAP_POPULATE},
	{"MAP_STACK", MAP_STACK},
	{NULL}
};

static const struct named_flag MPROTECT_FLAGS[] = {
	{"PROT_NONE", PROT_NONE},
	{"PROT_READ", PROT_READ},
	{"PROT_WRITE", PROT_WRITE},
	{"PROT_EXEC", PROT_EXEC},
	{"PROT_GROWSUP", PROT_GROWSUP},
	{"PROT_GROWSDOWN", PROT_GROWSDOWN},
	{NULL}
};
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
static void* init(void *arg);
static void* handle(void *argg, const struct tracee_state *state);
static void print_syscall(
	struct ghost_file *fp, pid_t pid, const struct user_regs_struct *regs
);
static uint64_t syscall_retval(const struct user_regs_struct *regs);
static uint64_t syscall_arg(int n, const struct user_regs_struct *regs);
static char *sprint_flags(
	char *str, ssize_t size, const struct named_flag *names, int flag
);
/******************************************************************************
*                              STATIC FUNCTIONS                               *
******************************************************************************/
static char *sprint_flags(
	char *str, ssize_t size, const struct named_flag *names, int flag
) {
	const char zero[] = "0";
	const char continuation[] = "|...";
	char *p = str;

	if(size <= sizeof(zero)) {
		return NULL;
	} else if(flag == 0) {
		memcpy(p, zero, sizeof(zero));
		return str;
	}

	size -= sizeof(continuation);

	if(size <= 0 ) {
		return NULL;
	}

	for(const struct named_flag *n = names; n->name != NULL; n += 1) {
		size_t slen = strlen(n->name);

		if(!(n->flag & flag)) {
			continue;
		}

		if(slen > (size + 1)) {
			memcpy(p, continuation, sizeof(continuation));
			return str;
		}

		if(p != str) {
			p[0] = '|';
			p += 1;
		}

		memcpy(p, n->name, slen);
		p += slen;
	}

	*p = '\0';
	return str;
}
/*****************************************************************************/
static uint64_t syscall_retval(const struct user_regs_struct *regs)
{
	return regs->rax;
}
/*****************************************************************************/
static uint64_t syscall_arg(int n, const struct user_regs_struct *regs)
{
	assert(n < 6);

	switch(n) {
	case 0:
		return regs->rdi;
	case 1:
		return regs->rsi;
	case 2:
		return regs->rdx;
	case 3:
		return regs->r10;
	case 4:
		return regs->r8;
	case 5:
		return regs->r9;
	default:
		return syscall_retval(regs);
	}
}
/*****************************************************************************/
static void print_syscall(
	struct ghost_file *fp, pid_t pid, const struct user_regs_struct *regs
) {
	char p_buffer_1[PRINT_BUFFER_SIZE];
	char p_buffer_2[PRINT_BUFFER_SIZE];

	int syscall_no = regs->orig_rax;

	switch(syscall_no) {
	case SYS_read:
		ghost_fprintf(
			fp, "[ID %d]: read(%d, %p, %ld) = %d (%s)\n",
			pid,
			SYSCALL_ARG(int,     0, regs),
			SYSCALL_ARG(void*,   1, regs),
			SYSCALL_ARG(int64_t, 2, regs),
			SYSCALL_RETVAL(int, regs),
			SYSCALL_BUF(p_buffer_1, PRINT_BUFFER_SIZE, 1, -1, regs)
		);
		break;
	case SYS_write:
		ghost_fprintf(
			fp, "[ID %d]: write(%d, %s, %ld) = %d\n",
			pid,
			SYSCALL_ARG(int,     0, regs),
			SYSCALL_BUF(p_buffer_1, PRINT_BUFFER_SIZE, 1, 2, regs),
			SYSCALL_ARG(int64_t, 2, regs),
			SYSCALL_RETVAL(int, regs)
		);
		break;
	case SYS_open:
		ghost_fprintf(
			fp, "[ID %d]: open(%s, %d, %ld) = %d\n",
			pid,
			SYSCALL_STR(p_buffer_1, PRINT_BUFFER_SIZE, 0, regs),
			SYSCALL_ARG(int,     1, regs),
			SYSCALL_ARG(int64_t, 2, regs),
			SYSCALL_RETVAL(int, regs)
		);
		break;
	case SYS_close:
		ghost_fprintf(
			fp, "[ID %d]: close(%d) = %d\n",
			pid,
			SYSCALL_ARG(int,     0, regs),
			SYSCALL_RETVAL(int, regs)
		);
		break;
	case SYS_fstat:
		ghost_fprintf(
			fp, "[ID %d]: fstat(%d, %p) = %d\n",
			pid,
			SYSCALL_ARG(int,     0, regs),
			SYSCALL_ARG(void*,   1, regs),
			SYSCALL_RETVAL(int, regs)
		);
		break;
	case SYS_lseek:
		ghost_fprintf(
			fp, "[ID %d]: lseek(%u, %d, %d) = %d\n",
			pid,
			SYSCALL_ARG(uint32_t,   0, regs),
			SYSCALL_ARG(int,        1, regs),
			SYSCALL_ARG(int,        2, regs),
			SYSCALL_RETVAL(int, regs)
		);
	case SYS_mmap:
		ghost_fprintf(
			fp, "[ID %d]: mmap(%p, %ld, %s, %s, %d, %lu) = %p\n",
			pid,
			SYSCALL_ARG(void*,    0, regs),
			SYSCALL_ARG(int64_t,  1, regs),
			SYSCALL_FLAG(
				p_buffer_1, PRINT_BUFFER_SIZE,
				MPROTECT_FLAGS, 2, regs
			),
			SYSCALL_FLAG(
				p_buffer_2, PRINT_BUFFER_SIZE,
				MMAP_FLAGS, 3, regs
			),
			SYSCALL_ARG(int,      4, regs),
			SYSCALL_ARG(uint64_t, 5, regs),
			SYSCALL_RETVAL(void*,    regs)
		);
		break;
	case SYS_mprotect:
		ghost_fprintf(
			fp, "[ID %d]: mprotect(%p, %ld, %s) = %d\n",
			pid,
			SYSCALL_ARG(void*,    0, regs),
			SYSCALL_ARG(size_t,   1, regs),
			SYSCALL_FLAG(
				p_buffer_1, PRINT_BUFFER_SIZE,
				MPROTECT_FLAGS, 2, regs
			),
			SYSCALL_RETVAL(int,      regs)
		);
		break;
	case SYS_munmap:
		ghost_fprintf(
			fp, "[ID %d]: munmap(%p, %lu) = %d\n",
			pid,
			SYSCALL_ARG(void*,    0, regs),
			SYSCALL_ARG(uint64_t, 1, regs),
			SYSCALL_RETVAL(int,      regs)
		);
		break;
	case SYS_rt_sigaction:
		ghost_fprintf(
			fp, "[ID %d]: sigaction(%d, %p, %p, %ld) = %d\n",
			pid,
			SYSCALL_ARG(int,      0, regs),
			SYSCALL_ARG(void*,    1, regs),
			SYSCALL_ARG(void*,    2, regs),
			SYSCALL_ARG(size_t,   3, regs),
			SYSCALL_RETVAL(int,      regs)
		);
		break;
	case SYS_ioctl:
		ghost_fprintf(
			fp, "[ID %d]: ioctl(%d, %lu, %p) = %d\n",
			pid,
			SYSCALL_ARG(int,       0, regs),
			SYSCALL_ARG(uint64_t,  1, regs),
			SYSCALL_ARG(void*,     2, regs),
			SYSCALL_RETVAL(int,    regs)
		);
		break;
	case SYS_access:
		ghost_fprintf(
			fp, "[ID %d]: access(%s, %d) = %d\n",
			pid,
			SYSCALL_STR(p_buffer_1, PRINT_BUFFER_SIZE, 0, regs),
			SYSCALL_ARG(int,       1, regs),
			SYSCALL_RETVAL(int,    regs)
		);
		break;
	case SYS_getpid:
		ghost_fprintf(
			fp, "[ID %d]: getpid() = %d\n",
			pid,
			SYSCALL_RETVAL(int,    regs)
		);
		break;
	case SYS_socket:
		ghost_fprintf(
			fp, "[ID %d]: socket(%d, %d, %d) = %d\n",
			pid,
			SYSCALL_ARG(int,       0, regs),
			SYSCALL_ARG(int,       1, regs),
			SYSCALL_ARG(int,       2, regs),
			SYSCALL_RETVAL(int,    regs)
		);
		break;
	case SYS_connect:
		ghost_fprintf(
			fp, "[ID %d]: connect(%d, %p, %d) = %d\n",
			pid,
			SYSCALL_ARG(int,       0, regs),
			SYSCALL_ARG(void*,     1, regs),
			SYSCALL_ARG(int,       2, regs),
			SYSCALL_RETVAL(int,    regs)
		);
		break;
	case SYS_clone:
		ghost_fprintf(
			fp, "[ID %d]: clone(%lu, %lu, %p, %d) = %d\n",
			pid,
			SYSCALL_ARG(uint64_t,  0, regs),
			SYSCALL_ARG(uint64_t,  1, regs),
			SYSCALL_ARG(void*,     2, regs),
			SYSCALL_ARG(int,       3, regs),
			SYSCALL_RETVAL(int,    regs)
		);
		break;
	case SYS_getdents:
		ghost_fprintf(
			fp, "[ID %d]: getdents(%d, %p, %d) = %d\n",
			pid,
			SYSCALL_ARG(int,       0, regs),
			SYSCALL_ARG(void*,     1, regs),
			SYSCALL_ARG(int,       2, regs),
			SYSCALL_RETVAL(int,    regs)
		);
		break;
	case SYS_geteuid:
		ghost_fprintf(
			fp, "[ID %d]: geteuid() = %d\n",
			pid,
			SYSCALL_RETVAL(int,    regs)
		);
		break;
	case SYS_futex:
		ghost_fprintf(
			fp, "[ID %d]: futex(%p, %d, %d, %p, %p, %d) = %d\n",
			pid,
			SYSCALL_ARG(void*,     0, regs),
			SYSCALL_ARG(int,       1, regs),
			SYSCALL_ARG(int,       2, regs),
			SYSCALL_ARG(void*,     3, regs),
			SYSCALL_ARG(void*,     4, regs),
			SYSCALL_ARG(int,       5, regs),
			SYSCALL_RETVAL(int,    regs)
		);
		break;
	case SYS_openat:
		ghost_fprintf(
			fp, "[ID %d]: openat(%d, %s, %d, %d) = %d\n",
			pid,
			SYSCALL_ARG(int,       0, regs),
			SYSCALL_STR(p_buffer_1, PRINT_BUFFER_SIZE, 1, regs),
			SYSCALL_ARG(int,       2, regs),
			SYSCALL_ARG(int,       3, regs),
			SYSCALL_RETVAL(int,    regs)
		);
		break;
	default:
		ghost_fprintf(
			fp, "[ID %d]: syscall(%d, ...) = %lu\n",
			pid, syscall_no, SYSCALL_RETVAL(uint64_t, regs)
		);
	}
}
/*****************************************************************************/
static void* init(void *arg)
{
	return ghost_stderr;
}
/*****************************************************************************/
static void* handle(void *arg, const struct tracee_state *state)
{
	struct ghost_file *fp = arg;

	if(state->status == STARTED) {
		ghost_fprintf(fp, "[ID %d]: Started\n", state->pid);
	} else if(state->status == SYSCALL_ENTER_STOP) {
	} else if(state->status == SYSCALL_EXIT_STOP) {
		print_syscall(fp, state->pid, &state->data.regs);
	} else if(state->status == EXITED_NORMAL) {
		ghost_fprintf(
			fp,
			"[ID %d]: Exited: %d\n",
			state->pid,
			state->data.exit_status
		);
	} else if(state->status == PTRACE_EXEC_OCCURED) {
		ghost_fprintf(fp, "[ID %d]: Called exec\n", state->pid);
	} else if(state->status == EXITED_UNEXPECTED) {
		ghost_fprintf(fp, "[ID %d]: Unexpected exit\n", state->pid);
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
