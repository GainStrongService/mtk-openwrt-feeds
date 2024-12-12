// SPDX-License-Identifier: BSD-3-Clause
/*
 * Author: Weijie Gao <hackpascal@gmail.com>
 *
 * Utility for restoring file time from snapshot
 */

#define _DEFAULT_SOURCE
#define _FILE_OFFSET_BITS			64

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <malloc.h>
#include <getopt.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "ftcommon.h"

static char *base_dir;
static const char *snapfile;

static void usage(FILE *con, const char *progname, int exitcode)
{
	size_t proglen = strlen(progname);
	const char *prog = progname + proglen - 1;

	while (prog > progname) {
		if (*prog == '/' || *prog == '\\') {
			prog++;
			break;
		}

		prog--;
	}

	printf(
		"File time restoring utility\n"
		"\n"
		"Usage: %s [options]\n"
		"\n"
		"Options:\n"
		"    -b <dir>    Base directory for restoring\n"
		"    -f <file>   Snapshot file\n"
		"    -h          Show this help\n",
		prog
	);

	exit(exitcode);
}

static bool parse_arg(int argc, char *argv[])
{
	static const char optstr[] = "b:f:h";
	size_t len;
	int opt;

	if (argc <= 1)
		usage(stdout, argv[0], 0);

	while ((opt = getopt(argc, argv, optstr)) != -1) {
		switch (opt) {
		case 'b':
			if (base_dir)
				free(base_dir);

			len = strlen(optarg);

			base_dir = calloc(1, len + 2);
			if (!base_dir) {
				fprintf(stderr, "No memory for setting base directory\n");
				return false;
			}

			memcpy(base_dir, optarg, len);

			if (base_dir[len - 1] != '/')
				base_dir[len] = '/';

			break;
		case 'f':
			snapfile = optarg;
			break;
		case 'h':
			usage(stdout, argv[0], 0);
			break;
		default:
			usage(stderr, argv[0], 1);
			break;
		}
	}

	if (!snapfile) {
		fprintf(stderr, "Error: snapshot file not specified\n");
		return false;
	}

	return true;
}

static int restore_file_time(const char *file, uint64_t size, uint64_t mtime, const char *sha1md)
{
	char mdstr[EVP_MAX_MD_SIZE * 2 + 1];
	uint8_t md[EVP_MAX_MD_SIZE];
	struct timespec new_times[2];
	uint64_t curr_mtime;
	int ret = 0, fd;
	struct stat st;

	fd = open(file, O_RDONLY);
	if (fd < 0) {
		ret = errno;
		if (ret == ENOENT)
			return 0;

		fprintf(stderr, "open() for '%s' failed with %u: %s\n", file, ret, strerror(ret));
		return -ret;
	}

	if (fstat(fd, &st) < 0) {
		ret = errno;
		fprintf(stderr, "fstat() for '%s' failed with %u: %s\n", file, ret, strerror(ret));
		goto out;
	}

	if (S_ISDIR(st.st_mode))
		goto out;

	if (st.st_size != size)
		goto out;

	ret = calc_file_sha1(fd, size, file, md);
	if (ret < 0)
		goto out;

	hash2str(md, ret, mdstr);

	if (strcmp(sha1md, mdstr))
		goto out;

	curr_mtime = st.st_mtim.tv_nsec + st.st_mtim.tv_sec * 1000000000;
	if (curr_mtime == mtime)
		goto out;

	new_times[0].tv_sec = 0;
	new_times[0].tv_nsec = UTIME_OMIT;

	new_times[1].tv_sec = mtime / 1000000000;
	new_times[1].tv_nsec = mtime % 1000000000;

	if (futimens(fd, new_times) < 0) {
		ret = errno;
		fprintf(stderr, "futimens() for '%s' failed with %u: %s\n", file, ret, strerror(ret));
	}

out:
	close(fd);

	return ret;
}

static int process_snapshots(void)
{
	char *buf, *p, *next, *file, *sha1md, *abspath;
	uint64_t file_size, mtime;
	size_t len;
	int ret;

	ret = read_file(snapfile, (void **)&buf, &len);
	if (ret)
		return ret;

	/* parse file content */
	p = buf;

	while (*p && p < buf + len) {
		next = p;
		while (*next && *next != '\r' && *next != '\n' && next < buf + len)
			next++;

		while ((*next == '\r' || *next == '\n') && next < buf + len) {
			*next = 0;
			next++;
		}

		file = p;

		p = strchr(file, '\t');
		if (!p)
			goto _next;

		*p = 0;

		file_size = strtoull(p + 1, &p, 10);
		if (file_size == ULLONG_MAX && errno == ERANGE)
			goto _next;

		if (*p != '\t')
			goto _next;

		mtime = strtoull(p + 1, &p, 10);
		if (mtime == ULLONG_MAX && errno == ERANGE)
			goto _next;

		if (*p != '\t')
			goto _next;

		sha1md = p + 1;

		abspath = get_absolute_path(base_dir, file);
		if (!abspath) {
			ret = -ENOMEM;
			goto out;
		}

		restore_file_time(abspath, file_size, mtime, sha1md);

		free(abspath);

	_next:
		p = next;
	}

out:
	free(buf);

	return ret;
}

int main(int argc, char *argv[])
{
	if (!parse_arg(argc, argv))
		return 1;

	if (process_snapshots())
		return 2;

	return 0;
}
