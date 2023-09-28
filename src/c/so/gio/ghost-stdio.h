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
*                                   DEFINES                                   *
******************************************************************************/
#define GHOST_IONBF 1
#define GHOST_IOLBF 2
#define GHOST_IOFBF 4

#define GHOST_EOF -1024

#define GHOST_SEEK_SET -1
#define GHOST_SEEK_CUR -2
#define GHOST_SEEK_END -3

#define GHOST_IO_BUF_SIZE 2048
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

extern const int ghost_L_tmpnam;
extern char *ghost_P_tmpdir;
/******************************************************************************
*                            FUNCTION DECLARATIONS                            *
******************************************************************************/
struct ghost_file *ghost_fopen(
		const char *restrict pathname,
		const char *restrict mode
);
struct ghost_file *ghost_fdopen(int fd, const char *restrict mode);
struct ghost_file *ghost_freopen(
	const char *restrict path,
	const char *restrict mode,
	struct ghost_file *restrict f
);
int ghost_fclose(struct ghost_file *file);
int ghost_fprintf(struct ghost_file *f, const char *restrict fmt, ...);
int ghost_printf(const char *restrict fmt, ...);
int ghost_snprintf(
	char *restrict str,
	size_t size,
	const char *restrict fmt,
	...
);
int ghost_sdprintf(
	char **str,
	size_t size,
	const char *restrict fmt,
	...
);
int ghost_fflush(struct ghost_file *file);
void ghost_stdio_init(void);
void ghost_stdio_cleanup(void);
int ghost_setvbuf(
	struct ghost_file *restrict f, char *restrict buf, int mode, size_t siz
);
void ghost_setbuffer(
	struct ghost_file *restrict f, char *restrict buf, size_t siz
);
void ghost_setbuf(struct ghost_file *restrict f, char *restrict buf);
struct ghost_file *ghost_tmpfile(void);
int ghost_fgetc(struct ghost_file *f);
int ghost_ungetc(int c, struct ghost_file *f);
size_t ghost_fread(
	void *restrict dst,
	size_t size,
	size_t nmemb,
	struct ghost_file *restrict f
);
size_t ghost_fwrite(
	const void *restrict src,
	size_t size,
	size_t nmemb,
	struct ghost_file *restrict f
);
void ghost_clearerr(struct ghost_file *restrict f);
int ghost_feof(struct ghost_file *restrict f);
int ghost_ferror(struct ghost_file *restrict f);
int ghost_fseek(struct ghost_file *f, long offset, int whence);
long ghost_ftell(struct ghost_file *f);
char *ghost_fgets(char *restrict s, int size, struct ghost_file *restrict f);
char *ghost_tmpnam(char *s);
int ghost_remove(const char *path);
int ghost_rename(const char *old, const char *new);
/*****************************************************************************/
#endif /* GHOST_STDIO_H */
