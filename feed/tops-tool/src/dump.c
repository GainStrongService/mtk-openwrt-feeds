// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Alvin Kuo <Alvin.Kuo@mediatek.com>
 */

#include <limits.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <poll.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "dump.h"

static int time_to_str(time_t *time_sec, char *time_str, unsigned int time_str_size)
{
	struct tm *ptm;
	int ret;

	ptm = gmtime(time_sec);
	if (!ptm)
		return -1;

	ret = strftime(time_str, time_str_size, "%Y%m%d%H%M%S", ptm);
	if (!ret)
		return -2;

	return 0;
}

static int save_dump_data(char *dump_root_dir,
			  struct dump_data_header *dd_hdr,
			  char *dd)
{
	size_t dump_file_size = dd_hdr->info.size + sizeof(struct dump_info);
	char dump_time_str[32];
	struct stat st = { 0 };
	char *dump_file = NULL;
	char *dump_dir = NULL;
	int ret = 0;
	int fd;

	ret = time_to_str((time_t *)&dd_hdr->info.dump_time_sec,
			  dump_time_str, sizeof(dump_time_str));
	if (ret < 0) {
		fprintf(stderr,
			DUMP_LOG_FMT("time_to_str(%lu) fail(%d)\n"),
			dd_hdr->info.dump_time_sec, ret);
		ret = -1;
		goto out;
	}

	dump_dir = malloc(strlen(dump_root_dir) + 1 +
			  strlen(dump_time_str) + 1);
	if (!dump_dir) {
		ret = -ENOMEM;
		goto out;
	}
	sprintf(dump_dir, "%s/%s", dump_root_dir, dump_time_str);

	dump_file = malloc(strlen(dump_dir) + 1 +
			   strlen(dd_hdr->info.name) + 1);
	if (!dump_file) {
		ret = -ENOMEM;
		goto free_dump_dir;
	}
	sprintf(dump_file, "%s/%s", dump_dir, dd_hdr->info.name);

	if (stat(dump_dir, &st)) {
		ret = mkdir(dump_dir, 0775);
		if (ret) {
			fprintf(stderr,
				DUMP_LOG_FMT("mkdir(%s) fail(%s)\n"),
				dump_dir, strerror(errno));
			ret = -1;
			goto free_dump_file;
		}

		/* TODO: only keep latest three dump directories */
	}

	fd = open(dump_file, 0664);
	if (fd < 0) {
		fprintf(stderr,
			DUMP_LOG_FMT("open(%s) fail(%s)\n"),
			dump_file, strerror(errno));
		ret = -1;
		goto free_dump_file;
	}

	/* write information of dump at begining of the file */
	lseek(fd, 0, SEEK_SET);
	write(fd, &dd_hdr->info, sizeof(struct dump_info));

	/* write data of dump start from data offset of the file */
	lseek(fd, dd_hdr->data_offset, SEEK_CUR);
	write(fd, dd, dd_hdr->data_len);

	close(fd);

	if (dd_hdr->last_frag) {
		stat(dump_file, &st);
		if ((size_t)st.st_size != dump_file_size) {
			fprintf(stderr,
				DUMP_LOG_FMT("file(%s) size %zu != %zu\n"),
				dump_file, st.st_size, dump_file_size);
			ret = -1;
			goto free_dump_file;
		}
	}

free_dump_file:
	free(dump_file);
	dump_file = NULL;

free_dump_dir:
	free(dump_dir);
	dump_dir = NULL;

out:
	return ret;
}

static int read_retry(int fd, void *buf, int len)
{
	int out_len = 0;
	int ret;

	while (len > 0) {
		ret = read(fd, buf, len);
		if (ret < 0) {
			if (errno == EINTR || errno == EAGAIN)
				continue;

			return -1;
		}

		if (!ret)
			return 0;

		out_len += ret;
		len -= ret;
		buf += ret;
	}

	return out_len;
}

