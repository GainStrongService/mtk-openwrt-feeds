// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 MediaTek Inc.
 *
 * Author: Frank-zj Lin <frank-zj.lin@mediatek.com>
 */

#include <linux/debugfs.h>
#include <linux/spinlock.h>

#include <pce/cdrt.h>

#include "crypto-eip/crypto-eip.h"

struct dentry *mtk_crypto_debugfs_root;

static int mtk_crypto_debugfs_read(struct seq_file *s, void *private)
{
	struct xfrm_params_list *xfrm_params_list;
	struct mtk_xfrm_params *xfrm_params;
	unsigned long flags;

	xfrm_params_list = mtk_xfrm_params_list_get();

	spin_lock_irqsave(&xfrm_params_list->lock, flags);

	list_for_each_entry(xfrm_params, &xfrm_params_list->list, node) {
		seq_printf(s, "XFRM STATE: spi 0x%x, cdrt_idx %3d: ",
			   htonl(xfrm_params->xs->id.spi),
			   xfrm_params->cdrt->idx);

		if (xfrm_params->cdrt->type == CDRT_DECRYPT)
			seq_puts(s, "DECRYPT\n");
		else if (xfrm_params->cdrt->type == CDRT_ENCRYPT)
			seq_puts(s, "ENCRYPT\n");
		else
			seq_puts(s, "\n");
	}

	spin_unlock_irqrestore(&xfrm_params_list->lock, flags);

	return 0;
}

static int mtk_crypto_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtk_crypto_debugfs_read, file->private_data);
}

static ssize_t mtk_crypto_debugfs_write(struct file *file,
					const char __user *ubuf,
					size_t count, loff_t *ppos)
{
	return count;
}

static const struct file_operations mtk_crypto_debugfs_fops = {
	.open = mtk_crypto_debugfs_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = mtk_crypto_debugfs_write,
	.release = single_release,
};

int mtk_crypto_debugfs_init(void)
{
	mtk_crypto_debugfs_root = debugfs_create_dir("mtk_crypto", NULL);

	debugfs_create_file("xfrm_params", 0644, mtk_crypto_debugfs_root, NULL,
			    &mtk_crypto_debugfs_fops);

	return 0;
}

void mtk_crypto_debugfs_exit(void)
{
	debugfs_remove_recursive(mtk_crypto_debugfs_root);
}
