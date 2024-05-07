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

static ssize_t mtkcrypto_key_write(struct file *file, const char __user *buffer,
					  size_t count, loff_t *data)
{
	char buf[65];
	int i, j;
	int ret;
	/*
	* Though the input is hexdump, the write function will treat it as "string".
	* Thus, for a 16 byte hexdump, count will be 32 since each character is considered
	* as 4 bits. Additionally, there will be a \0 at the end of input, so the count will
	* be 33. For aes key, it may be 16, 24, 32 bytes. So the count could be 33, 49, and 65.
	*/
	switch (count) {
	case 33:
	case 49:
	case 65:
		copy_from_user(buf, buffer, count);
		for (i = 0, j = 0; i < count - 1; i += 2, j++) {
			ret = sscanf(buf + i, "%2hhx", &key[j]);
			if (ret < 1) {
				pr_notice("Failed for string transform.\n");
				return -EINVAL;
			}
		}
		klen = (count - 1) / 2;
		break;
	default:
		pr_err("Invalid key size: %lu, should be 16, 24, or 32 bytes.\n", count);
	return -EINVAL;
	}

	return count;
}

static int mtkcrypto_key_read(struct seq_file *s, void *private)
{
	int i;

	seq_printf(s, "keysize: %u\n", klen);
	seq_puts(s, "current key: ");
	for (i = 0; i < klen; i++)
		seq_printf(s, "%02x", key[i]);
	seq_puts(s, "\n");

	return 0;
}

static int mtkcrypto_key_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtkcrypto_key_read, file->private_data);
}

static const struct file_operations mtkcrypto_key_ops = {
	.open = mtkcrypto_key_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = mtkcrypto_key_write,
	.release = single_release,
};

void mtkcrypto_key_init(void)
{
	debugfs_create_file("key", 0444, debug_root, NULL,
				&mtkcrypto_key_ops);
}
