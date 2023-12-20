#include "lib_tar.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define COLORIZE_RED(s) "\033[31m" s "\033[0m"
#define COLORIZE_GREEN(s) "\033[32m" s "\033[0m"
#define COLORIZE_YELLOW(s) "\033[33m" s "\033[0m"
#define COLORIZE_BLUE(s) "\033[34m" s "\033[0m"
#define COLORIZE_MAGENTA(s) "\033[35m" s "\033[0m"
#define COLORIZE_CYAN(s) "\033[36m" s "\033[0m"
#define COLORIZE_WHITE(s) "\033[37m" s "\033[0m"

#define DEBUG_FMT(fmt, ...) printf(COLORIZE_CYAN("%s:%d" " - " fmt "\n"), __FILE__, __LINE__, __VA_ARGS__)
#define DEBUG(s) DEBUG_FMT("%s", s)

typedef struct
{
    int valid;
    tar_header_t header;
} header_result_t;

int chksum(tar_header_t *header)
{
    int chksum = 0;
    for (int i = 0; i < sizeof(tar_header_t); i++)
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
            header_result_t *result = malloc(sizeof(header_result_t));
            result->valid = -1;
            return result;
        }

        if (strncmp(header->version, TVERSION, 2) != 0)
        {
            header_result_t *result = malloc(sizeof(header_result_t));
            result->valid = -2;
            return result;
        }

        if (chksum(header) != TAR_INT(header->chksum))
        {
            header_result_t *result = malloc(sizeof(header_result_t));
            result->valid = -3;
            return result;
        }

        header_result_t *result = malloc(sizeof(header_result_t));
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
    // printf(COLORIZE_CYAN("%s:%d - Follow symlinks\n"), __FILE__, __LINE__);
    DEBUG("Follow symlinks");
    while (header->typeflag == SYMTYPE)
    {

        char link[strlen(header->linkname) + 1];
        strncpy(link, header->linkname, sizeof(link));

        DEBUG_FMT("linkname: %s", link);

        lseek(tar_fd, 0, SEEK_SET);
        header = get_header(tar_fd, link);

        DEBUG_FMT("header->name 1: %s", header->name);
        // TODO: make it leaner
        if (header == NULL)
        {
            lseek(tar_fd, 0, SEEK_SET);
            // Add a slash to the link name and try again
            char link_with_slash[strlen(link) + 2];
            strcpy(link_with_slash, link);
            strcat(link_with_slash, "/");
            // printf("link_with_slash: %s\n", link_with_slash);
            header = get_header(tar_fd, link_with_slash);
            DEBUG_FMT("header->name 2: %s", header->name);
            if (header == NULL)
            {
                // printf("header is null\n");
                lseek(tar_fd, 0, SEEK_SET);
                return NULL;
            }
        }

        if (header == NULL)
            break;
    }

    return header;
}

/**
 * Checks whether the archive is valid.
 *
 * Each non-null header of a valid archive has:
 *  - a magic value of "ustar" and a null,
 *  - a version value of "00" and no null,
 *  - a correct checksum
 *
 * @param tar_fd A file descriptor pointing to the start of a file supposed to contain a tar archive.
 *
 * @return a zero or positive value if the archive is valid, representing the number of non-null headers in the archive,
 *         -1 if the archive contains a header with an invalid magic value,
 *         -2 if the archive contains a header with an invalid version value,
 *         -3 if the archive contains a header with an invalid checksum value
 */
int check_archive(int tar_fd)
{
    int count = 0;
    while (1)
    {
        header_result_t *header_result = next_valid_header(tar_fd);
        if (header_result == NULL)
            break;
        if (header_result->valid < 0)
        {
            count = header_result->valid;
            break;
        }
        tar_header_t *header = &(header_result->header);
        count++;
        // printf("%d %s\n", header->typeflag, header->name);

        skip_file_content(tar_fd, header);
    }
    lseek(tar_fd, 0, SEEK_SET);
    return count;
}

/**
 * Checks whether an entry exists in the archive.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive,
 *         any other value otherwise.
 */
int exists(int tar_fd, char *path)
{
    int count = 0;
    while (1)
    {
        tar_header_t *header = get_header(tar_fd, path);
        if (header == NULL)
            break;
        count++;
        skip_file_content(tar_fd, header);
    }
    lseek(tar_fd, 0, SEEK_SET);
    return count;
}

/**
 * Checks whether an entry exists in the archive and is a directory.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive or the entry is not a directory,
 *         any other value otherwise.
 */
int is_dir(int tar_fd, char *path)
{
    tar_header_t *header = get_header(tar_fd, path);
    lseek(tar_fd, 0, SEEK_SET);
    if (header == NULL)
        return 0;
    return header->typeflag == DIRTYPE;
}

