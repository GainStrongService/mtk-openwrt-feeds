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

static ssize_t mtkcrypto_result_write(struct file *file, const char __user *buffer,
					  size_t count, loff_t *data)
{
	pr_err("Should not write result\n");
	return -EFAULT;
}

static int mtkcrypto_result_read(struct seq_file *s, void *private)
{
	int i;

	seq_printf(s, "resultsize: %d\n", resultsize);
	if (resultsize < 0)
		return 0;

	seq_puts(s, "result: ");
	for (i = 0; i < resultsize; i++)
		seq_printf(s, "%02x", result[i]);
	seq_puts(s, "\n");

	return 0;
}

static int mtkcrypto_result_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtkcrypto_result_read, file->private_data);
}

static const struct file_operations mtkcrypto_result_ops = {
	.open = mtkcrypto_result_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = mtkcrypto_result_write,
	.release = single_release,
};

void mtkcrypto_result_init(void)
{
	debugfs_create_file("result", 0444, debug_root, NULL,
				&mtkcrypto_result_ops);
}
