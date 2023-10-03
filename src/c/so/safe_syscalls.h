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
#ifndef SAFE_SYSCALLS_H
#define SAFE_SYSCALLS_H
/******************************************************************************
*                                  INCLUDES                                   *
******************************************************************************/
#include <stdint.h>
#include <stdlib.h>
#include <sys/syscall.h>
/******************************************************************************
*                                    TYPES                                    *
******************************************************************************/
union _typ_pun{
	void *p;
	int64_t i64;
	uint64_t u64;
};
/******************************************************************************
*                              STATIC FUNCTIONS                               *
******************************************************************************/
static inline uint64_t _syscall0(uint64_t call)
{
	uint64_t ret;
	register long rax asm("rax") = call;

	asm volatile (
		"syscall": "=a"(ret) : "r"(rax): "rcx", "r11", "memory"
	);
	return ret;
}
/*****************************************************************************/
static inline uint64_t _syscall1(uint64_t call, int64_t a0)
{
	uint64_t ret;

	register long rax asm("rax") = call;
	register long rdi asm("rdi") = a0;

	asm volatile (
		"syscall":
		"=a"(ret) :
		"r"(rax), "r"(rdi):
		"rcx", "r11", "memory"
	);
	return ret;
}
/*****************************************************************************/
static inline uint64_t _syscall2(uint64_t call, int64_t a0, int64_t a1)
{
	uint64_t ret;

	register long rax asm("rax") = call;
	register long rdi asm("rdi") = a0;
	register long rsi asm("rsi") = a1;

	asm volatile (
		"syscall":
		"=a"(ret) :
		"r"(rax), "r"(rdi), "r"(rsi):
		"rcx", "r11", "memory"
	);
	return ret;
}
/*****************************************************************************/
static inline uint64_t _syscall3(
	uint64_t call,
	int64_t a0,
	int64_t a1,
	int64_t a2
) {
	uint64_t ret;

	register long rax asm("rax") = call;
	register long rdi asm("rdi") = a0;
	register long rsi asm("rsi") = a1;
	register long rdx asm("rdx") = a2;

	asm volatile (
		"syscall":
		"=a"(ret) :
		"r"(rax), "r"(rdi), "r"(rsi), "r"(rdx):
		"rcx", "r11", "memory"
	);
	return ret;
}
/*****************************************************************************/
static inline uint64_t _syscall4(
	uint64_t call,
	int64_t a0,
	int64_t a1,
	int64_t a2,
	int64_t a3
) {
	uint64_t ret;

	register long rax asm("rax") = call;
	register long rdi asm("rdi") = a0;
	register long rsi asm("rsi") = a1;
	register long rdx asm("rdx") = a2;
	register long r10 asm("r10") = a3;

	asm volatile (
		"syscall":
		"=a"(ret) :
		"r"(rax), "r"(rdi), "r"(rsi), "r"(rdx), "r"(r10):
		"rcx", "r11", "memory"
	);
	return ret;
}
/*****************************************************************************/
static inline uint64_t _syscall5(
	uint64_t call,
	int64_t a0,
	int64_t a1,
	int64_t a2,
	int64_t a3,
	int64_t a4
) {
	uint64_t ret;

	register long rax asm("rax") = call;
	register long rdi asm("rdi") = a0;
	register long rsi asm("rsi") = a1;
	register long rdx asm("rdx") = a2;
	register long r10 asm("r10") = a3;
	register long r8 asm("r8") = a4;

	asm volatile (
		"syscall":
		"=a"(ret) :
		"r"(rax), "r"(rdi), "r"(rsi), "r"(rdx), "r"(r10), "r"(r8):
		"rcx", "r11", "memory"
	);
	return ret;
}
/*****************************************************************************/
static inline uint64_t _syscall6(
	uint64_t call,
	int64_t a0,
	int64_t a1,
	int64_t a2,
	int64_t a3,
	int64_t a4,
	int64_t a5
) {
	uint64_t ret;

	register long rax asm("rax") = call;
	register long rdi asm("rdi") = a0;
	register long rsi asm("rsi") = a1;
	register long rdx asm("rdx") = a2;
	register long r10 asm("r10") = a3;
	register long r8 asm("r8") = a4;
	register long r9 asm("r9") = a5;

	asm volatile (
		"syscall":
		"=a"(ret) :
			"r"(rax), "r"(rdi), "r"(rsi), "r"(rdx),
			"r"(r10), "r"(r8), "r"(r9):
		"rcx", "r11", "memory"
	);
	return ret;
}
/*****************************************************************************/
static inline void *safe_mmap(
	void *addr, size_t length, int prot, int flags, int fd, off_t offset
) {
	union _typ_pun ret;
	union _typ_pun a0 = {.p = addr};
	union _typ_pun a1 = {.u64 = length};
	union _typ_pun a2 = {.i64 = prot};
	union _typ_pun a3 = {.i64 = flags};
	union _typ_pun a4 = {.i64 = fd};
	union _typ_pun a5 = {.i64 = offset};

	ret.u64 = _syscall6(
		SYS_mmap,
		a0.i64,
		a1.i64,
		a2.i64,
		a3.i64,
		a4.i64,
		a5.i64
	);

	return ret.p;
}
/*****************************************************************************/
static inline int safe_munmap(void *addr, size_t len)
{
	union _typ_pun ret;
	union _typ_pun a0 = {.p = addr};
	union _typ_pun a1 = {.u64 = len};

	ret.i64 = _syscall2(SYS_munmap, a0.i64, a1.i64);

	return (int)ret.i64;
}
/*****************************************************************************/
static inline int safe_kill(pid_t pid, int sig)
{
	union _typ_pun ret;
	union _typ_pun a0 = {.u64 = pid};
	union _typ_pun a1 = {.i64 = sig};

	ret.i64 = _syscall2(SYS_kill, a0.i64, a1.i64);

	return (int)ret.i64;
}
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
int safe_isatty(int fd);
/*****************************************************************************/
#endif /* SAFE_SYSCALLS_H */
