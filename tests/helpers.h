#if !defined(HELPERS_H)
#define HELPERS_H

#include <criterion/criterion.h>
#include <stdlib.h>
#include <string.h>

#include "../src/lib_tar.h"

#define COLORIZE_BLACK_ON_WHITE(str) "\033[30;47m" str "\033[0m"

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
		int expected_len = strlen(expected_buf);
		cr_assert_eq(len, expected_len, "read_file('%s') failed", path);
		for (int i = 0; i < buf_size; i++)
			cr_assert_eq(buf[i], expected_buf[i],
						 "read_file('%s') failed at i=%d\n\texpected: '%s'\n\tgot: '%s' - because: '%c' != '%c'",
						 path, i, expected_buf, buf, buf[i], expected_buf[i]);
	}

	free(buf);
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
		for (size_t i = 0; i < no_entries; i++)
			cr_assert_str_eq(entries[i], expected_entries[i], "list('%s') failed", path);
	}
}


#endif // HELPERS_H
