#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <criterion/criterion.h>

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
		entries[i] = malloc(sizeof(char) * 256);

	int ret = list(fd, path, entries, &no_entries);

	cr_assert_eq(ret, expected, "list('%s') failed", path);
	if (expected)
	{
		cr_assert_eq(no_entries, expected, "list('%s') failed", path);
		for (size_t i = 0; i < no_entries; i++)
			cr_assert_str_eq(entries[i], expected_entries[i], "list('%s') failed", path);
	}
}

int fd;

void setup(void)
{
	fd = open("tests/resources/dir1.tar", O_RDONLY);
	if (fd == -1)
	{
		perror("open(tar_file)");
		return;
	}
}

void teardown(void)
{
	close(fd);
}

TestSuite(test_dir_tar, .init = setup, .fini = teardown);

Test(test_dir_tar, check_archive)
{
	test_check_archive(fd, "../resources/dir1.tar", 15);
}

Test(test_dir_tar, exists)
{
	test_exists(fd, "file0.txt", 1, 1, 0, 0);
	test_exists(fd, "dir1/", 1, 0, 1, 0);
	test_exists(fd, "dir1/file1.txt", 1, 1, 0, 0);
	test_exists(fd, "dir2/", 1, 0, 1, 0);
	test_exists(fd, "dir2/file2.txt", 1, 1, 0, 0);
	test_exists(fd, "symlink1", 1, 0, 0, 1);
	test_exists(fd, "symlink2", 1, 0, 0, 1);
	test_exists(fd, "symlink_subdir1", 1, 0, 0, 1);
	test_exists(fd, "symlink_subdir1/subfile1.txt", 0, 0, 0, 0);
}

Test(test_dir_tar, read_file_success)
{
	test_read_file(fd, "dir1/file1.txt", 0, 14, 0, "Hello, World!\n");
	test_read_file(fd, "dir1/file1.txt", 0, 13, 1, "Hello, World!");
	test_read_file(fd, "dir1/file1.txt", 0, 12, 2, "Hello, World");
}

Test(test_dir_tar, read_file_success_with_bigger_buf)
{
	// TODO: fails
	test_read_file(fd, "dir1/file1.txt", 0, 16, 0, "Hello, World!\n");
}

Test(test_dir_tar, read_file_offset)
{
	// TODO: fails
	test_read_file(fd, "dir1/file1.txt", 1, 14, 0, "ello, World!\n");
	test_read_file(fd, "dir1/file1.txt", 1, 12, 1, "ello, World!");
	test_read_file(fd, "dir1/file1.txt", 13, 5, 0, "\n");
}

Test(test_dir_tar, read_file_symlink)
{
	test_read_file(fd, "symlink1", 0, 14, 0, "Hello, World!\n");
	test_read_file(fd, "symlink2", 0, 19, 8, "Hello, again!\nHow a");
}

Test(test_dir_tar, read_file_failure)
{
	test_read_file(fd, "dir1/file1.txt", 14, 4, -2, NULL);

	test_read_file(fd, "dir1/", 0, 14, -1, NULL);
	test_read_file(fd, "dir2/", 0, 14, -1, NULL);
}

Test(test_dir_tar, list)
{
	char *list_dir1[] = {"dir1/file1.txt", "dir1/subdir1/", "dir1/file2.txt"};
	char *list_dir2[] = {"dir2/file2.txt"};
	char *list_dir1_subdir1[] = {"dir1/subdir1/subsubdir1/", "dir1/subdir1/subfile2.txt", "dir1/subdir1/subfile1.txt"};
	char *list_dir1_subdir1_subsubdir1[] = {"dir1/subdir1/subsubdir1/.gitkeep"};

	test_list(fd, "file0.txt", 0, NULL);
	test_list(fd, "dir1/", 3, list_dir1);
	test_list(fd, "dir2/", 1, list_dir2);
	test_list(fd, "symlink1", 0, NULL);
	test_list(fd, "symlink2", 0, NULL);
	test_list(fd, "dir1/subdir1/", 3, list_dir1_subdir1);
	test_list(fd, "dir1/subdir1", 0, NULL);
	test_list(fd, "symlink_subdir1", 3, list_dir1_subdir1);
	test_list(fd, "symlink_symlink_subdir1", 3, list_dir1_subdir1);
	test_list(fd, "symlink_subdir1/", 0, NULL);
	test_list(fd, "dir1/subdir1/subsubdir1/", 1, list_dir1_subdir1_subsubdir1);
	test_list(fd, "dir1", 0, NULL);
	test_list(fd, "", 0, NULL);
}