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
#ifndef SYSCALL_UTL_H
#define SYSCALL_UTL_H
/******************************************************************************
*                                  INCLUDES                                   *
******************************************************************************/
#include <sys/types.h>
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
void syscall_exit(int code);
pid_t syscall_gettid(void);
int arch_prctl_get_fs(unsigned long *fs);
int arch_prctl_set_fs(unsigned long fs);
int arch_prctl_get_gs(unsigned long *gs);
int arch_prctl_set_gs(unsigned long gs);
/*****************************************************************************/
#endif /* SYSCALL_UTL_H */