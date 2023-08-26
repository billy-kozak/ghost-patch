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
#ifndef TRACEE_STATE_TABLE_H
#define TRACEE_STATE_TABLE_H
/******************************************************************************
*                                  INCLUDES                                   *
******************************************************************************/
#include <stdint.h>
#include <sys/types.h>
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
uint8_t tracee_state_table_retrieve(const void *table, pid_t id);
int tracee_state_table_store(void *table, pid_t id, uint8_t state);
void tracee_state_table_destroy(void *table);
void *tracee_state_table_init(void);
/*****************************************************************************/
#endif /* TRACEE_STATE_TABLE_H */