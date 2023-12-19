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

// Add your tests here
