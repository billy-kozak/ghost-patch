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
#include "pseudo-strace.h"

#include "trace.h"

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

#define CHAR_ARR_STRLEN(s) (sizeof(s) - 1)
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
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
static void* init(void *arg);
static void* handle(void *argg, const struct tracee_state *state);
static void print_syscall(
	FILE *fp, pid_t pid, const struct user_regs_struct *regs
);
static uint64_t syscall_retval(const struct user_regs_struct *regs);
static uint64_t syscall_arg(int n, const struct user_regs_struct *regs);
static int repr_byte(char *str, char byte, ssize_t *space_size);
static char octal_char(int val, int n);
static char *sprint_buffer(
	const char *buffer,
	char *space,
	ssize_t buffer_size,
	ssize_t space_size
);
static char *sprint_flags(
	char *str, ssize_t size, const struct named_flag *names, int flag
);
/******************************************************************************
*                              STATIC FUNCTIONS                               *
******************************************************************************/
static char *sprint_flags(
	char *str, ssize_t size, const struct named_flag *names, int flag
) {
	const char continuation[] = "|...";
	char *p = str;

	size -= sizeof(continuation);

	if(size < 0) {
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
static char octal_char(int val, int n)
{
	return ((val >> (3 * n)) & 0x7) + '0';
}
/*****************************************************************************/
static int repr_byte(char *str, char byte, ssize_t *space_size)
{
	if((byte == '"') || (byte == '\\')) {
		if(*space_size < 2) {
			return 0;
		} else {
			str[0] = '\\';
			str[1] = byte;
			*space_size -= 2;
			return 2;
		}
	} else if(byte == '\n') {
		if(*space_size < 2) {
			return 0;
		} else {
			str[0] = '\\';
			str[1] = 'n';
			*space_size -= 2;
			return 2;
		}
	} else if(isprint(byte) || (byte == '\t')) {
		if(*space_size == 0) {
			return 0;
		} else {
			str[0] = byte;
			*space_size -= 1;
			return 1;
		}
	} else {
		if(*space_size < 4) {
			return 0;
		} else {
			str[0] = '\\';
			str[1] = octal_char(byte, 2);
			str[2] = octal_char(byte, 1);
			str[3] = octal_char(byte, 0);
			*space_size -= 4;
			return 4;
		}
	}
}
/*****************************************************************************/
static char *sprint_buffer(
	const char *buffer,
	char *str,
	ssize_t buffer_size,
	ssize_t space_size
) {
	int len = 0;
	char border = '"';
	const char continuation[] = "\"...";

	space_size -= sizeof(border) + CHAR_ARR_STRLEN(continuation) + 1;

	if(space_size < 0) {
		return NULL;
	}

	str[len] = border;
	len += 1;

	for(size_t i = 0; i < buffer_size; i++) {
		int s = 0;
		char c = buffer[i];

		if((s = repr_byte(str + len, c, &space_size)) == 0) {
			memcpy(str + len, continuation, sizeof(continuation));
			return str;
		}
		len += s;
	}

	str[len] = border;
	str[len + 1] = '\0';

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
		return 0;
	}
}
/*****************************************************************************/
static void print_syscall(
	FILE *fp, pid_t pid, const struct user_regs_struct *regs
) {
	char p_buffer[PRINT_BUFFER_SIZE];
	int syscall_no = regs->orig_rax;

	switch(syscall_no) {
	case SYS_read:
		fprintf(
			fp, "[ID %d]: read(%d, %p, %ld) = %d\n",
			pid,
			SYSCALL_ARG(int,     0, regs),
			SYSCALL_ARG(void*,   1, regs),
			SYSCALL_ARG(int64_t, 2, regs),
			SYSCALL_RETVAL(int, regs)
		);
		break;
	case SYS_write:
		fprintf(
			fp, "[ID %d]: write(%d, %s, %ld) = %d\n",
			pid,
			SYSCALL_ARG(int,     0, regs),
			SYSCALL_BUF(p_buffer, PRINT_BUFFER_SIZE, 1, 2, regs),
			SYSCALL_ARG(int64_t, 2, regs),
			SYSCALL_RETVAL(int, regs)
		);
		break;
	case SYS_open:
		fprintf(
			fp, "[ID %d]: open(%s, %d, %ld) = %d\n",
			pid,
			SYSCALL_STR(p_buffer, PRINT_BUFFER_SIZE, 0, regs),
			SYSCALL_ARG(int,     1, regs),
			SYSCALL_ARG(int64_t, 2, regs),
			SYSCALL_RETVAL(int, regs)
		);
		break;
	case SYS_close:
		fprintf(
			fp, "[ID %d]: close(%d) = %d\n",
			pid,
			SYSCALL_ARG(int,     0, regs),
			SYSCALL_RETVAL(int, regs)
		);
		break;
	case SYS_fstat:
		fprintf(
			fp, "[ID %d]: fstat(%d, %p) = %d\n",
			pid,
			SYSCALL_ARG(int,     0, regs),
			SYSCALL_ARG(void*,   1, regs),
			SYSCALL_RETVAL(int, regs)
		);
		break;
	case SYS_mmap:
		fprintf(
			fp, "[ID %d]: mmap(%p, %ld, %d, %s, %d, %lu) = %p\n",
			pid,
			SYSCALL_ARG(void*,    0, regs),
			SYSCALL_ARG(int64_t,  1, regs),
			SYSCALL_ARG(int,      2, regs),
			SYSCALL_FLAG(
				p_buffer, PRINT_BUFFER_SIZE,
				MMAP_FLAGS, 3, regs
			),
			SYSCALL_ARG(int,      4, regs),
			SYSCALL_ARG(uint64_t, 5, regs),
			SYSCALL_RETVAL(void*,    regs)
		);
		break;
	case SYS_munmap:
		fprintf(
			fp, "[ID %d]: munmap(%p, %lu) = %d\n",
			pid,
			SYSCALL_ARG(void*,    0, regs),
			SYSCALL_ARG(uint64_t, 1, regs),
			SYSCALL_RETVAL(int,      regs)
		);
		break;
	case SYS_rt_sigaction:
		fprintf(
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
		fprintf(
			fp, "[ID %d]: ioctl(%d, %lu, %p) = %d\n",
			pid,
			SYSCALL_ARG(int,       0, regs),
			SYSCALL_ARG(uint64_t,  1, regs),
			SYSCALL_ARG(void*,     2, regs),
			SYSCALL_RETVAL(int,    regs)
		);
		break;
	case SYS_getpid:
		fprintf(
			fp, "[ID %d]: getpid() = %d\n",
			pid,
			SYSCALL_RETVAL(int,    regs)
		);
		break;
	case SYS_clone:
		fprintf(
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
		fprintf(
			fp, "[ID %d]: getdents(%d, %p, %d) = %d\n",
			pid,
			SYSCALL_ARG(int,       0, regs),
			SYSCALL_ARG(void*,     1, regs),
			SYSCALL_ARG(int,       2, regs),
			SYSCALL_RETVAL(int,    regs)
		);
		break;
	case SYS_futex:
		fprintf(
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
		fprintf(
			fp, "[ID %d]: openat(%d, %s, %d, %d) = %d\n",
			pid,
			SYSCALL_ARG(int,       0, regs),
			SYSCALL_STR(p_buffer, PRINT_BUFFER_SIZE, 1, regs),
			SYSCALL_ARG(int,       2, regs),
			SYSCALL_ARG(int,       3, regs),
			SYSCALL_RETVAL(int,    regs)
		);
		break;
	default:
		fprintf(
			fp, "[ID %d]: syscall(%d, ...) = %lu\n",
			pid, syscall_no, SYSCALL_RETVAL(uint64_t, regs)
		);
	}
}
/*****************************************************************************/
static void* init(void *arg)
{
	return fopen("/dev/stderr", "w");
}
/*****************************************************************************/
static void* handle(void *arg, const struct tracee_state *state)
{
	if(state->status == STARTED) {
		printf("[ID %d]: Started\n", state->pid);
	} else if(state->status == SYSCALL_ENTER_STOP) {

	} else if(state->status == SYSCALL_EXIT_STOP) {
		print_syscall(arg, state->pid, &state->data.regs);
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