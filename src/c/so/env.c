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
#include "env.h"

#include <utl/str-utl.h>

#include <stdlib.h>
#include <assert.h>
/******************************************************************************
*                                    DATA                                     *
******************************************************************************/
const char *const *ghost_envp;
/******************************************************************************
*                              STATIC FUNCTIONS                               *
******************************************************************************/
static const char *env_cmp(const char *env, const char *var)
{
	size_t idx = 0;

	while(env[idx] != '=') {
		if(env[idx] == '\0') {
			/* environment is not setup correctly */
			assert(false);
			return NULL;
		} else if(var[idx] == '\0') {
			return NULL;
		} else if(env[idx] != var[idx]) {
			return NULL;
		}
		idx += 1;
	}

	if(var[idx] != '\0') {
		return NULL;
	}

	return env + idx + 1;
}
/******************************************************************************
*                            FUNCTION DEFINITIONS                             *
******************************************************************************/
const char *ghost_getenv(const char *var)
{
	for(size_t i = 0; ghost_envp[i] != NULL; i++) {
		const char *val = env_cmp(ghost_envp[i], var);
		if(val != NULL) {
			return val;
		}
	}
	return NULL;
}
/*****************************************************************************/
void ghost_env_init(char **envp)
{
	ghost_envp = (const char *const *)envp;
}
/*****************************************************************************/
