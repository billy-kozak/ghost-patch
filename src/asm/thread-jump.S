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
*                                TEXT SECTION                                 *
******************************************************************************/
.section .text
	.global __tj_set
	.global __tj_jump
/******************************************************************************
*                            FUNCTION DEFINITIONS                             *
******************************************************************************/
__tj_set:
	// save the return address
	mov (%rsp), %rax
	mov %rax,  (8)(%rdi)

	//save the stack pointer, and other registers
	mov %rsp, (16)(%rdi)
	mov %rbp, (24)(%rdi)
	mov %rbx, (32)(%rdi)
	mov %r12, (40)(%rdi)
	mov %r13, (48)(%rdi)
	mov %r14, (56)(%rdi)
	mov %r15, (64)(%rdi)

	// Raise the flag to indicate that the set is now complete
	mfence
	movq $1, (%rdi)

	mov $0,   %rax
	ret
/*****************************************************************************/
__tj_jump:

	// Restore registers
	mov (16)(%rdi), %rsp
	mov (24)(%rdi), %rbp
	mov (32)(%rdi), %rbx
	mov (40)(%rdi), %r12
	mov (48)(%rdi), %r13
	mov (56)(%rdi), %r14
	mov (64)(%rdi), %r15

	//Restore the return address
	mov (8)(%rdi), %rax
	mov %rax,      (%rsp)

	mov $1,   %rax
	ret
/*****************************************************************************/