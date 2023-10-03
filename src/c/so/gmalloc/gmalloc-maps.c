/******************************************************************************
* Copyright (C) 2023 Billy Kozak                                              *
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
#include "gmalloc-maps.h"

#include <utl/file-utl.h>
#include <utl/str-utl.h>
#include <utl/math-utl.h>

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <sys/mman.h>
/******************************************************************************
*                                  CONSTANTS                                  *
******************************************************************************/
static const size_t ADDR_BUFFER = 4L * (1L << 30);

static const size_t NUM_MAPPINGS_INITIAL = 1024;
static const char MAPPING_FILE[] = "/proc/self/maps";

static const size_t MAPPING_MAX_LINE =
	(2 * 16 + 1) + /* Size of addresses fields */
	1 + /* space */
	4 + /* perms */
	1 + /* space */
	8 + /* offset */
	1 + /* space */
	5 + /* dev */
	1 + /* space */
	5 + /* inode */
	19 + /* spaces */
	PATH_MAX + /* Size of pathname */
	1 /* newline */;
/******************************************************************************
*                                    TYPES                                    *
******************************************************************************/
enum mapping_type {
	FILE,
	HEAP,
	STACK,
	ANON,
	VSDO
};

struct memory_mapping {
	void *addr_start;
	void *addr_end;
	enum mapping_type type;
};

