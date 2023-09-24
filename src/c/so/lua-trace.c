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
#include "lua-trace.h"

#include <trace.h>
#include <secret-heap.h>
#include <assert.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>
/******************************************************************************
*                                    TYPES                                    *
******************************************************************************/
struct lua_trace_data {
	lua_State *ls;
	const char *ent;
};
/******************************************************************************
*                                    DATA                                     *
******************************************************************************/
static struct lua_trace_data trace_data;
/******************************************************************************
*                              STATIC FUNCTIONS                               *
******************************************************************************/
static void *alloc_f(void *ud, void *ptr, size_t osize, size_t nsize)
{
	struct ghost_heap *heap = ud;

	if(nsize == 0) {
		ghost_free(heap, ptr);
		return NULL;
	} else {
		return ghost_realloc(heap, ptr, nsize);
	}
}
/*****************************************************************************/
static void *handler(void *arg, const struct tracee_state *state)
{
	return arg;
}
/*****************************************************************************/
static void *handler_init(void *arg)
{
	int err;
	lua_State *ls = lua_newstate(alloc_f, sheap);
	trace_data.ls = ls;

	assert(trace_data.ls != NULL);

	luaL_openlibs(ls);
	err = luaL_loadfile(ls, trace_data.ent);

	if(err != LUA_OK) {
		if(err == LUA_ERRFILE) {
			ghost_fprintf(
				ghost_stderr,
				"Error opening file: %s\n",
				trace_data.ent
			);
		} else {
			ghost_fprintf(
				ghost_stderr,
				"Error loading lua entry point: %d\n",
				err
			);
		}
		abort();
		return NULL;
	}

	err = lua_pcall(ls, 0, 0, 0);
	if(err != LUA_OK) {
		ghost_fprintf(ghost_stderr, "Error running lua entry point\n");
		abort();
		return NULL;
	}

	return arg;
}
/******************************************************************************
*                            FUNCTION DEFINITIONS                             *
******************************************************************************/
struct trace_descriptor lua_trace_descriptor(const char *ent)
{
	struct trace_descriptor descr;

	descr.init = handler_init;
	descr.handle = handler;
	descr.arg = &trace_data;

	trace_data.ent = ent;
	trace_data.ls = NULL;

	return descr;
}
/*****************************************************************************/
