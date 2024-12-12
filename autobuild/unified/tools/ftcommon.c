// SPDX-License-Identifier: BSD-3-Clause
/*
 * Author: Weijie Gao <hackpascal@gmail.com>
 *
 * Common part of file time snapshot
 */

#define _DEFAULT_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "ftcommon.h"

static const char hexchars[] = "0123456789abcdef";

char *path_concat(bool end_sep, size_t extra_len, const char *base, ...)
{
	size_t len, new_len = 0, n = 0;
	char *new_path, *p;
	const char *str;
	va_list args;

	if (!base)
		return NULL;

	new_len = strlen(base);

	va_start(args, base);

	do {
		str = va_arg(args, const char *);
		if (str) {
			new_len += strlen(str) + 1;
			n++;
		}
	} while (str);

	va_end(args);

	new_path = calloc(1, new_len + extra_len + 1);
	if (!new_path) {
		fprintf(stderr, "No memory for path concatenating\n");
		return NULL;
	}

	p = new_path;
	len = strlen(base);
	memcpy(p, base, len);
	p += len;

	va_start(args, base);

	do {
		str = va_arg(args, const char *);
		if (str) {
			len = strlen(str);
			memcpy(p, str, len);
			p += len;
			*p++ = '/';
		}
	} while (str);

	va_end(args);

	if (!end_sep && n)
		p[-1] = 0;

	return new_path;
}

char *get_absolute_path(const char *base, const char *path)
{
	char *abspath;

	if (!base || path[0] == '/')
		return strdup(path);

	abspath = path_concat(false, 0, base, path, NULL);
	if (!abspath)
		fprintf(stderr, "Error: No memory for absolute path\n");

	return abspath;
}

int calc_file_sha1(int fd, uint64_t size, const char *file, uint8_t *outmd)
{
	uint32_t readsize, md_len;
	uint8_t buf[4096];
	ssize_t retsize;
	EVP_MD_CTX *ctx;
	int ret = 0;

#if OPENSSL_VERSION_NUMBER < 0x10100000L
	ctx = EVP_MD_CTX_create();
#else
	ctx = EVP_MD_CTX_new();
#endif
	if (!ctx) {
		fprintf(stderr, "EVP_MD_CTX_new() failed\n");
		return -1;
	}

	if (!EVP_DigestInit_ex(ctx, EVP_sha1(), NULL)) {
		fprintf(stderr, "EVP_DigestInit_ex() failed\n");
		ret = -1;
		goto cleanup;
	}

	lseek(fd, 0, SEEK_SET);

	while (size) {
		readsize = sizeof(buf);
		if (readsize > size)
			readsize = size;

		retsize = read(fd, buf, readsize);
		if (retsize < 0) {
			ret = errno;
			fprintf(stderr, "read() for '%s' failed with %u: %s\n", file, ret, strerror(ret));
			ret = -ret;
			goto cleanup;
		}

		if (!EVP_DigestUpdate(ctx, buf, retsize)) {
			fprintf(stderr, "EVP_DigestUpdate() failed\n");
			ret = -1;
			goto cleanup;
		}

		size -= retsize;
	}

	if (!EVP_DigestFinal_ex(ctx, outmd, &md_len)) {
		fprintf(stderr, "EVP_DigestFinal_ex() failed\n");
		ret = -1;
		goto cleanup;
	}

	ret = md_len;

cleanup:
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	EVP_MD_CTX_destroy(ctx);
#else
	EVP_MD_CTX_free(ctx);
#endif

	return ret;
}

int read_file(const char *file, void **outbuf, size_t *retsize)
{
	size_t currpos;
	struct stat st;
	ssize_t ret;
	int fd, err;
	size_t len;
	char *buf;

	fd = open(file, O_RDONLY);
	if (fd < 0) {
		err = errno;
		fprintf(stderr, "open() for '%s' for read failed with %u: %s\n", file, err, strerror(err));
		return -err;
	}

	if (fstat(fd, &st) < 0) {
		err = errno;
		fprintf(stderr, "fstat() for '%s' failed with %u: %s\n", file, err, strerror(err));
		close(fd);
		return -err;
	}

	if (!S_ISREG(st.st_mode)) {
		fprintf(stderr, "File '%s' is not a regular file\n", file);
		close(fd);
		return -ENOTSUP;
	}

	if (st.st_size > SSIZE_MAX) {
		fprintf(stderr, "File '%s' is too big\n", file);
		close(fd);
		return -E2BIG;
	}

	len = (size_t)st.st_size;
	buf = malloc(len + 1);
	if (!buf) {
		fprintf(stderr, "No memory for reading file '%s'\n", file);
		close(fd);
		return -ENOMEM;
	}

	currpos = 0;
	buf[len] = 0;

	while (currpos < len) {
		ret = read(fd, buf + currpos, len - currpos);
		if (ret < 0) {
			if (errno != EINTR) {
				err = errno;
				fprintf(stderr, "read() for '%s' failed with %u: %s\n", file, err, strerror(err));
				close(fd);
				free(buf);
				return -err;
			}

			ret = 0;
		}

		currpos += ret;
	}

	close(fd);

	*outbuf = buf;

	if (retsize)
		*retsize = len;

	return 0;
}

void hash2str(const void *hash, size_t len, char *buf)
{
	const uint8_t *h = hash;
	size_t i;

	for (i = 0; i < len; i++) {
		buf[2 * i] = hexchars[(h[i] >> 4) & 0xf];
		buf[2 * i + 1] = hexchars[h[i] & 0xf];
	}

	buf[2 * len] = 0;
}
