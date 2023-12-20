#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "lib_tar.h"

#define COLORIZE_RED(x) "\033[31m" x "\033[0m"
#define COLORIZE_GREEN(x) "\033[32m" x "\033[0m"
#define COLORIZE_YELLOW(x) "\033[33m" x "\033[0m"
#define COLORIZE_BLUE(x) "\033[34m" x "\033[0m"
#define COLORIZE_MAGENTA_BOLD(x) "\033[35;1m" x "\033[0m"

#define COLORIZE_BLACK_ON_WHITE(x) "\033[30;47m" x "\033[0m"

#define TAB_SIZE 4

#define TEST_TITLE(fmt, ...) printf(COLORIZE_YELLOW("#%d") " " COLORIZE_BLUE(fmt), ++test_num, __VA_ARGS__);
#define SUBTEST_TITLE(n_spaces, fmt, ...) printf("%*s"                     \
												 " - " COLORIZE_BLUE(fmt), \
												 n_spaces, "", __VA_ARGS__);

#define EXPECTED(ret, expected)                                            \
	if (ret != expected)                                                   \
		printf(" -> " COLORIZE_RED("%d (expected: %d)\n"), ret, expected); \
	else                                                                   \
		printf(" -> " COLORIZE_GREEN("%d\n"), ret);

int test_num = 0;

void test_check_archive(int fd, char *filename, int expected)
{
	int ret = check_archive(fd);
	TEST_TITLE("check_archive('%s')", filename);
	EXPECTED(ret, expected);
}

void test_exists(int fd, char *path, int expected, int is_file_exp, int is_dir_exp, int is_symlink_exp)
{
	TEST_TITLE("exists('%s')", path);
	EXPECTED(exists(fd, path), expected);

	if (expected)
	{
		SUBTEST_TITLE(TAB_SIZE, "is_file('%s')", path);
		EXPECTED(is_file(fd, path), is_file_exp);

		SUBTEST_TITLE(TAB_SIZE, "is_dir('%s')", path);
		EXPECTED(is_dir(fd, path), is_dir_exp);

		SUBTEST_TITLE(TAB_SIZE, "is_symlink('%s')", path);
		EXPECTED(is_symlink(fd, path), is_symlink_exp);
	}
}

void test_read_file(int fd, char *path, size_t offset, int buf_size, int expected)
{
	uint8_t *buf = (uint8_t *)malloc(sizeof(uint8_t) * buf_size);

	size_t len = (size_t)buf_size;
	int ret = read_file(fd, path, offset, buf, &len);
	TEST_TITLE("read_file('%s', %lu, buf, %lu)", path, offset, len);
	EXPECTED(ret, expected);
	if (ret == expected && ret >= 0)
	{
		for (size_t i = 0; i < len; i++)
		{
			if (buf[i] == '\n')
				printf(COLORIZE_BLACK_ON_WHITE("\\n"));
			else
				printf(COLORIZE_BLACK_ON_WHITE("%c"), buf[i]);
		}
		printf("\n");
	}

	free(buf);
}

void test_list(int fd, char *path, int expected)
{
	size_t no_entries = 10;
	char *entries[no_entries];
	for (int i = 0; i < no_entries; i++)
		entries[i] = malloc(sizeof(char) * 256);

	int ret = list(fd, path, entries, &no_entries);

	TEST_TITLE("list('%s', entries, %lu)", path, no_entries);
	EXPECTED(ret, expected);
	if (expected)
	{
		for (int i = 0; i < no_entries; i++)
			printf("%s\t", entries[i]);
		printf("\n");
	}
}

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s tar_file\n", argv[0]);
		return -1;
	}

	int fd = open(argv[1], O_RDONLY);
	if (fd == -1)
	{
		perror("open(tar_file)");
		return -1;
	}

	/**
	 * CHECK_ARCHIVE TESTS
	 * */
	printf(COLORIZE_MAGENTA_BOLD("-- TEST CHECK_ARCHIVE --") "\n");
	test_check_archive(fd, argv[1], 15);

	/**
	 * EXISTS TESTS
	 * */
	printf(COLORIZE_MAGENTA_BOLD("-- TEST EXISTS --") "\n");
	test_exists(fd, "file0.txt", 1, 1, 0, 0);
	test_exists(fd, "dir1/", 1, 0, 1, 0);
	test_exists(fd, "dir1/file1.txt", 1, 1, 0, 0);
	test_exists(fd, "dir2/", 1, 0, 1, 0);
	test_exists(fd, "dir2/file2.txt", 1, 1, 0, 0);
	test_exists(fd, "symlink1", 1, 0, 0, 1);
	test_exists(fd, "symlink2", 1, 0, 0, 1);
	test_exists(fd, "symlink_subdir1", 1, 0, 0, 1);
	test_exists(fd, "symlink_subdir1/subfile1.txt", 0, 0, 0, 0);

	/**
	 * READ_FILE TESTS
	 * */
	printf(COLORIZE_MAGENTA_BOLD("-- TEST READ_FILE --") "\n");
	test_read_file(fd, "dir1/file1.txt", 0, 14, 0);
	test_read_file(fd, "dir1/file1.txt", 0, 13, 1);
	test_read_file(fd, "dir1/file1.txt", 0, 12, 2);
	test_read_file(fd, "dir1/file1.txt", 1, 14, 0);
	test_read_file(fd, "dir1/file1.txt", 1, 12, 1);
	test_read_file(fd, "dir1/file1.txt", 13, 5, 0);
	test_read_file(fd, "dir1/file1.txt", 14, 4, -2); // TODO Should this be -2 or just 0 (EOF)?

	printf("\n");
	test_read_file(fd, "symlink1", 0, 14, 0);
	test_read_file(fd, "symlink2", 0, 19, 8);

	printf("\n");
	test_read_file(fd, "dir1/", 0, 14, -1);
	test_read_file(fd, "dir2/", 0, 14, -1);

	/**
	 * LIST TESTS
	 * */
	printf(COLORIZE_MAGENTA_BOLD("-- TEST LIST --") "\n");
	test_list(fd, "file0.txt", 0);
	test_list(fd, "dir1/", 3);
	test_list(fd, "dir2/", 1);
	test_list(fd, "symlink1", 0);
	test_list(fd, "symlink2", 0);
	test_list(fd, "dir1/subdir1/", 3);
	test_list(fd, "dir1/subdir1", 0);
	test_list(fd, "symlink_subdir1", 3);
	test_list(fd, "symlink_symlink_subdir1", 3);
	test_list(fd, "symlink_subdir1/", 0);
	test_list(fd, "dir1/subdir1/subsubdir1/", 1);
	test_list(fd, "dir1", 0);
	test_list(fd, "", 0);


	fd = open("files.tar", O_RDONLY);
	if (fd == -1)
	{
		perror("open(tar_file)");
		return -1;
	}

	test_list(fd, "ls_a", 3);



	return 0;
}