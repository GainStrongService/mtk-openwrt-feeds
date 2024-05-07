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

static ssize_t mtkcrypto_assoc_write(struct file *file, const char __user *buffer,
					  size_t count, loff_t *data)
{
	char *buf;
	int i, j;
	int ret;

	if (count == 2) {
		alen = 0;
		return count;
	}

	if (count > 17001) { // 8500 * 2 + 1
		pr_notice("assoc too big!\n");
		return -ENOMEM;
	}

	if ((count - 1) % 2 != 0) {
		pr_notice("the input should be multiple of byte.\n");
		return -EINVAL;
	}

	buf = kmalloc(count, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	copy_from_user(buf, buffer, count);
	for (i = 0, j = 0; i < count - 1; i += 2, j++) {
		ret = sscanf(buf + i, "%2hhx", &assoc[j]);
		if (ret < 1) {
			pr_notice("Failed for string transform.\n");
			return -EINVAL;
		}
	}

	alen = (count - 1) / 2;

	kfree(buf);
	return count;
}

static int mtkcrypto_assoc_read(struct seq_file *s, void *private)
{
	int i;

	seq_printf(s, "assoclen: %u\n", alen);
	seq_puts(s, "current assoc: ");
	for (i = 0; i < alen; i++)
		seq_printf(s, "%02x", assoc[i]);
	seq_puts(s, "\n");

	return 0;
}

static int mtkcrypto_assoc_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtkcrypto_assoc_read, file->private_data);
}

static const struct file_operations mtkcrypto_assoc_ops = {
	.open = mtkcrypto_assoc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = mtkcrypto_assoc_write,
	.release = single_release,
};

void mtkcrypto_assoc_init(void)
{
	debugfs_create_file("assoc", 0444, debug_root, NULL,
				&mtkcrypto_assoc_ops);
}
