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
#ifndef GHOST_STDIO_H
#define GHOST_STDIO_H
/******************************************************************************
*                                  INCLUDES                                   *
******************************************************************************/
#include <stdlib.h>
/******************************************************************************
*                                    TYPES                                    *
******************************************************************************/
struct ghost_file;
/******************************************************************************
*                                    DATA                                     *
******************************************************************************/
extern struct ghost_file *ghost_stdin;
extern struct ghost_file *ghost_stdout;
extern struct ghost_file *ghost_stderr;
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
struct ghost_file *ghost_fopen(
		const char *restrict pathname,
		const char *restrict mode
);
struct ghost_file *ghost_fdopen(int fd, const char *restrict mode);
int ghost_fclose(struct ghost_file *file);
int ghost_fprintf(struct ghost_file *f, const char *restrict fmt, ...);
int ghost_printf(const char *restrict fmt, ...);
int ghost_snprintf(char *restrict str, size_t size, char *restrict fmt, ...);
int ghost_fflush(struct ghost_file *file);
void ghost_stdio_init(void);
void ghost_stdio_cleanup(void);
/*****************************************************************************/
#endif /* GHOST_STDIO_H */