/**
 * Checks whether an entry exists in the archive and is a file.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive or the entry is not a file,
 *         any other value otherwise.
 */
int is_file(int tar_fd, char *path)
{
    tar_header_t *header = get_header(tar_fd, path);
    lseek(tar_fd, 0, SEEK_SET);
    if (header == NULL)
        return 0;
    return header->typeflag == REGTYPE;
}

/**
 * Checks whether an entry exists in the archive and is a symlink.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 * @return zero if no entry at the given path exists in the archive or the entry is not symlink,
 *         any other value otherwise.
 */
int is_symlink(int tar_fd, char *path)
{
    tar_header_t *header = get_header(tar_fd, path);
    lseek(tar_fd, 0, SEEK_SET);
    if (header == NULL)
        return 0;
    return header->typeflag == SYMTYPE;
}

/**
 * Lists the entries at a given path in the archive.
 * list() does not recurse into the directories listed at the given path.
 *
 * Example:
 *  dir/          list(..., "dir/", ...) lists "dir/a", "dir/b", "dir/c/" and "dir/e/"
 *   ├── a
 *   ├── b
 *   ├── c/
 *   │   └── d
 *   └── e/
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive. If the entry is a symlink, it must be resolved to its linked-to entry.
 * @param entries An array of char arrays, each one is long enough to contain a tar entry path.
 * @param no_entries An in-out argument.
 *                   The caller set it to the number of entries in `entries`.
 *                   The callee set it to the number of entries listed.
 *
 * @return zero if no directory at the given path exists in the archive,
 *         any other value otherwise.
 */
int list(int tar_fd, char *path, char **entries, size_t *no_entries)
{
    tar_header_t *header = get_header(tar_fd, path);
    lseek(tar_fd, 0, SEEK_SET);
    if (header != NULL && header->typeflag == SYMTYPE)
    {
        header = follow_symlinks(tar_fd, header);
        path = header->name;
    }

    if (header == NULL || header->typeflag != DIRTYPE)
    {
        lseek(tar_fd, 0, SEEK_SET);
        return 0;
    }

    int path_len = strlen(path);
    int count = 0;

    for (int i = 0; i < *no_entries; i++)
    {
        header_result_t *header_result = next_valid_header(tar_fd);
        if (header_result == NULL)
            continue;

        header = &(header_result->header);

        if (strncmp(header->name, path, path_len) == 0)
        {
            // char *entry = (header->name) + path_len;
            char *entry = header->name;
            DEBUG_FMT("entry: %s", entry);

            // Skip the directory itself
            if (strcmp(entry, path) == 0)
                continue;

            // Skip files in subdirectories
            char *slash = strchr(entry + path_len + 1, '/');
            if (slash != NULL)
            {
                if (slash[1] != '\0')
                    continue;
            }



            strncpy(entries[count], entry, strlen(entry) + 1);
            count++;
        }
        skip_file_content(tar_fd, header);
    }

    *no_entries = count;
    lseek(tar_fd, 0, SEEK_SET);
    return count;
}

/**
 * Reads a file at a given path in the archive.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive to read from.  If the entry is a symlink, it must be resolved to its linked-to entry.
 * @param offset An offset in the file from which to start reading from, zero indicates the start of the file.
 * @param dest A destination buffer to read the given file into.
 * @param len An in-out argument.
 *            The caller set it to the size of dest.
 *            The callee set it to the number of bytes written to dest.
 *
 * @return -1 if no entry at the given path exists in the archive or the entry is not a file,
 *         -2 if the offset is outside the file total length,
 *         zero if the file was read in its entirety into the destination buffer,
 *         a positive value if the file was partially read, representing the remaining bytes left to be read to reach
 *         the end of the file.
 *
 */
ssize_t read_file(int tar_fd, char *path, size_t offset, uint8_t *dest, size_t *len)
{
    tar_header_t *header = get_header(tar_fd, path);
    if (header == NULL || header->typeflag == DIRTYPE)
    {
        lseek(tar_fd, 0, SEEK_SET);
        return -1;
    }

    if (header->typeflag == SYMTYPE)
    {
        header = follow_symlinks(tar_fd, header);
    }
    if (header == NULL || header->typeflag == DIRTYPE)
    {
        lseek(tar_fd, 0, SEEK_SET);
        return -1;
    }

    int size = TAR_INT(header->size);
    if (offset >= size)
    {
        lseek(tar_fd, 0, SEEK_SET);
        return -2;
    }
    lseek(tar_fd, offset, SEEK_CUR);

    int read_size = size - offset;
    int rest = read_size - *len;
    if (rest < 0)
    {
        *len = read_size;
        rest = 0;
    }

    read(tar_fd, dest, *len);
    lseek(tar_fd, 0, SEEK_SET);
    return rest;
}