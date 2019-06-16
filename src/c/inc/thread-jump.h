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
#include <assert.h>
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
int __tj_swap(struct thread_jump *src, struct thread_jump *dst, int set_fs);
int __tj_jump(struct thread_jump *src, struct thread_jump *dst, int set_fs);
/******************************************************************************
*                              INLINE FUNCTIONS                               *
******************************************************************************/
static inline void tj_spinwait(struct thread_jump *tj)
{
	while(tj->flag == 0);
}
/*****************************************************************************/
static inline ALWAYS_INLINE void tj_jump(struct thread_jump *tj, int set_fs)
{
	__tj_jump(NULL, tj, set_fs);
}
/*****************************************************************************/
static inline ALWAYS_INLINE void tj_set_and_exit(struct thread_jump *tj)
{
	arch_prctl_get_fs(&tj->fs);
	arch_prctl_get_gs(&tj->gs);

	if(__tj_swap(tj, NULL, 0)) {
		arch_prctl_set_fs(tj->fs);
		arch_prctl_set_gs(tj->gs);
	}
}
/*****************************************************************************/
static inline ALWAYS_INLINE void tj_swap(
	struct thread_jump *src, struct thread_jump *dst, int set_fs
) {
	arch_prctl_get_fs(&src->fs);
	arch_prctl_get_gs(&src->gs);

	if(__tj_swap(src, dst, set_fs)) {
		arch_prctl_set_fs(src->fs);
		arch_prctl_set_gs(src->gs);
	}
}
/*****************************************************************************/
#endif /* THREAD_JUMP_H */