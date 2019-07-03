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
#include "syscall-utl.h"

#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <asm/prctl.h>
#include <sys/prctl.h>
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
int arch_prctl(int code, ...);
/******************************************************************************
*                           FUNCTION DEFINITIONS                              *
******************************************************************************/
pid_t syscall_gettid(void)
{
	return (pid_t)syscall(SYS_gettid);
}
/*****************************************************************************/
void syscall_exit(int code)
{
	syscall(SYS_exit,code);
}
/*****************************************************************************/
pid_t syscall_getpid(void)
{
	return 	(pid_t)syscall(SYS_getpid);
}
/*****************************************************************************/
unsigned long arch_prctl_get_fs_nocheck(void)
{
	unsigned long fs;
	arch_prctl(ARCH_GET_FS, &fs);

	return fs;
}
/*****************************************************************************/
int arch_prctl_get_fs(unsigned long *fs)
{
	return arch_prctl(ARCH_GET_FS, fs);
}
/*****************************************************************************/
int arch_prctl_set_fs(unsigned long fs)
{
	return arch_prctl(ARCH_SET_FS, fs);
}
/*****************************************************************************/
int arch_prctl_get_gs(unsigned long *gs)
{
	return arch_prctl(ARCH_GET_GS, gs);
}
/*****************************************************************************/
int arch_prctl_set_gs(unsigned long gs)
{
	return arch_prctl(ARCH_SET_GS, gs);
}
/*****************************************************************************/