struct mapping_list {
	size_t n;
	struct memory_mapping mappings[];
};
/******************************************************************************
*                              STATIC FUNCTIONS                               *
******************************************************************************/
static void *poor_malloc(size_t size)
{
	size_t page_size = getpagesize();
	size_t map_size = align_up_unsigned(size, page_size);

	return mmap(
		NULL,
		map_size,
		PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS,
		-1,
		0
	);
}
/*****************************************************************************/
static void poor_free(void *mem, size_t size)
{
	size_t page_size = getpagesize();
	size_t map_size = align_up_unsigned(size, page_size);

	munmap(mem, map_size);
}
/*****************************************************************************/
static void *poor_realloc(void *mem, size_t old_size, size_t new_size)
{
	size_t page_size = getpagesize();
	size_t real_old_size = align_up_unsigned(old_size, page_size);
	size_t real_new_size = align_up_unsigned(new_size, page_size);

	if(real_old_size <= real_new_size) {
		return mem;
	}

	size_t diff = real_new_size - real_old_size;
	uint8_t *after = ((uint8_t*)mem) + real_old_size;

	void *new_mem = mmap(
		after,
		diff,
		PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE,
		-1,
		0
	);

	if(new_mem != MAP_FAILED) {
		return new_mem;
	}

	new_mem = poor_malloc(real_new_size);

	if(new_mem == NULL) {
		return NULL;
	}

	memcpy(new_mem, mem, old_size);
	poor_free(mem, real_old_size);
	return new_mem;
}
/*****************************************************************************/
static size_t mapping_list_byte_size(size_t list_size)
{
	return (
		sizeof(struct mapping_list) +
		(list_size * sizeof(struct memory_mapping))
	);
}
/*****************************************************************************/
static bool map_tok_empty(struct lstring *s)
{
	if(s->len == 0) {
		return true;
	} else if(s->str[0] == '\n') {
		return true;
	} else {
		return false;
	}
}
/*****************************************************************************/
static int parse_mapping(
	struct memory_mapping *map,
	const char *line,
	size_t len
) {
	const char *saveptr = NULL;

	if(len == 0) {
		return -1;
	}
	if(line[len -1] == '\n') {
		len -= 1;
	}

	struct lstring addr = str_utl_tok_and_sqz(line, len, ' ', &saveptr);
	struct lstring perms = str_utl_tok_and_sqz(line, len, ' ', &saveptr);
	struct lstring offset = str_utl_tok_and_sqz(line, len, ' ', &saveptr);
	struct lstring dev = str_utl_tok_and_sqz(line, len, ' ', &saveptr);
	struct lstring inode = str_utl_tok_and_sqz(line, len, ' ', &saveptr);
	struct lstring path = str_utl_tok_and_sqz(line, len, ' ', &saveptr);

	if(map_tok_empty(&addr)) {
		return -1;
	}
	if(map_tok_empty(&perms)) {
		return -1;
	}
	if(map_tok_empty(&offset)) {
		return -1;
	}
	if(map_tok_empty(&dev)) {
		return -1;
	}
	if(map_tok_empty(&inode)) {
		return -1;
	}

	saveptr = NULL;
	struct lstring addr_start = str_utl_tok_and_sqz(
		addr.str,
		addr.len,
		'-',
		&saveptr
	);
	struct lstring addr_end = str_utl_tok_and_sqz(
		addr.str,
		addr.len,
		'-',
		&saveptr
	);

	map->addr_start = (void*)strtoull(addr_start.str, NULL, 16);
	map->addr_end = (void*)strtoull(addr_end.str, NULL, 16);

	if(path.len == 0) {
		map->type = ANON;
	} else if(lstring_cmp(&path, "[heap]") == 0) {
		map->type = HEAP;
	} else if(lstring_cmp(&path, "[stack]") == 0) {
		map->type = STACK;
	} else if(lstring_cmp(&path, "[vsdo]") == 0) {
		map->type = VSDO;
	} else if(path.str[0] == '[') {
		map->type = VSDO;
	} else {
		map->type = FILE;
	}

	return 0;
}
/*****************************************************************************/
static int parse_mappings(struct mapping_list **list)
{
	int fd = open(MAPPING_FILE, 0, O_RDWR);
	int count = 0;

	char line_buffer[MAPPING_MAX_LINE + 1];

	if(fd < 0) {
		count = -1;
		goto exit;
	}

	struct file_utl_reader_state reader;
	file_utl_reader_init(&reader, fd, line_buffer, MAPPING_MAX_LINE + 1);

	int r;
	while((r = file_utl_read_line(&reader)) > 0) {
		if(count >= (*list)->n) {
			void *tmp = poor_realloc(
				*list,
				mapping_list_byte_size((*list)->n),
				mapping_list_byte_size(2 * (*list)->n)
			);
			if(tmp == NULL) {
				count = -1;
				goto exit;
			}
			*list = tmp;
			(*list)->n *= 2;
		}
		const char *line = reader.data;
		size_t len = reader.len;

		if(parse_mapping((*list)->mappings + count, line, len) != 0) {
			count = -1;
			goto exit;
		}
		count += 1;
	}

	if(r != FILE_UTL_READER_EOF) {
		count = -1;
	}

exit:
	close(fd);
	return count;
}
/*****************************************************************************/
static void* check_collision(
	struct memory_mapping *mappings,
	int map_count,
	void *addr,
	size_t buffer
) {
	uint8_t *byte_addr = (void*)addr;
	size_t page_size = getpagesize();

	for(int i = 0; i < map_count; i++) {
		uint8_t *byte_start = (uint8_t*)mappings[i].addr_start;
		uint8_t *byte_end = (uint8_t*)mappings[i].addr_end;

		uint8_t *s = byte_start - buffer;
		uint8_t *e = byte_end + buffer + page_size;

		if((byte_addr >= s) && (byte_addr < e)) {
			return e;
		}
	}
	return NULL;
}
/*****************************************************************************/
static void* start_of_stack(struct memory_mapping *mappings, int map_count)
{
	for(int i = 0; i < map_count; i++) {
		if(mappings[i].type == STACK) {
			return mappings[i].addr_start;
		}
	}
	return NULL;
}
/*****************************************************************************/
static void* start_of_heap(struct memory_mapping *mappings, int map_count)
{
	for(int i = 0; i < map_count; i++) {
		if(mappings[i].type == HEAP) {
			return mappings[i].addr_start;
		}
	}
	return NULL;
}
/*****************************************************************************/
static void* end_of_data(struct memory_mapping *mappings, int map_count)
{
	if(map_count == 0) {
		return NULL;
	}

	void *end_prev = mappings[0].addr_end;

	for(int i = 1; i < map_count; i++) {
		if(mappings[i].addr_start != end_prev) {
			break;
		}
		end_prev = mappings[i].addr_end;
	}

	return end_prev;
}
/*****************************************************************************/
static size_t get_total_system_memory(void)
{
	size_t page_size = getpagesize();
	size_t pages = sysconf(_SC_PHYS_PAGES);

	return pages * page_size;
}
/******************************************************************************
*                            FUNCTION DEFINITIONS                             *
******************************************************************************/
void* gmalloc_maps_find_suitable_heap(void)
{
	struct mapping_list *mappings = poor_malloc(
		mapping_list_byte_size(NUM_MAPPINGS_INITIAL)
	);

	mappings->n = NUM_MAPPINGS_INITIAL;

	int count = parse_mappings(&mappings);

	uint8_t *ret = NULL;

	if(count <= 0) {
		goto exit;
	}

	size_t mem = get_total_system_memory();
	uint8_t *soh = start_of_heap(mappings->mappings, count);
	uint8_t *sos = start_of_stack(mappings->mappings, count);

	assert(sos != NULL);

	uint8_t *addr = NULL;

	if(soh == NULL) {
		uint8_t *eod = end_of_data(mappings->mappings, count);
		assert(eod != NULL);

		addr = eod + mem + getpagesize() * 2;
	} else {
		addr = soh + mem;
	}

	while(addr < sos) {
		uint8_t *next = check_collision(
			mappings->mappings,
			count,
			addr,
			ADDR_BUFFER
		);

		if(next != NULL) {
			addr = next;
		} else {
			ret = addr;
			goto exit;
		}
	}

exit:
	poor_free(mappings, mapping_list_byte_size(mappings->n));
	return ret;
}
/*****************************************************************************/
