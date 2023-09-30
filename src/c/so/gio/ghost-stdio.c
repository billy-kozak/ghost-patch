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
#include "ghost-stdio.h"
#include "ghost-stdio-internal.h"

#include <utl/math-utl.h>
#include <circ_buffer.h>
#include <secret-heap.h>
#include <utl/random-utl.h>

#define __USE_GNU
#include <fcntl.h>
#undef __USE_GNU

#include <limits.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <assert.h>
#include <stdbool.h>
/******************************************************************************
*                                   DEFINES                                   *
******************************************************************************/
#define MIN_BUF_SIZE 16
#define PREBUF_THRESH 8

#define GHOST_L_TMPNAM 128
#define GHOST_TMPNAM_FLEN 32

#define MAX_PROCID_STRLEN 10
/******************************************************************************
*                                    DATA                                     *
******************************************************************************/
struct ghost_file *ghost_stdin;
struct ghost_file *ghost_stdout;
struct ghost_file *ghost_stderr;

char *ghost_P_tmpdir = "/tmp";
const int ghost_L_tmpnam = GHOST_L_TMPNAM;

static char tmpnam_static_space[GHOST_L_TMPNAM];

static char fd_link_prefix[] = "/proc/self/fd";
/******************************************************************************
*                            FORWARD DECLARATIONS                             *
******************************************************************************/
/* manually declare rename, so we can avoid including all of stdio.
 * we want to avoid including stdio, so that it's easier to keep track of
 * code using stdio functions in order to ensure that no stdio functions are
 * used which will not work when running in ghost patch */
int rename(const char *old_path, const char *new_path);
/******************************************************************************
*                              STATIC FUNCTIONS                               *
******************************************************************************/
static int interp_mode(const char *restrict mode, struct fmode *p)
{
	size_t len = 0;
	while(mode[len] != '\0') {
		if(len > 3) {
			return -1;
		}
		len += 1;
	}

	char ch0 = mode[0];
	char ch1 = len >= 2 ? mode[1] : '\0';
	char ch2 = len >= 3 ? mode[2] : '\0';

	if((ch1 == 'b') && (ch2 == 'b')) {
		return -1;
	}
	if(ch1 != 'b' && ch1 != '+' && ch1 != '\0') {
		return -1;
	}
	if(ch2 != 'b' && ch2 != '+' && ch2 != '\0') {
		return -1;
	}

	if(ch0 == 'r') {
		if(ch1 == '+' || ch2 == '+') {
			p->flags = O_RDWR;
			p->mode = 0;
		} else {
			p->flags = O_RDONLY;
			p->mode = 0;
		}
	} else if(ch0 == 'w') {
		if(ch1 == '+' || ch2 == '+') {
			p->flags = O_RDWR;
			p->mode = O_CREAT;
		} else {
			p->flags = O_WRONLY;
			p->mode = O_CREAT;
		}
	} else if(ch0 == 'a') {
		if(ch1 == '+' || ch2 == '+') {
			p->flags = O_RDWR;
			p->mode = O_CREAT | O_APPEND;
		} else {
			p->flags = O_WRONLY;
			p->mode = O_CREAT | O_APPEND;
		}
	} else {
		return -1;
	}

	return 0;
}
/*****************************************************************************/
static int get_rw_flags(struct fmode *mode)
{
	int acc_mode = (mode->flags) & O_ACCMODE;

	if(acc_mode == O_RDWR) {
		return GIO_FLAG_READ | GIO_FLAG_WRITE;
	} else if(acc_mode == O_RDONLY) {
		return GIO_FLAG_READ;
	} else {
		return GIO_FLAG_WRITE;
	}
}
/*****************************************************************************/
static struct ghost_file *internal_ghost_fdopen_into(
	int fd, struct fmode fmode, struct ghost_file *file
) {
	int acc_mode = (fmode.flags) & O_ACCMODE;

	file->fd = fd;
	file->flags = GIO_FLAG_SBUF | GIO_FLAG_OPEN;
	file->flags |= get_rw_flags(&fmode);

	if(acc_mode == O_RDWR) {

		circ_buffer_init(
			&file->wb,
			(uint8_t*)file->sys_buffer,
			GHOST_IO_BUF_SIZE / 2
		);

		circ_buffer_init(
			&file->rb,
			(uint8_t*)(file->sys_buffer + GHOST_IO_BUF_SIZE / 2),
			GHOST_IO_BUF_SIZE - (GHOST_IO_BUF_SIZE / 2)
		);
	} else if(acc_mode == O_RDONLY) {

		circ_buffer_init(
			&file->rb,
			(uint8_t*)file->sys_buffer,
			GHOST_IO_BUF_SIZE
		);

		file->wb.buf = NULL;
		file->wb.buf_size = 0;
	} else {

		circ_buffer_init(
			&file->wb,
			(uint8_t*)file->sys_buffer,
			GHOST_IO_BUF_SIZE
		);

		file->rb.buf = NULL;
		file->rb.buf_size = 0;
	}

	file->err = 0;

	if(isatty(fd)) {
		file->flags |= GIO_FLAG_LF;
	}
	if(fd != 2) {
		file->flags |= GIO_FLAG_BUF;
	}

	return file;
}
/*****************************************************************************/
static struct ghost_file *internal_ghost_fdopen( int fd, struct fmode fmode)
{
	struct ghost_file *file = ghost_malloc(
		sheap, sizeof(*file) + GHOST_IO_BUF_SIZE
	);

