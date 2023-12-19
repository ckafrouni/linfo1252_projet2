#ifndef __LIB_TAR_INTERNAL_H__
#define __LIB_TAR_INTERNAL_H__

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "lib_tar.h"

typedef struct
{
    int valid;
    tar_header_t header;
} header_result_t;

int chksum(tar_header_t *header)
{
    int chksum = 0;
    for (size_t i = 0; i < sizeof(tar_header_t); i++)
    {
        if (i >= 148 && i < 156)
        {
            chksum += ' ';
            continue;
        }
        chksum += ((uint8_t *)header)[i];
    }
    return chksum;
}

header_result_t *next_valid_header(int tar_fd)
{
    int zero_blocks = 0;
    while (1)
    {
        char buf[sizeof(tar_header_t)];
        read(tar_fd, buf, sizeof(tar_header_t));

        int is_zero_block = 0;
        if (buf[0] == '\0')
        {
            is_zero_block = 1;
        }

        if (is_zero_block)
        {
            zero_blocks++;
            if (zero_blocks == 2)
                return NULL;
            continue;
        }

        zero_blocks = 0;

        tar_header_t *header = (tar_header_t *)buf;

        if (strncmp(header->magic, TMAGIC, 6) != 0)
        {
            header_result_t *result = (header_result_t *)malloc(sizeof(header_result_t));
            result->valid = -1;
            return result;
        }

        if (strncmp(header->version, TVERSION, 2) != 0)
        {
            header_result_t *result = (header_result_t *)malloc(sizeof(header_result_t));
            result->valid = -2;
            return result;
        }

        if (chksum(header) != TAR_INT(header->chksum))
        {
            header_result_t *result = (header_result_t *)malloc(sizeof(header_result_t));
            result->valid = -3;
            return result;
        }

        header_result_t *result = (header_result_t *)malloc(sizeof(header_result_t));
        result->valid = 1;
        result->header = *header;
        return result;
    }
}

void skip_file_content(int tar_fd, tar_header_t *header)
{
    int size = TAR_INT(header->size);
    int blocks = (size + 511) / 512; // Round up to nearest block
    lseek(tar_fd, blocks * 512, SEEK_CUR);
}

tar_header_t *get_header(int tar_fd, char *path)
{
    while (1)
    {
        header_result_t *header_result = next_valid_header(tar_fd);
        if (header_result == NULL)
            break;

        tar_header_t *header = &(header_result->header);

        if (header == NULL)
            break;

        if (strcmp(header->name, path) == 0)
            return header;
        skip_file_content(tar_fd, header);
    }
    return NULL;
}

tar_header_t *follow_symlinks(int tar_fd, tar_header_t *header)
{
    while (header->typeflag == SYMTYPE)
    {

        char link[strlen(header->linkname) + 1];
        strncpy(link, header->linkname, sizeof(link));

        lseek(tar_fd, 0, SEEK_SET);
        header = get_header(tar_fd, link);

        // TODO: make it leaner
        if (header == NULL)
        {
            lseek(tar_fd, 0, SEEK_SET);
            // Add a slash to the link name and try again
            char link_with_slash[strlen(link) + 2];
            strcpy(link_with_slash, link);
            strcat(link_with_slash, "/");
            header = get_header(tar_fd, link_with_slash);
            if (header == NULL)
            {
                lseek(tar_fd, 0, SEEK_SET);
                return NULL;
            }
        }

        if (header == NULL)
            break;
    }

    return header;
}

#endif // __LIB_TAR_INTERNAL_H__
