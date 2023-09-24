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
#include "set-options.h"

#include <options.h>
#include <str-utl-libc.h>
#include <utl/str-utl.h>

#include <string.h>
/******************************************************************************
*                                    DATA                                     *
******************************************************************************/
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

	if(opts->lua_ent != NULL) {
		char *tmp = append_to_dyn_str(
			NULL,
			env_str,
			LUA_ENT_FIELD,
			"=",
			opts->lua_ent,
			";"
		);
		if(tmp == NULL) {
			ret = -1;
			goto exit;
		}
		env_str = tmp;
	}

	if(setenv(OPTION_ENV_VAR, env_str, 1)) {
		ret = -1;
		goto exit;
	}

	memcpy(&cached_opts, opts, sizeof(cached_opts));
exit:
	free(env_str);
	return ret;
}
/*****************************************************************************/
