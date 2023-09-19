/******************************************************************************
* Copyright (C) 2023  Billy Kozak                                             *
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
#include <utl/random-utl.h>

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
/******************************************************************************
*                                  CONSTANTS                                  *
******************************************************************************/
static const char URANDOM_PATH[] = "/dev/urandom";

static const char ALPHA_NUM[] =
	"0123456789"
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz";
/******************************************************************************
*                            FUNCTION DEFINITIONS                             *
******************************************************************************/
void random_utl_rand_alpha_num(
	struct drand48_data *restrict data,
	char *restrict str,
	size_t len
) {
	for(int i = 0; i < len; i++) {
		long t;
		lrand48_r(data, &t);
		unsigned long r = (unsigned long)t;

		str[i] = ALPHA_NUM[r % (sizeof(ALPHA_NUM) - 1)];
	}
}
/*****************************************************************************/
int random_utl_seed_from_urandom(struct drand48_data *data)
{
	int fd = open(URANDOM_PATH, O_RDONLY);
	int ret = -1;

	if(fd < 0) {
		goto exit;
	}

	char buf[sizeof(long int)];

	int e = read(fd, buf, sizeof(long int));
	if(e != sizeof(long int)) {
		goto exit;
	}

	long int seed = *((long int*)buf);
	srand48_r(seed, data);

	ret = 0;
exit:
	if(fd >= 0) {
		close(fd);
	}
	return ret;
}
/*****************************************************************************/
int random_utl_seed_from_clock(struct drand48_data *data)
{
	time_t seed = time(NULL);

	srand48_r(seed, data);
	return 0;
}
/*****************************************************************************/
