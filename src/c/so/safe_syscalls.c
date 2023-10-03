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
*                                  INCLUDES                                   *
******************************************************************************/
#include "safe_syscalls.h"

#include <sys/syscall.h>
#include <sys/ioctl.h>
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
int safe_isatty(int fd)
{
	struct winsize wsz;

	union _typ_pun ret;
	union _typ_pun a0 = {.i64 = fd};
	union _typ_pun a1 = {.u64 = TIOCGWINSZ};
	union _typ_pun a2 = {.p = &wsz};

	ret.u64 = _syscall3(SYS_ioctl, a0.i64, a1.i64, a2.i64);

	return (ret.u64 == 0) ? 1 : 0;
}
/*****************************************************************************/
