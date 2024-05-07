// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 Mediatek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/device.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/scatterlist.h>
#include <linux/uaccess.h>
#include <crypto/aead.h>

#include "fips.h"

static ssize_t mtkcrypto_iv_write(struct file *file, const char __user *buffer,
					  size_t count, loff_t *data)
{
	char buf[33];
	int i, j;
	int ret;

	switch (count) {
	case 17:
	case 25:
	case 33:
		copy_from_user(buf, buffer, count);
		for (i = 0, j = 0; i < count - 1; i += 2, j++) {
			ret = sscanf(buf + i, "%2hhx", &iv[j]);
			if (ret < 1) {
				pr_notice("Failed for string transform.\n");
				return -EINVAL;
			}
		}
		ivsize = (count - 1) / 2;
		break;
	default:
		pr_err("Invalid IV size: %lu\n", count);
		return -EINVAL;
	}

	return count;
}

static int mtkcrypto_iv_read(struct seq_file *s, void *private)
{
	int i;

	seq_printf(s, "ivsize: %u\n", ivsize);
	seq_puts(s, "current iv: ");
	for (i = 0; i < ivsize; i++)
		seq_printf(s, "%02x", iv[i]);
	seq_puts(s, "\n");

	return 0;
}

static int mtkcrypto_iv_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtkcrypto_iv_read, file->private_data);
}

static const struct file_operations mtkcrypto_iv_ops = {
	.open = mtkcrypto_iv_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = mtkcrypto_iv_write,
	.release = single_release,
};

void mtkcrypto_iv_init(void)
{
	debugfs_create_file("iv", 0444, debug_root, NULL,
				&mtkcrypto_iv_ops);
}
