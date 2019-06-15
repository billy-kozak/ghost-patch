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
#ifndef THREAD_JUMP_H
#define THREAD_JUMP_H
/******************************************************************************
*                                  INCLUDES                                   *
******************************************************************************/
#include "platform.h"
#include "syscall-utl.h"

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <asm/prctl.h>
#include <sys/prctl.h>
/******************************************************************************
*                                    TYPES                                    *
******************************************************************************/
struct thread_jump {
	volatile uint64_t flag;

	void *ret_addr;
	void *rsp;
	void *rbp;
	uint64_t rbx;
	uint64_t r12;
	uint64_t r13;
	uint64_t r14;
	uint64_t r15;
	unsigned long fs;
	unsigned long gs;
};
/******************************************************************************
*                                   DEFINES                                   *
******************************************************************************/
#define TJ_BUFFER_INITIAL {0}
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
int __tj_set(struct thread_jump *tj);
void __tj_jump(struct thread_jump *tj);
int arch_prctl(int code, ...);
/******************************************************************************
*                              INLINE FUNCTIONS                               *
******************************************************************************/
static inline void tj_spinwait(struct thread_jump *tj)
{
	while(tj->flag == 0);
}
/*****************************************************************************/
static inline ALWAYS_INLINE int tj_set(struct thread_jump *tj)
{
	arch_prctl(ARCH_GET_FS, &tj->fs);
	arch_prctl(ARCH_GET_GS, &tj->gs);
	return __tj_set(tj);
}
/*****************************************************************************/
static inline ALWAYS_INLINE void tj_jump(struct thread_jump *tj, int set_fs)
{
	if(set_fs) {
		arch_prctl(ARCH_SET_FS, tj->fs);
		arch_prctl(ARCH_SET_GS, tj->gs);
	}
	__tj_jump(tj);
}
/*****************************************************************************/
static inline ALWAYS_INLINE void tj_set_and_exit(struct thread_jump *tj)
{
	if(tj_set(tj) == 0) {
		syscall_exit(0);
	}
}
/*****************************************************************************/
static inline ALWAYS_INLINE void tj_swap(
	struct thread_jump *src, struct thread_jump *dst, int set_fs
) {
	if(tj_set(src) == 0) {
		tj_spinwait(dst);
		tj_jump(dst, set_fs);
	}
}
/*****************************************************************************/
#endif /* THREAD_JUMP_H */