	if(file == NULL) {
		return file;
	}

	return internal_ghost_fdopen_into(fd, fmode, file);
}
/*****************************************************************************/
static int read_to_fill_buffer(struct ghost_file *f)
{
	struct circ_buffer *rb = &f->rb;

	if(circ_buffer_capacity(rb) == 0) {
		return 0;
	}

	uint8_t *wptr = circ_buffer_wptr(rb);
	size_t wcount = circ_buffer_contig_wsize(rb);

	int r = read(f->fd, wptr, wcount);

	if(r < 0) {
		f->err |= GIO_ERR_IOERR;
		return r;
	} else if(r == 0) {
		f->err |= GIO_ERR_EOF;
		return r;
	}

	circ_buffer_increment_used(rb, r);

	if(r < wcount) {
		return r;
	}

	size_t total_read = r;

	wptr = circ_buffer_wptr(rb);
	wcount = circ_buffer_contig_wsize(rb);

	if(wcount == 0) {
		return total_read;
	}

	r = read(f->fd, wptr, wcount);

	if(r < 0) {
		f->err |= GIO_ERR_IOERR;
		return total_read;
	}

	circ_buffer_increment_used(rb, r);

	return total_read;
}
/*****************************************************************************/
static int path_of_fd(int fd, char *path)
{
	size_t link_size = MAX_PROCID_STRLEN + sizeof(fd_link_prefix) + 1;
	char link_path[link_size];

	ghost_snprintf(link_path, link_size, "%s/%d", fd_link_prefix, fd);

	if(readlink(link_path, path, PATH_MAX) != 0) {
		return -1;
	}

	if(path[0] != '/') {
		path[0] = '\0';
		return -1;
	}

	return 0;
}
/******************************************************************************
*                            FUNCTION DEFINITIONS                             *
******************************************************************************/
struct ghost_file *ghost_fopen(
	const char *restrict pathname,
	const char *restrict mode
) {
	struct fmode fmode;

	if(interp_mode(mode, &fmode) != 0) {
		return NULL;
	}

	int fd = open(pathname, fmode.flags, fmode.mode);

	if(fd < 0) {
		return NULL;
	}

	return internal_ghost_fdopen(fd, fmode);
}
/*****************************************************************************/
struct ghost_file *ghost_fdopen(int fd, const char *restrict mode)
{
	struct fmode fmode;

	if(interp_mode(mode, &fmode) != 0) {
		return NULL;
	}

