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
#include <options.h>

#include <utl/str-utl.h>

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
/******************************************************************************
*                                  CONSTANTS                                  *
******************************************************************************/
const char *OPTION_ENV_VAR = "GHOST_PATCH_OPTS";
const char *FAKE_PID_FIELD = "fake_pid";
const char *LUA_ENT_FIELD = "lua_ent";
/*****************************************************************************/
