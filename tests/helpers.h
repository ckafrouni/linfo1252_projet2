#ifndef __HELPERS_H__
#define __HELPERS_H__

#include <criterion/criterion.h>
#include <criterion/assert.h>

#include <stdlib.h>
#include <string.h>

#include "lib_tar.h"

void test_check_archive(int fd, char *filename, int expected)
{
	int ret = check_archive(fd);
	cr_assert_eq(ret, expected, "check_archive('%s') failed", filename);
}

void test_exists(int fd, char *path, int expected, int is_file_exp, int is_dir_exp, int is_symlink_exp)
{
	cr_assert_eq(exists(fd, path), expected, "exists('%s') failed", path);

	if (expected)
	{
		cr_assert_eq(is_file(fd, path), is_file_exp, "is_file('%s') failed", path);
		cr_assert_eq(is_dir(fd, path), is_dir_exp, "is_dir('%s') failed", path);
		cr_assert_eq(is_symlink(fd, path), is_symlink_exp, "is_symlink('%s') failed", path);
	}
}

void test_read_file(int fd, char *path, size_t offset, int buf_size, int expected, char *expected_buf)
{
	uint8_t *buf = (uint8_t *)malloc(sizeof(uint8_t) * buf_size);

	size_t len = (size_t)buf_size;
	int ret = read_file(fd, path, offset, buf, &len);
	cr_assert_eq(ret, expected, "read_file('%s') failed", path);
	if (ret == expected && ret >= 0)
	{
		size_t expected_len = strlen(expected_buf);
		cr_assert_eq(len, expected_len, "read_file('%s') failed", path);
		if (len != expected_len)
			return;
		for (size_t i = 0; i < expected_len; i++)
		{
			cr_assert_eq(buf[i], expected_buf[i],
						 "read_file('%s') failed\n"
						 "\texpected: '%s'\n"
						 "\tresult: '%s'\n"
						 "at index: %ld -> '%c' != '%c'",
						 path, expected_buf, buf, i, expected_buf[i], buf[i]);
		}
	}

	free(buf);
}

char *get_array_str(char *arr[], size_t len)
{
	char *str = (char *)malloc(sizeof(char) * 256);
	strcpy(str, "[");
	for (size_t i = 0; i < len; i++)
	{
		strcat(str, "\"");
		strcat(str, arr[i]);
		strcat(str, "\"");
		if (i < len - 1)
			strcat(str, ", ");
	}
	strcat(str, "]");
	return str;
}

int compare_strings(const void *a, const void *b)
{
	const char **ia = (const char **)a;
	const char **ib = (const char **)b;
	return strcmp(*ia, *ib);
}

void test_list(int fd, char *path, int expected, char *expected_entries[])
{
	size_t no_entries = 10;
	char *entries[no_entries];
	for (size_t i = 0; i < no_entries; i++)
		entries[i] = (char *)malloc(sizeof(char) * 256);

	int ret = list(fd, path, entries, &no_entries);

	cr_assert_eq(ret, expected, "list('%s') failed", path);
	if (expected)
	{
		cr_assert_eq(no_entries, expected, "list('%s') failed", path);

		qsort(entries, no_entries, sizeof(char *), compare_strings);
		qsort(expected_entries, no_entries, sizeof(char *), compare_strings);

		cr_assert(
			strcmp(get_array_str(entries, no_entries), get_array_str(expected_entries, no_entries)) == 0,
			"list('%s') failed\n"
			"\texpected: %s\n"
			"\tresult: %s\n",
			path, get_array_str(expected_entries, no_entries), get_array_str(entries, no_entries)
		);
	}
}

#endif // __HELPERS_H__
