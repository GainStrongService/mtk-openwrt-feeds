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

static ssize_t mtkcrypto_msg_write(struct file *file, const char __user *buffer,
					  size_t count, loff_t *data)
{
	char *buf;
	int i, j;
	int ret;

	if (count == 2) {
		msglen = 0;
		return count;
	}

	if (count > 17001) { // 8500 * 2 + 1
		pr_notice("Msg too big!\n");
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
		ret = sscanf(buf + i, "%2hhx", &text[j]);
		if (ret < 1) {
			pr_notice("Failed for string transform.\n");
			return -EINVAL;
		}
	}

	msglen = (count - 1) / 2;

	kfree(buf);
	return count;
}

static int mtkcrypto_msg_read(struct seq_file *s, void *private)
{
	int i;

	seq_printf(s, "msglen: %u\n", msglen);
	seq_puts(s, "current msg: ");
	for (i = 0; i < msglen; i++)
		seq_printf(s, "%02x", text[i]);
	seq_puts(s, "\n");

	return 0;
}

static int mtkcrypto_msg_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtkcrypto_msg_read, file->private_data);
}

static const struct file_operations mtkcrypto_msg_ops = {
	.open = mtkcrypto_msg_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = mtkcrypto_msg_write,
	.release = single_release,
};

void mtkcrypto_msg_init(void)
{
	debugfs_create_file("msg", 0444, debug_root, NULL,
				&mtkcrypto_msg_ops);
}
