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
#include "lua/lua.h"

#include <trace.h>
#include <secret-heap.h>
#include <assert.h>
#include <gio/ghost-stdio.h>

#include <lua/lualib.h>
#include <lua/lauxlib.h>
/******************************************************************************
*                                    TYPES                                    *
******************************************************************************/
struct lua_trace_data {
	lua_State *ls;
	const char *ent;
	int lua_cb_ref;
};
/******************************************************************************
*                                  CONSTANTS                                  *
******************************************************************************/
const char LUA_TRACE_INIT_F[] = "LT_init";
/******************************************************************************
*                                    DATA                                     *
******************************************************************************/
static struct lua_trace_data trace_data;
/******************************************************************************
*                              STATIC FUNCTIONS                               *
******************************************************************************/
static int luaf_lua_trace_init(lua_State *ls)
{
	int stack_size = lua_gettop(ls);
	char *err = NULL;

	if(stack_size != 1) {
		ghost_sdprintf(
			&err,
			0,
			(
				"lua_trace_init: Wrong number of arguments. "
				"Expected 1, recieved %d."
			),
			stack_size
		);
		lua_pushstring(ls, err);
		lua_error(ls);

		goto exit;
	}


	if(!lua_isfunction(ls, 1)) {
		ghost_sdprintf(
			&err,
			0,
			(
				"lua_trace_init: Argument type error, "
				"required function, recieved %s."
			),
			lua_typename(ls, 1)
		);
		lua_pushstring(ls, err);
		lua_error(ls);

		goto exit;
	}

	int method_ref = luaL_ref(ls, LUA_REGISTRYINDEX);
	trace_data.lua_cb_ref = method_ref;

exit:
	ghost_free(sheap, err);
	return 0;
}
/*****************************************************************************/
static void define_global_int(struct lua_State *ls, const char *name, int val)
{
	lua_pushinteger(ls, val);
	lua_setglobal(ls, name);
}
/*****************************************************************************/
static void insert_int64_to_table(
	struct lua_State *ls, int tab_idx, const char *field, int64_t val
) {
	lua_pushinteger(ls, val);
	lua_setfield(ls, tab_idx, field);
}
/*****************************************************************************/
static void push_lua_uregs(
	struct lua_State *ls, const struct user_regs_struct *uregs
) {
	lua_newtable(ls);
	int i = lua_gettop(ls);

	insert_int64_to_table(ls, i, "r15", uregs->r15);
	insert_int64_to_table(ls, i, "r15", uregs->r15);
	insert_int64_to_table(ls, i, "r14", uregs->r14);
	insert_int64_to_table(ls, i, "r13", uregs->r13);
	insert_int64_to_table(ls, i, "r12", uregs->r12);
	insert_int64_to_table(ls, i, "rbp", uregs->rbp);
	insert_int64_to_table(ls, i, "rbx", uregs->rbx);
	insert_int64_to_table(ls, i, "r11", uregs->r11);
	insert_int64_to_table(ls, i, "r10", uregs->r10);
	insert_int64_to_table(ls, i, "r9", uregs->r9);
	insert_int64_to_table(ls, i, "r8", uregs->r8);
	insert_int64_to_table(ls, i, "rax", uregs->rax);
	insert_int64_to_table(ls, i, "rcx", uregs->rcx);
	insert_int64_to_table(ls, i, "rdx", uregs->rdx);
	insert_int64_to_table(ls, i, "rsi", uregs->rsi);
	insert_int64_to_table(ls, i, "rdi", uregs->rdi);
	insert_int64_to_table(ls, i, "orig_rax", uregs->orig_rax);
	insert_int64_to_table(ls, i, "rip", uregs->rip);
	insert_int64_to_table(ls, i, "cs", uregs->cs);
	insert_int64_to_table(ls, i, "eflags", uregs->eflags);
	insert_int64_to_table(ls, i, "rsp", uregs->rsp);
	insert_int64_to_table(ls, i, "ss", uregs->ss);
	insert_int64_to_table(ls, i, "fs_base", uregs->fs_base);
	insert_int64_to_table(ls, i, "gs_base", uregs->gs_base);
	insert_int64_to_table(ls, i, "ds", uregs->ds);
	insert_int64_to_table(ls, i, "es", uregs->es);
	insert_int64_to_table(ls, i, "fs", uregs->fs);
	insert_int64_to_table(ls, i, "gs", uregs->gs);
}
/*****************************************************************************/
static void setup_lua_runtime(const struct lua_trace_data *dat)
{
	struct lua_State *ls = dat->ls;

	luaL_openlibs(ls);
	lua_register(ls, LUA_TRACE_INIT_F, luaf_lua_trace_init);

	define_global_int(ls, "LT_STARTED", STARTED);
	define_global_int(ls, "LT_EXIT_NORMAL", EXITED_NORMAL);
	define_global_int(ls, "LT_EXIT_UNEXPECT", EXITED_UNEXPECTED);
	define_global_int(ls, "LT_SYSCALL_ENTER", SYSCALL_ENTER_STOP);
	define_global_int(ls, "LT_SYSCALL_EXIT", SYSCALL_EXIT_STOP);
	define_global_int(ls, "LT_SIG_DELIVER", SIGNAL_DELIVERY_STOP);
	define_global_int(ls, "LT_GROUP_STOP", GROUP_STOP);
	define_global_int(ls, "LT_PTRACE_EVENT", PTRACE_EVENT_OCCURED_STOP);
	define_global_int(ls, "LT_EXEC_OCCURED", PTRACE_EXEC_OCCURED);
}
/*****************************************************************************/
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
	struct lua_trace_data *dat = (struct lua_trace_data*)arg;
	struct lua_State *ls = dat->ls;
	const struct user_regs_struct *uregs = &state->data.regs;

	if(dat->lua_cb_ref < 0) {
		return arg;
	}

	lua_rawgeti(ls, LUA_REGISTRYINDEX, dat->lua_cb_ref);

	lua_pushinteger(ls, state->status);
	lua_pushinteger(ls, state->pid);
	push_lua_uregs(ls, uregs);

	int err = lua_pcall(ls, 3, 0, 0);

	if(err != LUA_OK) {
		const char *err_msg = lua_tostring(ls, -1);
		ghost_fprintf(
			ghost_stderr,
			"Error in lua callback: %s\n",
			err_msg
		);
	}

	return arg;
}
/*****************************************************************************/
static void *handler_init(void *arg)
{
	int err;
	const char *err_msg;

	lua_State *ls = lua_newstate(alloc_f, sheap);
	trace_data.ls = ls;
	trace_data.lua_cb_ref = -1;

	assert(trace_data.ls != NULL);

	setup_lua_runtime(&trace_data);


	err = luaL_loadfile(ls, trace_data.ent);

	if(err != LUA_OK) {
		if(err == LUA_ERRFILE) {
			ghost_fprintf(
				ghost_stderr,
				"Error opening file: %s\n",
				trace_data.ent
			);
		} else {
			err_msg = lua_tostring(ls, -1);
			ghost_fprintf(
				ghost_stderr,
				"Error loading lua entry point: %s\n",
				err_msg
			);
		}
		abort();
		return NULL;
	}

	err = lua_pcall(ls, 0, 0, 0);

	if(err == LUA_ERRRUN) {
		err_msg = lua_tostring(ls, -1);
		ghost_fprintf(
			ghost_stderr,
			"Lua runtime error: %s\n",
			err_msg
		);
	} else if(err != LUA_OK) {
		err_msg = lua_tostring(ls, -1);
		ghost_fprintf(
			ghost_stderr,
			"Error running lua entry point: %s\n",
			err_msg
		);
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
	trace_data.lua_cb_ref = 0;

	return descr;
}
/*****************************************************************************/
