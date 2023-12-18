#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "lib_tar.h"
#include "lib_tar_internal.h"

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

    for (size_t i = 0; i < *no_entries; i++)
    {
        header_result_t *header_result = next_valid_header(tar_fd);
        if (header_result == NULL)
            continue;

        header = &(header_result->header);

        if (strncmp(header->name, path, path_len) == 0)
        {
            // char *entry = (header->name) + path_len;
            char *entry = header->name;
            // DEBUG_FMT("entry: %s", entry);

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

    size_t size = TAR_INT(header->size);
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