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
#include "get-options.h"

#include <utl/str-utl.h>
#include <env.h>

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
/******************************************************************************
*                                    DATA                                     *
******************************************************************************/
static struct prog_opts cached_opts = DEFAULT_PROG_ARGS;
static char lua_ent_opt[PATH_MAX + 1];
/******************************************************************************
*                            FUNCTION DEFINITIONS                             *
******************************************************************************/
int get_options(struct prog_opts *opts)
{
	const char *env_str;
	const char *sptr;

	env_str = ghost_getenv(OPTION_ENV_VAR);
	sptr = env_str;

	if(env_str == NULL) {
		memcpy(opts, &cached_opts, sizeof(cached_opts));
		return -1;
	}

	memset(opts, 0, sizeof(*opts));

	size_t flen = 0;

	while(*sptr != '\0') {
		if(strdcmp(sptr, FAKE_PID_FIELD, '=') == 0) {
			sptr += strlen(FAKE_PID_FIELD) + 1;

			if(strdcmp(sptr, "true", ';') == 0) {
				opts->fake_pid = true;
				sptr += sizeof("true");
			} else if(strdcmp(sptr, "false", ';') == 0) {
				opts->fake_pid = false;
				sptr += sizeof("false");
			} else {
				return -1;
			}
		} else if(strdcmp(sptr, LUA_ENT_FIELD, '=') == 0) {
			sptr += strlen(LUA_ENT_FIELD) + 1;
			flen = strdcpy(lua_ent_opt, sptr, ';', PATH_MAX + 1);

			if(sptr[flen] != ';') {
				return -1;
			}
			opts->lua_ent = lua_ent_opt;
			sptr += flen + 1;
		} else {
			return -1;
		}
	}

	return 0;
}
/*****************************************************************************/
