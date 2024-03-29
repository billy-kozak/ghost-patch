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
/******************************************************************************
*                                TEXT SECTION                                 *
******************************************************************************/
.section .text
	.global __tj_swap
	.global __tj_jump
/******************************************************************************
*                            FUNCTION DEFINITIONS                             *
******************************************************************************/
exit:
	mov $60, %rax
	xor %rdi, %rdi
	syscall
__tj_swap:
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

	cmp $0, %rsi
	je exit

__tj_jump:

	mov (%rsi), %rax
	cmp $0, %rax
	je __tj_jump

	// Restore registers
	mov (16)(%rsi), %rsp
	mov (24)(%rsi), %rbp
	mov (32)(%rsi), %rbx
	mov (40)(%rsi), %r12
	mov (48)(%rsi), %r13
	mov (56)(%rsi), %r14
	mov (64)(%rsi), %r15

	//Restore the return address
	mov (8)(%rsi), %rax
	mov %rax,      (%rsp)

	mov %rdx, %rax
	ret
/*****************************************************************************/