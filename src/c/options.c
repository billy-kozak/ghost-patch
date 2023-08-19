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
#include "options.h"

#include <utl/str-utl.h>

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
/******************************************************************************
*                                  CONSTANTS                                  *
******************************************************************************/
static const char OPTION_ENV_VAR[] = "GHOST_PATCH_OPTS";
static const char FAKE_PID_FIELD[] = "fake_pid";
/******************************************************************************
*                                    DATA                                     *
******************************************************************************/
static bool options_loaded = false;
static struct prog_opts cached_opts = DEFAULT_PROG_ARGS;
/******************************************************************************
*                            FUNCTION DEFINITIONS                             *
******************************************************************************/
int set_options(const struct prog_opts *opts)
{
	int ret = 0;
	char *env_str = NULL;

	env_str = concatenate_strings(
		FAKE_PID_FIELD,
		"=",
		bool_to_string(opts->fake_pid),
		";"
	);

	if(env_str == NULL) {
		ret = -1;
		goto exit;
	}

	if(setenv(OPTION_ENV_VAR, env_str, 1)) {
		ret = -1;
		goto exit;
	}

	memcpy(&cached_opts, opts, sizeof(cached_opts));

	options_loaded = true;
exit:
	free(env_str);
	return ret;
}
/*****************************************************************************/
int get_options(struct prog_opts *opts)
{
	const char *env_str;
	const char *sptr;

	if(options_loaded) {
		memcpy(opts, &cached_opts, sizeof(cached_opts));
		return 0;
	}

	env_str = getenv(OPTION_ENV_VAR);
	sptr = env_str;

	if(env_str == NULL) {
		memcpy(opts, &cached_opts, sizeof(cached_opts));
		return -1;
	}

	while(*sptr != '\0') {
		if(strdcmp(sptr, FAKE_PID_FIELD, '=') == 0) {
			sptr += sizeof(FAKE_PID_FIELD);

			if(strdcmp(sptr, "true", ';') == 0) {
				opts->fake_pid = true;
				sptr += sizeof("true");
			} else if(strdcmp(sptr, "false", ';') == 0) {
				opts->fake_pid = false;
				sptr += sizeof("false");
			} else {
				return -1;
			}
		} else {
			return -1;
		}
	}

	return 0;
}
/*****************************************************************************/
