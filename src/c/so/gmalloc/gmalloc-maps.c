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

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
/******************************************************************************
*                                  CONSTANTS                                  *
******************************************************************************/
static const size_t ADDR_BUFFER = 4L * (1L << 30);

static const size_t MAX_MAPPINGS = 1024;
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
/******************************************************************************
*                              STATIC FUNCTIONS                               *
******************************************************************************/
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
static int parse_mappings(struct memory_mapping *space)
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
		if(count > MAX_MAPPINGS) {
			count = -1;
			goto exit;
		}
		const char *line = reader.data;
		size_t len = reader.len;

		if(parse_mapping(space + count, line, len) != 0) {
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
	struct memory_mapping mappings[MAX_MAPPINGS];

	int count = parse_mappings(mappings);

	if(count <= 0) {
		return NULL;
	}

	size_t mem = get_total_system_memory();
	uint8_t *soh = start_of_heap(mappings, count);
	uint8_t *sos = start_of_stack(mappings, count);

	assert(sos != NULL);

	uint8_t *addr;

	if(soh == NULL) {
		uint8_t *eod = end_of_data(mappings, count);
		assert(eod != NULL);

		addr = eod + mem + getpagesize() * 2;
	} else {
		addr = soh + mem;
	}

	while(addr < sos) {
		uint8_t *next = check_collision(
			mappings,
			count,
			addr,
			ADDR_BUFFER
		);

		if(next != NULL) {
			addr = next;
		} else {
			return addr;
		}
	}

	return NULL;
}
/*****************************************************************************/
