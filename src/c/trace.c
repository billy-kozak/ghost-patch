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
#define _GNU_SOURCE
/******************************************************************************
*                                  INCLUDES                                   *
******************************************************************************/
#include "trace.h"

#include "fake-pthread.h"

#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
static int monitor_thread(void* arg);
/******************************************************************************
*                              STATIC FUNCTIONS                               *
******************************************************************************/
static int monitor_thread(void* arg)
{
	printf("Monitor thread online\n");
	return 0;
}
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
int start_trace(void)
{
	return fake_pthread(monitor_thread, NULL);
}
/*****************************************************************************/