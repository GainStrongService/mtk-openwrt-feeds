// SPDX-License-Identifier: BSD-3-Clause
/*
 * Author: Weijie Gao <hackpascal@gmail.com>
 *
 * Utility for generating file time snapshot
 */

#define _DEFAULT_SOURCE
#define _FILE_OFFSET_BITS			64

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <malloc.h>
#include <getopt.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <fnmatch.h>

#include "ftcommon.h"

#define PTRLIST_CAPACITY_INCREMENT		10

struct ptr_list {
	void **ptrs;
	uint32_t capacity;
	uint32_t used;
};

static char *base_dir;
static const char *outfile;

static struct ptr_list snap_dirs;
static struct ptr_list snap_files;
static struct ptr_list snap_lists;
static struct ptr_list snap_items;
static struct ptr_list exclude_patterns;

static bool ptr_list_add(struct ptr_list *list, void *ptr)
{
	uint32_t new_capacity;
	void **new_ptrs;

	if (list->used == list->capacity) {
		new_capacity = list->capacity + PTRLIST_CAPACITY_INCREMENT;
		new_ptrs = realloc(list->ptrs, sizeof(void *) * new_capacity);
		if (!new_ptrs)
			return false;

		list->ptrs = new_ptrs;
		list->capacity = new_capacity;
	}

	list->ptrs[list->used++] = ptr;
	return true;
}

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
		"File time snapshot utility\n"
		"\n"
		"Usage: %s [options] <outfile>\n"
		"\n"
		"Options:\n"
		"    -b <dir>    Base directory for files\n"
		"    -d <dir>    Directory for generating denendency list\n"
		"                This can be specified more than once\n"
		"    -l <list>   List of files to be added to denendency list\n"
		"                This can be specified more than once\n"
		"    -f <file>   File to be added to denendency list\n"
		"                This can be specified more than once\n"
		"    -x <pat>    Path pattern to be excluded\n"
		"                This can be specified more than once\n"
		"    -h          Show this help\n",
		prog
	);

	exit(exitcode);
}

static bool parse_arg(int argc, char *argv[])
{
	static const char optstr[] = "b:d:l:f:x:h";
	struct stat st;
	int opt, ret;
	size_t len;

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
		case 'd':
			if (!ptr_list_add(&snap_dirs, optarg)) {
				fprintf(stderr, "No memory for adding directory\n");
				return false;
			}
			break;
		case 'l':
			if (!ptr_list_add(&snap_lists, optarg)) {
				fprintf(stderr, "No memory for adding file list\n");
				return false;
			}
			break;
		case 'f':
			ret = stat(optarg, &st);
			if (ret < 0) {
				ret = errno;

				if (ret == ENOENT)
					break;

				fprintf(stderr, "stat() failed with %u for %s: %s\n", ret, optarg, strerror(ret));
				return false;
			}

			if (!(st.st_mode & (S_IFLNK | S_IFREG))) {
				fprintf(stderr, "%s is not either a regular file nor a symbolic link\n", optarg);
				return false;
			}

			if (!ptr_list_add(&snap_files, optarg)) {
				fprintf(stderr, "No memory for adding file\n");
				return false;
			}
			break;
		case 'x':
			if (!ptr_list_add(&exclude_patterns, optarg)) {
				fprintf(stderr, "No memory for excluding pattern\n");
				return false;
			}
			break;
		case 'h':
			usage(stdout, argv[0], 0);
			break;
		default:
			usage(stderr, argv[0], 1);
			break;
		}
	}

	if (!snap_dirs.used && !snap_lists.used && !snap_files.used) {
		fprintf(stderr, "Error: No directory/file specified\n");
		return false;
	}

	if (argc <= optind) {
		fprintf(stderr, "Error: output file not specified\n");
		return false;
	}

	outfile = argv[optind];

	return true;
}

