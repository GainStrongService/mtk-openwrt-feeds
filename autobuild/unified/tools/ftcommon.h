/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Author: Weijie Gao <hackpascal@gmail.com>
 *
 * Common part of file time snapshot
 */

#ifndef _FTCOMMON_H_
#define _FTCOMMON_H_

#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>

#include <openssl/evp.h>

char *path_concat(bool end_sep, size_t extra_len, const char *base, ...);
char *get_absolute_path(const char *base, const char *path);

int calc_file_sha1(int fd, uint64_t size, const char *file, uint8_t *outmd);

int read_file(const char *file, void **outbuf, size_t *retsize);

void hash2str(const void *hash, size_t len, char *buf);

#endif /* _FTCOMMON_H_ */
