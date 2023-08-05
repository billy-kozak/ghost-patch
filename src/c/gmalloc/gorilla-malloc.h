/******************************************************************************
* Copyright (C) 2019  Billy Kozak                                             *
*                                                                             *
* This file is part of the gorilla-patch program                              *
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
#ifndef GORILLA_MALLOC_H
#define GORILLA_MALLOC_H
/******************************************************************************
*                                  INCLUDES                                   *
******************************************************************************/
#include <stdlib.h>
/******************************************************************************
*                                    TYPES                                    *
******************************************************************************/
struct gorilla_heap;
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
void *gorilla_malloc(struct gorilla_heap *heap, size_t size);
void gorilla_free(struct gorilla_heap *heap, void *ptr);
void *gorilla_realloc(struct gorilla_heap *heap, void *ptr, size_t size);
void *gorilla_malloc_check_leaks(struct gorilla_heap *heap, void **ptr);
int gorilla_heap_destroy(struct gorilla_heap *heap);
struct gorilla_heap *gorilla_heap_init(void);
/*****************************************************************************/
#endif /* GORILLA_MALLOC_H */