static int excluded(const char *path, bool is_dir)
{
	int ret, flags = 0;
	const char *pat;
	uint32_t i;

	if (!is_dir)
		flags = FNM_PERIOD | FNM_PATHNAME;

	for (i = 0; i < exclude_patterns.used; i++) {
		pat = exclude_patterns.ptrs[i];

		ret = fnmatch(pat, path, flags);
		if (!ret)
			return 0;

		if (ret == FNM_NOMATCH)
			continue;

		fprintf(stderr, "Invalid pattern %s\n", pat);
		return ret;
	}

	return FNM_NOMATCH;
}

static int enum_dir(const char *dir, bool *retempty)
{
	bool is_empty = true, sub_is_empty;
	char *fullpath, *absdir;
	struct dirent *dent;
	int err, ret = 0;
	size_t dirlen;
	DIR *d;

	absdir = get_absolute_path(base_dir, dir);
	if (!absdir)
		return -ENOMEM;

	d = opendir(absdir);
	free(absdir);

	if (!d) {
		if (retempty)
			*retempty = true;

		err = errno;
		if (err == ENOENT)
			return 0;

		fprintf(stderr, "opendir() failed with %u: %s\n", err, strerror(err));
		return -err;
	}

	errno = 0;
	dent = readdir(d);

	if (!dent) {
		err = errno;
		if (!err)
			goto cleanup;

		fprintf(stderr, "Initial readdir() failed with %u: %s\n", err, strerror(err));
		ret = -1;
		goto cleanup;
	}

	do {
		if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))
			goto next_ent;

		is_empty = false;

		/* Check only the name first */
		ret = excluded(dent->d_name, false);
		if (!ret)
			goto next_ent;
		else if (ret < 0)
			goto cleanup;

		dirlen = strlen(dir);

		if (dir[dirlen - 1] == '/')
			fullpath = path_concat(false, 1, dir, dent->d_name, NULL);
		else
			fullpath = path_concat(false, 1, dir, "", dent->d_name, NULL);

		/* Check the full path */
		ret = excluded(fullpath, dent->d_type == DT_DIR);
		if (!ret) {
			free(fullpath);
			goto next_ent;
		} else if (ret < 0) {
			free(fullpath);
			goto cleanup;
		}

		/* Append '/' for directory */
		if (dent->d_type == DT_DIR) {
			dirlen = strlen(fullpath);
			fullpath[dirlen] = '/';
			fullpath[dirlen + 1] = 0;
		}

		if (dent->d_type == DT_DIR)
			ret = enum_dir(fullpath, &sub_is_empty);
		else
			ret = 0;

		if (dent->d_type != DT_DIR || (dent->d_type == DT_DIR && sub_is_empty)) {
			if (!ptr_list_add(&snap_items, fullpath)) {
				fprintf(stderr, "No memory for path list\n");
				free(fullpath);
				goto cleanup;
			}
		}

	next_ent:
		errno = 0;
		if (!ret)
			dent = readdir(d);
	} while (!ret && dent);

	if (!dent) {
		err = errno;
		if (err) {
			fprintf(stderr, "readdir() failed with %u: %s\n", err, strerror(err));
			ret = -1;
		}
	}

cleanup:
	closedir(d);

	if (retempty)
		*retempty = is_empty;

	return ret;
}

static int parse_list(const char *list_file)
{
	char *buf, *p, *next, *path, *abspath;
	struct stat st;
	size_t len;
	int ret;

	ret = read_file(list_file, (void **)&buf, &len);
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

		abspath = get_absolute_path(base_dir, p);
		if (!abspath)
			return -ENOMEM;

		ret = stat(abspath, &st);
		free(abspath);

		if (ret < 0) {
			ret = errno;
			if (ret == ENOENT)
				goto _next;

			fprintf(stderr, "stat() for '%s' failed with %u: %s\n", abspath, ret, strerror(ret));
			free(buf);
			return -ret;
		}

		if (S_ISDIR(st.st_mode)) {
			ret = enum_dir(p, NULL);
			if (ret) {
				free(buf);
				return ret;
			}

			goto _next;
		}

		path = strdup(p);
		if (!path) {
			fprintf(stderr, "No memory for file list processing\n");
			free(buf);
			return -ENOMEM;
		}

		if (!ptr_list_add(&snap_items, path)) {
			fprintf(stderr, "No memory for path list\n");
			free(buf);
			return -ENOMEM;
		}

	_next:
		p = next;
	}

	free(buf);
	return 0;
}

