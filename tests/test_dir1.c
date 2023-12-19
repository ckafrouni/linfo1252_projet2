/**
 * Test suite for the archive represented by the following tree:
 * 
 * tests/resources/test_dir1
 * ├── dir1
 * │   ├── file1.txt
 * │   ├── file2.txt
 * │   └── subdir1
 * │       ├── subfile1.txt
 * │       ├── subfile2.txt
 * │       └── subsubdir1
 * ├── dir2
 * │   └── file2.txt
 * ├── file0.txt
 * ├── symlink1 -> dir1/file1.txt
 * ├── symlink2 -> dir2/file2.txt
 * ├── symlink_subdir1 -> dir1/subdir1
 * └── symlink_symlink_subdir1 -> symlink_subdir1
 * 
 * 6 directories, 8 files
 */

#include <stdio.h>
#include <fcntl.h>

#include <criterion/criterion.h>

#include "../src/lib_tar.h"
#include "helpers.h"

int fd;

void setup(void)
{
	fd = open("tests/bin/test_dir1.tar", O_RDONLY);
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

TestSuite(TS_dir1, .init = setup, .fini = teardown);

Test(TS_dir1, check_archive)
{
	test_check_archive(fd, "tests/bin/test_dir1.tar", 15);
}

Test(TS_dir1, exists)
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

Test(TS_dir1, read_file_success)
{
	test_read_file(fd, "dir1/file1.txt", 0, 14, 0, "Hello, World!\n");
	test_read_file(fd, "dir1/file1.txt", 0, 13, 1, "Hello, World!");
	test_read_file(fd, "dir1/file1.txt", 0, 12, 2, "Hello, World");
}

Test(TS_dir1, read_file_success_with_bigger_buf)
{
	test_read_file(fd, "dir1/file1.txt", 0, 16, 0, "Hello, World!\n");
}

Test(TS_dir1, read_file_offset)
{
	test_read_file(fd, "dir1/file1.txt", 1, 14, 0, "ello, World!\n");
	test_read_file(fd, "dir1/file1.txt", 1, 12, 1, "ello, World!");
	test_read_file(fd, "dir1/file1.txt", 13, 5, 0, "\n");
}

Test(TS_dir1, read_file_symlink)
{
	test_read_file(fd, "symlink1", 0, 14, 0, "Hello, World!\n");
	test_read_file(fd, "symlink2", 0, 19, 8, "Hello, again!\nHow a");
}

Test(TS_dir1, read_file_failure)
{
	test_read_file(fd, "dir1/file1.txt", 14, 4, -2, NULL);

	test_read_file(fd, "dir1/", 0, 14, -1, NULL);
	test_read_file(fd, "dir2/", 0, 14, -1, NULL);
}

Test(TS_dir1, list)
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