	return internal_ghost_fdopen(fd, fmode);
}
/*****************************************************************************/
struct ghost_file *ghost_freopen(
	const char *restrict path,
	const char *restrict mode,
	struct ghost_file *restrict f
) {
	struct fmode fmode;

	if(interp_mode(mode, &fmode) != 0) {
		return NULL;
	}

	int new_flags = get_rw_flags(&fmode);

	if(path == NULL) {
		if((new_flags & f->flags) == new_flags) {
			/* new mode is fully compatible with old, no changes to
			 * underlying file descriptor are needed. Just change
			* flags to disallow any permissions which were
			* revoked and then seek back to start of file. */
			f->flags &= ~(GIO_FLAG_READ | GIO_FLAG_WRITE);
			f->flags = new_flags;
			ghost_fseek(f, 0, GHOST_SEEK_SET);
			return f;
		}
		char old_path[PATH_MAX];
		struct stat sb;
		if(path_of_fd(f->fd, old_path) != 0) {
			return NULL;
		}
		if(lstat(old_path, &sb) != 0) {
			return NULL;
		}
		if((sb.st_mode & S_IFMT) != S_IFREG) {
			return NULL;
		}
		int new_fd = open(old_path, fmode.flags, fmode.mode);
		if(new_fd < 0) {
			return NULL;
		}
		close(f->fd);
		return internal_ghost_fdopen_into(new_fd, fmode, f);
	} else {
		close(f->fd);
		int new_fd = open(path, fmode.flags, fmode.mode);
		if(new_fd < 0) {
			return NULL;
		}
		return internal_ghost_fdopen_into(new_fd, fmode, f);
	}
}
/*****************************************************************************/
int ghost_fclose(struct ghost_file *file)
{
	if(file == NULL) {
		return 0;
	}

	ghost_fflush(file);

	int ret = close(file->fd);

	ghost_free(sheap, file);

	return ret;
}
/*****************************************************************************/
int ghost_fflush(struct ghost_file *file)
{
	if(!(file->flags & GIO_FLAG_WRITE)) {
		return 0;
	}

	size_t w_len = circ_buffer_contig_rsize(&file->wb);
	uint8_t *rptr = circ_buffer_rptr(&file->wb);

	if(w_len == 0) {
		return 0;
	}

	int w = write(file->fd, rptr, w_len);

	if(w < 0) {
		return -1;
	}

	circ_buffer_decrement_used(&file->wb, w);

	if(circ_buffer_used(&file->wb) == 0) {
		return 0;
	}

	/* flush bytes from the cb which wrapped around to start of the memory
	 * space ... or fewer bytes were written by write() than were supplied
	 */

	w_len = circ_buffer_contig_rsize(&file->wb);
	rptr = circ_buffer_rptr(&file->wb);

	assert(rptr == file->wb.buf);

	w = write(file->fd, rptr, w_len);

	if(w < 0) {
		return -1;
	}
	circ_buffer_decrement_used(&file->wb, w);

	assert((w == w_len) == (circ_buffer_used(&file->wb) == 0));

	return 0;
}
/*****************************************************************************/
int ghost_setvbuf(
	struct ghost_file *restrict f, char *restrict buf, int mode, size_t siz
) {
	int e = ghost_fflush(f);

	bool reassign_buf =
		buf != NULL &&
		siz >= MIN_BUF_SIZE &&
		e == 0 &&
		circ_buffer_used(&f->wb) == 0 &&
		circ_buffer_used(&f->rb) == 0;

	if(reassign_buf) {
		if(f->flags & GIO_FLAG_SBUF) {
			ghost_realloc(sheap, f, sizeof(*f));
			f->flags &= ~GIO_FLAG_SBUF;
		}
		if(f->flags & GIO_FLAG_READ && f->flags & GIO_FLAG_WRITE) {
			circ_buffer_init( &f->wb, (uint8_t*)buf, siz / 2);
			circ_buffer_init(&f->rb, (uint8_t*)buf, siz - siz / 2);
		} else if(f->flags & GIO_FLAG_WRITE) {
			circ_buffer_init(&f->wb, (uint8_t*)buf, siz);
		} else {
			circ_buffer_init(&f->rb, (uint8_t*)buf, siz);
		}
	}

	if(mode == GHOST_IONBF) {
		f->flags &= ~GIO_FLAG_BUF;
	} else if(mode == GHOST_IOFBF) {
		f->flags |= GIO_FLAG_BUF;
		f->flags &= ~GIO_FLAG_LF;
	} else if(mode== GHOST_IOLBF) {
		f->flags |= GIO_FLAG_BUF | GIO_FLAG_LF;
	}

	return 0;
}
/*****************************************************************************/
void ghost_setbuffer(
	struct ghost_file *restrict f, char *restrict buf, size_t siz
) {
	int mode = buf == NULL ? GHOST_IONBF : GHOST_IOFBF;
	ghost_setvbuf(f, buf, mode, siz);
}
/*****************************************************************************/
void ghost_set_buf(struct ghost_file *restrict f, char *restrict buf) {
	int mode = buf == NULL ? GHOST_IONBF : GHOST_IOFBF;
	ghost_setvbuf(f, buf, mode, GHOST_IO_BUF_SIZE);
}
/*****************************************************************************/
struct ghost_file *ghost_tmpfile(void)
{
	int fd = open("/tmp", O_TMPFILE | O_RDWR, 0600);

	if(fd < 0) {
		return NULL;
	}