static int path_sort_compare(const void *a, const void *b)
{
	return strcmp(*(const char **)a, *(const char **)b);
}

static int create_item_list(void)
{
	char *filepath;
	uint32_t i;
	int ret;

	for (i = 0; i < snap_dirs.used; i++) {
		ret = enum_dir(snap_dirs.ptrs[i], NULL);
		if (ret)
			return ret;
	}

	for (i = 0; i < snap_lists.used; i++) {
		ret = parse_list(snap_lists.ptrs[i]);
		if (ret)
			return ret;
	}

	for (i = 0; i < snap_files.used; i++) {
		filepath = strdup(snap_files.ptrs[i]);
		if (!filepath) {
			fprintf(stderr, "No memory for file path\n");
			return -ENOMEM;
		}

		if (!ptr_list_add(&snap_items, filepath)) {
			fprintf(stderr, "No memory for path list\n");
			free(filepath);
			return -ENOMEM;
		}
	}

	qsort(snap_items.ptrs, snap_items.used, sizeof(snap_items.ptrs[0]), path_sort_compare);

	return 0;
}

static void print_path(FILE *f, const char *path)
{
	while (*path) {
		switch (*path) {
		case '\\':
			fprintf(f, "\\\\");
			break;

		case ' ':
			fprintf(f, "\\ ");
			break;

		default:
			fputc(*path, f);
		}

		path++;
	}
}

static int print_file_sha1(FILE *fout, int fd, uint64_t size, const char *file)
{
	char mdstr[EVP_MAX_MD_SIZE * 2 + 1];
	uint8_t md[EVP_MAX_MD_SIZE];
	int ret;

	ret = calc_file_sha1(fd, size, file, md);
	if (ret < 0)
		return ret;

	hash2str(md, ret, mdstr);

	fprintf(fout, "%s", mdstr);

	return 0;
}

static int process_items(void)
{
	char *abspath = NULL;
	const char *path;
	int fd, ret = 0;
	struct stat st;
	uint32_t i;
	FILE *f;

	f = fopen(outfile, "wb");
	if (!f) {
		fprintf(stderr, "Unable to open '%s' for write\n", outfile);
		return errno;
	}

	for (i = 0; i < snap_items.used; i++) {
		path = snap_items.ptrs[i];

		abspath = get_absolute_path(base_dir, path);
		if (!abspath) {
			ret = ENOMEM;
			goto cleanup;
		}

		fd = open(abspath, O_RDONLY);
		if (fd < 0) {
			ret = errno;
			if (ret == ENOENT)
				goto _next;

			fprintf(stderr, "open() for '%s' failed with %u: %s\n", abspath, ret, strerror(ret));
			goto cleanup;
		}

		if (fstat(fd, &st) < 0) {
			ret = errno;
			fprintf(stderr, "fstat() for '%s' failed with %u: %s\n", abspath, ret, strerror(ret));
			close(fd);
			goto cleanup;
		}

		if (S_ISDIR(st.st_mode))
			continue;

		print_path(f, path);
		fprintf(f, "\t");

		fprintf(f, "%" PRIu64 "\t", st.st_size);
		fprintf(f, "%" PRIu64 "\t", st.st_mtim.tv_nsec + st.st_mtim.tv_sec * 1000000000);

		ret = print_file_sha1(f, fd, st.st_size, abspath);
		close(fd);

		if (ret)
			goto cleanup;

		fprintf(f, "\n");

	_next:
		free(abspath);
		abspath = NULL;
	}

cleanup:
	if (abspath)
		free(abspath);

	fclose(f);
	return -ret;
}

int main(int argc, char *argv[])
{
	if (!parse_arg(argc, argv))
		return 1;

	if (create_item_list())
		return 2;

	if (process_items())
		return 3;

	return 0;
}
