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
#include <linux/kernel.h>
#include <crypto/aead.h>

#include "fips.h"

static ssize_t mtkcrypto_tagsize_write(struct file *file, const char __user *buffer,
					  size_t count, loff_t *data)
{
	char buf[4];
	int ret;

	if (count > 3) {
		pr_err("Wrong param number\n");
		return -EINVAL;
	}

	copy_from_user(buf, buffer, count);
	ret = kstrtou32(buf, 0, &authsize);
	if (ret) {
		pr_notice("Failed for string transform.\n");
		return -EINVAL;
	}

	return count;
}

static int mtkcrypto_tagsize_read(struct seq_file *s, void *private)
{
	seq_printf(s, "authsize: %u\n", authsize);
	return 0;
}

static int mtkcrypto_tagsize_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtkcrypto_tagsize_read, file->private_data);
}

static const struct file_operations mtkcrypto_tagsize_ops = {
	.open = mtkcrypto_tagsize_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = mtkcrypto_tagsize_write,
	.release = single_release,
};

void mtkcrypto_tagsize_init(void)
{
	debugfs_create_file("tagsize", 0444, debug_root, NULL,
				&mtkcrypto_tagsize_ops);
}