static int mkdir_p(char *path, mode_t mode)
{
	struct stat st = { 0 };
	char *cpy_path = NULL;
	char *cur_path = NULL;
	char *tmp_path = NULL;
	int ret = 0;
	char *dir;

	cpy_path = malloc(strlen(path) + 1);
	if (!cpy_path) {
		ret = -ENOMEM;
		goto out;
	}
	strcpy(cpy_path, path);

	cur_path = malloc(strlen(path) + 1);
	if (!cur_path) {
		ret = -ENOMEM;
		goto free_cpy_path;
	}
	memset(cur_path, 0, strlen(path) + 1);

	for (dir = strtok(cpy_path, "/");
	     dir != NULL;
	     dir = strtok(NULL, "/")) {
		/* keep current path */
		tmp_path = malloc(strlen(cur_path) + 1);
		if (!tmp_path) {
			ret = -ENOMEM;
			goto free_cur_path;
		}
		strcpy(tmp_path, cur_path);

		/* append directory in current path */
		sprintf(cur_path, "%s/%s", tmp_path, dir);

		free(tmp_path);
		tmp_path = NULL;

		if (stat(cur_path, &st)) {
			ret = mkdir(cur_path, mode);
			if (ret) {
				fprintf(stderr,
					DUMP_LOG_FMT("mkdir(%s) fail(%s)\n"),
					cur_path, strerror(errno));
				goto free_cur_path;
			}
		}
	}

free_cur_path:
	free(cur_path);
	cur_path = NULL;

free_cpy_path:
	free(cpy_path);
	cpy_path = NULL;

out:
	return ret;
}

int tops_save_dump_data(char *dump_root_dir)
{
	struct stat st = { 0 };
	int ret = 0;
	int fd;

	if (!dump_root_dir) {
		ret = -1;
		goto out;
	}

	/* reserve 256 bytes for saving name of dump directory and dump file */
	if (strlen(dump_root_dir) > (PATH_MAX - 256)) {
		fprintf(stderr,
			DUMP_LOG_FMT("dump_root_dir(%s) length %zu > %u\n"),
			dump_root_dir, strlen(dump_root_dir), PATH_MAX - 256);
		return -1;
	}

	if (stat(dump_root_dir, &st)) {
		ret = mkdir_p(dump_root_dir, 0775);
		if (ret) {
			fprintf(stderr,
				DUMP_LOG_FMT("mkdir_p(%s) fail(%d)\n"),
				dump_root_dir, ret);
			ret = -1;
			goto out;
		}
	}

	fd = open(DUMP_DATA_PATH, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr,
			DUMP_LOG_FMT("open(%s) fail(%s)\n"),
			DUMP_DATA_PATH, strerror(errno));
		ret = -1;
		goto out;
	}

	while (1) {
		char dd[RELAY_DUMP_SUBBUF_SIZE - sizeof(struct dump_data_header)];
		struct dump_data_header dd_hdr;
		struct pollfd pfd = {
			.fd = fd,
			.events = POLLIN | POLLHUP | POLLERR,
		};

		poll(&pfd, 1, -1);

		ret = read_retry(fd, &dd_hdr, sizeof(struct dump_data_header));
		if (ret < 0) {
			fprintf(stderr,
				DUMP_LOG_FMT("read dd_hdr fail(%d)\n"), ret);
			ret = -1;
			break;
		}

		if (!ret)
			continue;

		if (dd_hdr.data_len == 0) {
			fprintf(stderr,
				DUMP_LOG_FMT("read empty data\n"));
			continue;
		}

		if (dd_hdr.data_len > sizeof(dd)) {
			fprintf(stderr,
				DUMP_LOG_FMT("data length %u > %lu\n"),
				dd_hdr.data_len, sizeof(dd));
			ret = -1;
			break;
		}

		ret = read_retry(fd, dd, dd_hdr.data_len);
		if (ret < 0) {
			fprintf(stderr,
				DUMP_LOG_FMT("read dd fail(%d)\n"), ret);
			ret = -1;
			break;
		}

		if ((uint32_t)ret != dd_hdr.data_len) {
			fprintf(stderr,
				DUMP_LOG_FMT("read dd length %u != %u\n"),
				(uint32_t)ret, dd_hdr.data_len);
			ret = -1;
			break;
		}

		ret = save_dump_data(dump_root_dir, &dd_hdr, dd);
		if (ret) {
			fprintf(stderr,
				DUMP_LOG_FMT("save_dump_data(%s) fail(%d)\n"),
				dump_root_dir, ret);
			break;
		}
	}

	close(fd);

out:
	return ret;
}
