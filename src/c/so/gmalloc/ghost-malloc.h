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
#ifndef GHOST_MALLOC_H
#define GHOST_MALLOC_H
/******************************************************************************
*                                  INCLUDES                                   *
******************************************************************************/
#include <stdlib.h>
/******************************************************************************
*                                    TYPES                                    *
******************************************************************************/
struct ghost_heap;
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
void *ghost_malloc(struct ghost_heap *heap, size_t size);
void ghost_free(struct ghost_heap *heap, void *ptr);
void *ghost_realloc(struct ghost_heap *heap, void *ptr, size_t size);
void *ghost_malloc_check_leaks(struct ghost_heap *heap, void **ptr);
int ghost_heap_destroy(struct ghost_heap *heap);
struct ghost_heap *ghost_heap_init(void);
/*****************************************************************************/
#endif /* GHOST_MALLOC_H */
