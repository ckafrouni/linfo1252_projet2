/**
 * Test suite for the tar file represented by the following tree:
 *
 * TODO: Add the tree here.
 * 
 * tests/resources/test_dir2
 * └── file0.txt
 * 
 * 0 directories, 1 file
 */
#include <stdio.h>
#include <fcntl.h>

#include <criterion/criterion.h>

#include "../src/lib_tar.h"
#include "helpers.h"

int fd;

void setup(void)
{
    fd = open("tests/bin/test_dir2.tar", O_RDONLY);
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

TestSuite(TS_dir2, .init = setup, .fini = teardown);

// TODO: Add tests here.

Test(TS_dir2, check_archive)
{
    test_check_archive(fd, "tests/bin/test_dir2.tar", 1);
}

Test(TS_dir2, exists)
{
    test_exists(fd, "file0.txt", 1, 1, 0, 0);
}
