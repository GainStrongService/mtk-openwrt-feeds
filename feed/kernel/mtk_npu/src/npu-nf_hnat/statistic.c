// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/debugfs.h>
#include <linux/types.h>

#include "npu-nf_hnat/internal.h"
#include "npu-nf_hnat/statistic.h"
#include "npu-nf_hnat/tnl-offload.h"

static int tnl_hnat_statistic_show(struct seq_file *s, void *private)
{
	mtk_npu_nf_hnat_statistic_show(s);

	return 0;
}

static int tnl_hnat_statistic_open(struct inode *inode, struct file *file)
{
	return single_open(file, tnl_hnat_statistic_show, file->private_data);
}

static ssize_t tnl_hnat_statistic_write(struct file *file, const char __user *buffer,
					size_t count, loff_t *data)
{
	char cmd[21];

	if (count >= sizeof(cmd))
		return -ENOMEM;

	if (copy_from_user(cmd, buffer, count))
		return -EFAULT;

	cmd[count] = '\0';

	if (!strcmp(cmd, "clean")) {
		mtk_npu_nf_hnat_statistic_clear();
	} else if (!strcmp(cmd, "enable")) {
		mtk_npu_nf_hnat_statistic_enable(true);
	} else if (!strcmp(cmd, "disable")) {
		mtk_npu_nf_hnat_statistic_enable(false);
		mtk_npu_nf_hnat_statistic_clear();
	} else {
		NPU_INFO("write \"clean\" to clear histroy\n");
		NPU_INFO("write \"enable\" to enable tunnel statistic\n");
		NPU_INFO("write \"disable\" to disable tunnel statistic\n");
	}

	return count;
}

static const struct file_operations tnl_hnat_statistic_ops = {
	.open = tnl_hnat_statistic_open,
	.read = seq_read,
	.write = tnl_hnat_statistic_write,
	.llseek = seq_lseek,
	.release = single_release,
};

int mtk_npu_nf_hnat_statistic_init(void)
{
	/*
	 *struct dentry *dir;

	 *dir = debugfs_lookup("npu", NULL);
	 *if (!dir)
	 *	return -ENOENT;

	 *dir = debugfs_lookup("statistic", dir);
	 *if (!dir)
	 *	return -ENOENT;

	 *debugfs_create_file("tnl_hnat", 0444, dir, NULL, &tnl_hnat_statistic_ops);

	 *mtk_npu_nf_hnat_statistic_enable(true);
	 */

	return 0;
}

void mtk_npu_nf_hnat_statistic_deinit(void)
{
	mtk_npu_nf_hnat_statistic_enable(false);
}
