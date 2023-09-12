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
#include <gio/ghost-stdio.h>

#include <picounit/picounit.h>
#include <secret-heap.h>

#include <string.h>
#include <stdio.h>
/******************************************************************************
*                                    TESTS                                    *
******************************************************************************/
static bool test_noargs_fmt(void)
{
	char test_str[4096];
	size_t size = sizeof(test_str);

	ghost_snprintf(test_str, size, "foo");

	PUNIT_ASSERT(strcmp(test_str, "foo") == 0);

	return true;
}
/*****************************************************************************/
static bool test_str_fmt(void)
{
	char test_str[4096];
	size_t size = sizeof(test_str);

	ghost_snprintf(test_str, size, "%s", "foo");
	PUNIT_ASSERT(strcmp(test_str, "foo") == 0);

	ghost_snprintf(test_str, size, "%s %s", "foo", "bar");
	PUNIT_ASSERT(strcmp(test_str, "foo bar") == 0);

	ghost_snprintf(test_str, size, "%s %s", "foo", "bar");
	PUNIT_ASSERT(strcmp(test_str, "foo bar") == 0);

	ghost_snprintf(test_str, size, "%.2s", "foo");
	PUNIT_ASSERT(strcmp(test_str, "fo") == 0);

	ghost_snprintf(test_str, size, "%.10s", "foo");
	PUNIT_ASSERT(strcmp(test_str, "foo") == 0);

	return true;
}
/*****************************************************************************/
static bool test_char_fmt(void)
{
	char test_str[4096];
	size_t size = sizeof(test_str);

	ghost_snprintf(test_str, size, "%c %c %c %c", 'A', 'B', 'C', 'D');
	PUNIT_ASSERT(strcmp(test_str, "A B C D") == 0);

	return true;
}
/*****************************************************************************/
static bool test_unsigned_int_fmt(void)
{
	char test_str[4096];
	size_t size = sizeof(test_str);

	ghost_snprintf(test_str, size, "%u", 12);
	PUNIT_ASSERT(strcmp(test_str, "12") == 0);

	ghost_snprintf(test_str, size, "foo %u foo", 12);
	PUNIT_ASSERT(strcmp(test_str, "foo 12 foo") == 0);

	short s = 10;
	ghost_snprintf(test_str, size, "%hu %u", s, 100);
	PUNIT_ASSERT(strcmp(test_str, "10 100") == 0);

	ghost_snprintf(test_str, size, "%3u", 12);
	PUNIT_ASSERT(strcmp(test_str, " 12") == 0);

	ghost_snprintf(test_str, size, "%03u", 12);
	PUNIT_ASSERT(strcmp(test_str, "012") == 0);

	ghost_snprintf(test_str, size, "%o", 12);
	PUNIT_ASSERT(strcmp(test_str, "14") == 0);

	ghost_snprintf(test_str, size, "%#o", 12);
	PUNIT_ASSERT(strcmp(test_str, "014") == 0);

	ghost_snprintf(test_str, size, "%x", 0xdeadbeefu);
	PUNIT_ASSERT(strcmp(test_str, "deadbeef") == 0);

	ghost_snprintf(test_str, size, "%X", 0xdeadbeef);
	PUNIT_ASSERT(strcmp(test_str, "DEADBEEF") == 0);

	ghost_snprintf(test_str, size, "%#X", 0xdeadbeef);
	PUNIT_ASSERT(strcmp(test_str, "0XDEADBEEF") == 0);

	ghost_snprintf(test_str, size, "%#x", 0xdeadbeef);
	PUNIT_ASSERT(strcmp(test_str, "0xdeadbeef") == 0);

	void *p = (void*)(0xdeadbeefu);
	ghost_snprintf(test_str, size, "%p", p);
	PUNIT_ASSERT(strcmp(test_str, "0xdeadbeef") == 0);

	return true;
}
/*****************************************************************************/
static bool test_signed_int_fmt(void)
{
	char test_str[4096];
	size_t size = sizeof(test_str);

	ghost_snprintf(test_str, size, "%d", 12);
	PUNIT_ASSERT(strcmp(test_str, "12") == 0);

	ghost_snprintf(test_str, size, "%d", -12);
	PUNIT_ASSERT(strcmp(test_str, "-12") == 0);

	ghost_snprintf(test_str, size, "foo %d foo", -12);
	PUNIT_ASSERT(strcmp(test_str, "foo -12 foo") == 0);

	ghost_snprintf(test_str, size, "%d foo %d", 12, -12);
	PUNIT_ASSERT(strcmp(test_str, "12 foo -12") == 0);

	ghost_snprintf(test_str, size, "a %d b %d c %d d", 1, 2, 3);
	PUNIT_ASSERT(strcmp(test_str, "a 1 b 2 c 3 d") == 0);

	ghost_snprintf(test_str, size, "%3d", 10);
	PUNIT_ASSERT(strcmp(test_str, " 10") == 0);

	ghost_snprintf(test_str, size, "%03d", 10);
	PUNIT_ASSERT(strcmp(test_str, "010") == 0);

	ghost_snprintf(test_str, size, "%+4d", 10);
	PUNIT_ASSERT(strcmp(test_str, " +10") == 0);

	ghost_snprintf(test_str, size, "%-3d", 10);
	PUNIT_ASSERT(strcmp(test_str, "10 ") == 0);


	return true;
}
/*****************************************************************************/
void test_suite_ghost_stdio(void)
{
	secret_heap_init();

	PUNIT_RUN_TEST(test_noargs_fmt);
	PUNIT_RUN_TEST(test_signed_int_fmt);
	PUNIT_RUN_TEST(test_unsigned_int_fmt);
	PUNIT_RUN_TEST(test_char_fmt);
	PUNIT_RUN_TEST(test_str_fmt);
}
/*****************************************************************************/