	return ghost_fdopen(fd, "w+");
}
/*****************************************************************************/
int ghost_fgetc(struct ghost_file *f)
{
	if(!(f->flags & GIO_FLAG_READ)) {
		f->err |= GIO_ERR_BAD_MODE;
		return -1;
	}

	char c;

	if(circ_buffer_used(&f->rb) != 0) {
		circ_buffer_read(&f->rb, &c, 1);
		return c;
	}

	uint8_t *wptr = circ_buffer_wptr(&f->rb);
	size_t rcount = circ_buffer_contig_wsize(&f->rb);

	int r = read(f->fd, wptr, rcount);

	if(r == 0) {
		f->flags |= GIO_ERR_EOF;
		return GHOST_EOF;
	} else if(r < 0) {
		return -1;
	}

	circ_buffer_increment_used(&f->rb, r);
	circ_buffer_read(&f->rb, &c, 1);

	return c;
}
/*****************************************************************************/
int ghost_ungetc(int c, struct ghost_file *f)
{
	return circ_buffer_prepend(&f->rb, c);
}
/*****************************************************************************/
void ghost_stdio_cleanup(void)
{
	ghost_fflush(ghost_stdout);
	ghost_fflush(ghost_stderr);
}
/*****************************************************************************/
size_t ghost_fread(
	void *restrict dst,
	size_t size,
	size_t nmemb,
	struct ghost_file *restrict f
) {
	size_t total = size * nmemb;
	size_t pre_buffed = circ_buffer_used(&f->rb);


	if(!(f->flags & GIO_FLAG_READ)) {
		f->err |= GIO_ERR_BAD_MODE;
		return 0;
	}

	if(pre_buffed >= total) {
		return circ_buffer_read(&f->rb, dst, total);
	}

	if(f->rb.buf_size < (size - 1)) {
		f->err |= GIO_ERR_BUFSIZ;
		return 0;
	}

	size_t needed = total - pre_buffed;
	int r = 0;

	size_t buf_cap = circ_buffer_capacity(&f->rb);

	if(needed < PREBUF_THRESH && buf_cap >= PREBUF_THRESH) {
		r = read_to_fill_buffer(f);
		if(r < 0) {
			return 0;
		}
		if(r >= needed) {
			return circ_buffer_read(&f->rb, dst, total);
		} else {
			needed -= r;
			pre_buffed += r;
		}
	}

	/* read from file to offset in dst which will give exactly enough room
	 * to write the whole contents of the cb into the start of dst.
	* This way we can avoid relocating bytes as much as possible */
	uint8_t *ptr = ((uint8_t*)dst) + pre_buffed;
	r = read(f->fd, ptr, needed);

	if(r < 0) {
		f->err |= GIO_ERR_IOERR;
		return 0;
	} else if(r == 0 && pre_buffed == 0) {
		f->err |= GIO_ERR_EOF;
		return 0;
	} else if(r == 0) {
		f->err |= GIO_ERR_EOF;
	}

	size_t bytes_total = pre_buffed + r;
	size_t writable = align_down_unsigned(bytes_total, size);
	size_t extra = bytes_total % size;

	if(bytes_total < size) {
		assert(circ_buffer_capacity(&f->rb) >= r);
		circ_buffer_write(&f->rb, ptr, r);
		return 0;
	}

	if(writable <= pre_buffed) {
		/* write buffered bytes into dst */
		circ_buffer_read(&f->rb, dst, writable);
		/* move any bytes read from the file back into the cb, as any
		 * bytes read from the file are unusable for the purpose of
		 * constructing a whole object of size *size* */
		circ_buffer_write(&f->rb, ptr, r);
		return writable / size;
	}

	/*write cb contents to beggining of dst array */
	circ_buffer_read(&f->rb, dst, pre_buffed);
	/* move excess back into the cb */
	circ_buffer_write(&f->rb, ptr + (r - extra), extra);

	return writable / size;
}
/*****************************************************************************/
size_t ghost_fwrite(
	const void *restrict src,
	size_t size,
	size_t nmemb,
	struct ghost_file *restrict f
) {
	size_t total = size * nmemb;
	uint8_t *bsrc = (uint8_t*)src;

	if(!(f->flags & GIO_FLAG_BUF)) {
		int w = write(f->fd, src, total);
		if(w < 0) {
			f->err |= GIO_ERR_IOERR;
			return 0;
		} else {
			return w;
		}
	}

	size_t total_written = 0;

	while(total > 0) {
		size_t w = circ_buffer_write(&f->wb, bsrc, total);
		total_written += w;

		if(w <= total) {
			if(ghost_fflush(f) != 0) {
				f->err |= GIO_ERR_IOERR;
				return total_written;
			}
		}
		total -= w;
		bsrc += w;
	}

	assert(total_written == (size * nmemb));

	if(!(f->flags & GIO_FLAG_LF) || circ_buffer_used(&f->wb) == 0) {
		return total_written;
	}

     	size_t flush_count = 0;

	for(int i = circ_buffer_used(&f->wb) - 1; i >= 0; i--) {
		int c = circ_buffer_get(&f->wb, i);
		if(c == '\n') {
			flush_count = i + 1;
			break;
		}
	}

	while(flush_count != 0) {
		size_t wcount = min_u64(
			flush_count,
			circ_buffer_contig_rsize(&f->wb)
		);
		int w = write(f->fd, circ_buffer_rptr(&f->wb), wcount);

		if(w < 0) {
			f->err |= GIO_ERR_IOERR;
			return total_written;
		} else if(w == 0) {
			return total_written;
		}
		circ_buffer_decrement_used(&f->wb, w);
		flush_count -= w;
	}

	return total_written;
}
/*****************************************************************************/
void ghost_clearerr(struct ghost_file *restrict f)
{
	f->err = 0;
}
/*****************************************************************************/
int ghost_ferror(struct ghost_file *restrict f)
{
	return !!(f->err & ~GIO_ERR_EOF);
}
/*****************************************************************************/
int ghost_feof(struct ghost_file *restrict f)
{
	if(!!(f->err & GIO_ERR_EOF)) {
		return circ_buffer_used(&f->rb) == 0;
	} else {
		return 0;
	}
}
/*****************************************************************************/
int ghost_fseek(struct ghost_file *f, long offset, int whence)
{
	ghost_fflush(f);

	off_t ret;

	if(whence == GHOST_SEEK_SET) {
		ret = lseek(f->fd, offset, SEEK_SET);
	} else if(whence == GHOST_SEEK_CUR) {
		ret = lseek(f->fd, offset, SEEK_CUR);
	} else if(whence == GHOST_SEEK_END) {
		ret = lseek(f->fd, offset, SEEK_END);
	} else {
		ret = lseek(f->fd, offset, whence);
	}

	if(ret >= 0) {
		circ_buffer_clear(&f->wb);
		circ_buffer_clear(&f->rb);
	}

	return ret;
}
/*****************************************************************************/
long ghost_ftell(struct ghost_file *f)
{
	size_t rbuf = circ_buffer_used(&f->rb);
	size_t wbuf = circ_buffer_used(&f->wb);

	assert((rbuf != 0) && (wbuf != 0));

	off_t off = lseek(f->fd, 0, SEEK_CUR);

	if(off < 0) {
		return off;
	}

	if(rbuf) {
		assert(off >= rbuf);
		return off - rbuf;
	} else if(wbuf) {
		return off + wbuf;
	} else {
		return off;
	}
}
/*****************************************************************************/
char *ghost_fgets(char *restrict s, int size, struct ghost_file *restrict f)
{
	int idx = 0;

	while(idx < (size - 1)) {
		size_t buffered = circ_buffer_used(&f->rb);
		for(int i = 0; i < buffered; i++) {
			int c = circ_buffer_pop(&f->rb);
			s[idx] = c;
			idx += 1;
			if(c == '\n') {
				goto end;
			}
		}
		int r = read_to_fill_buffer(f);
		if(r <= 0) {
			goto end;
		}
	}
end:
	s[idx] = '\0';
	return idx == 0 ? NULL : s;
}
/*****************************************************************************/
char *ghost_tmpnam(char *s)
{
	struct drand48_data rng;
	random_utl_seed_from_clock(&rng);

	char *target = s == NULL ? tmpnam_static_space : s;

	size_t prefix_len = strlen(ghost_P_tmpdir);
	strncpy(target, ghost_P_tmpdir, prefix_len);

	target[prefix_len] = '/';
	prefix_len += 1;

	assert((prefix_len + GHOST_TMPNAM_FLEN + 1) < ghost_L_tmpnam);

	do {
		random_utl_rand_alpha_num(
			&rng, s + prefix_len, GHOST_TMPNAM_FLEN
		);
		s[prefix_len + GHOST_TMPNAM_FLEN] = '\0';
	} while(access(target, F_OK) == 0);

	return target;
}
/*****************************************************************************/
int ghost_remove(const char *path)
{
	struct stat sb;

	if(lstat(path, &sb) != 0) {
		return -1;
	}

	if((sb.st_mode & S_IFMT) == S_IFDIR) {
		return rmdir(path);
	} else {
		return unlink(path);
	}
}
/*****************************************************************************/
int ghost_rename(const char *old, const char *new)
{
	return rename(old, new);
}
/*****************************************************************************/
void ghost_stdio_init(void)
{
	ghost_stdin = ghost_fdopen(0, "r");
	ghost_stdout = ghost_fdopen(1, "w");
	ghost_stderr = ghost_fdopen(2, "w");
}
/*****************************************************************************/
