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
#define _GNU_SOURCE
/******************************************************************************
*                                  INCLUDES                                   *
******************************************************************************/
#include "tracee-state-table.h"

#include "secret-heap.h"
#include <gio/ghost-stdio.h>

#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
/******************************************************************************
*                                   MACROS                                    *
******************************************************************************/
#define TABLE(t) ((uint8_t*)t)
/******************************************************************************
*                                  CONSTANTS                                  *
******************************************************************************/
const size_t READ_BUFFER_SIZE = 32;
/******************************************************************************
*                                    DATA                                     *
******************************************************************************/
static int max_threads;
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
static int compute_max_threads(void);
/******************************************************************************
*                              STATIC FUNCTIONS                               *
******************************************************************************/
static int compute_max_threads(void)
{
	/* avoid using stdio when we are inside the memory space of another
	process */

	int fd = open("/proc/sys/kernel/pid_max", O_RDONLY);
	char str[READ_BUFFER_SIZE];
	char *endptr = NULL;
	int ret = -1;
	unsigned long converted = 0;

	ssize_t len = 0;
	ssize_t count = 0;

	if(fd < 0) {
		goto exit;
	}

	while((count = read(fd, str + len, READ_BUFFER_SIZE - len)) != 0) {
		if(count == -1) {
			goto exit;
		}

		len += count;
	}

	if(len == READ_BUFFER_SIZE) {
		errno = ENOMEM;
		goto exit;
	}

	str[len - 1] = '\0';

	errno = 0;

	converted = strtoul(str, &endptr, 10);

	if(errno != 0) {
		goto exit;
	}
	if(endptr != (str + len - 1)) {
		errno = EINVAL;
		goto exit;
	}
	if(converted > INT_MAX) {
		errno = EINVAL;
		goto exit;
	}

	ret = (int)converted;

exit:
	if(fd >= 0) {
		close(fd);
	}
	return ret;
}
/******************************************************************************
*                            FUNCTION DEFINITIONS                             *
******************************************************************************/
uint8_t tracee_state_table_retrieve(const void *t, pid_t id)
{
	return TABLE(t)[id];
}
/*****************************************************************************/
int tracee_state_table_store(void *t, pid_t id, uint8_t state)
{
	if(id > max_threads) {
		return -1;
	} else {
		TABLE(t)[id] = state;
		return 0;
	}
}
/*****************************************************************************/
void tracee_state_table_destroy(void *table)
{
	ghost_free(sheap, table);
}
/*****************************************************************************/
void *tracee_state_table_init(void)
{
	void *ret = NULL;

	if(max_threads == 0) {
		max_threads = compute_max_threads();

		if(max_threads < 0) {
			return NULL;
		}
	}

	/* avoid calling malloc when we are operating within the memory
	space of another process */
	ret = ghost_malloc(sheap, max_threads);

	if(ret != NULL) {
		memset(ret, -1, max_threads);
	}
	return ret;
}
/*****************************************************************************/
