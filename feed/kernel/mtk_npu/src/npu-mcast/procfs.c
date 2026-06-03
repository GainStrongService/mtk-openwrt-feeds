// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/device.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/version.h>

#include "npu/procfs.h"

#include "npu-mcast/internal.h"
#include "npu-mcast/mcast.h"
#include "npu-mcast/mcast-statistic.h"
#include "npu-mcast/procfs.h"

#define FONT_RESET		"\x1B[0m"
#define FONT_YELLOW_BOLD	"\x1B[1;33m"
#define FONT_GREEN_BOLD		"\x1B[1;32m"

static struct proc_dir_entry *mcast_proc_root;
static u32 group_show_id = NPU_MCAST_TBL_IDX_MAX;

static int bss_show(struct seq_file *s, void *private)
{
	seq_puts(s, FONT_YELLOW_BOLD "Multicast BSS Info\n" FONT_RESET);

	mtk_npu_mcast_bss_show(s);

	return 0;
}

static int bss_open(struct inode *inode, struct file *file)
{
	return single_open(file, bss_show, file->private_data);
}

static int group_show(struct seq_file *s, void *private)
{
	u32 i;

	seq_puts(s, FONT_YELLOW_BOLD "Multicast Group Info\n" FONT_RESET);

	if (group_show_id < NPU_MCAST_TBL_IDX_MAX) {
		mtk_npu_mcast_grp_show(s, group_show_id);
	} else {
		for (i = 0; i < NPU_MCAST_TBL_IDX_MAX; i++)
			mtk_npu_mcast_grp_show(s, i);
	}

	return 0;
}

static int group_open(struct inode *inode, struct file *file)
{
	return single_open(file, group_show, file->private_data);
}

static ssize_t group_write(struct file *file, const char __user *ubuf,
			   size_t count, loff_t *ppos)
{
	u32 idx;

	if (kstrtou32_from_user(ubuf, count, 10, &idx))
		return -EINVAL;

	if (idx >= NPU_MCAST_TBL_IDX_MAX)
		return -EINVAL;

	group_show_id = idx;

	return count;
}

static int statistic_show(struct seq_file *s, void *private)
{
	struct npu_mcast_statistic *sta = mtk_npu_mcast_statistic_read();
	bool en = mtk_npu_mcast_statistic_is_enabled();
	u32 i, j;

	if (!sta || !en)
		return 0;

	seq_puts(s, FONT_YELLOW_BOLD "Multicast Statistic\n" FONT_RESET);
	for (i = 0; i < NPU_MCAST_TBL_IDX_MAX; i++) {
		seq_printf(s, FONT_GREEN_BOLD "Group%02u" FONT_RESET " packet count: %6u\n",
			   i, sta->grps[i].pkt_cnt);

		for (j = 0; j < NPU_MCAST_CLIENT_MAX; j++) {
			seq_printf(s, "Client%02u packet count: %6u\t",
				   j, sta->grps[i].clients[j].pkt_cnt);
			if ((j + 1) % 4 == 0)
				seq_puts(s, "\n");
		}
		seq_puts(s, "\n");
	}

	return 0;
}

static int statistic_open(struct inode *inode, struct file *file)
{
	return single_open(file, statistic_show, file->private_data);
}

#if KERNEL_VERSION(5, 6, 0) > LINUX_VERSION_CODE
static const struct file_operations bss_proc_ops = {
	.owner = THIS_MODULE,
	.open = bss_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations group_proc_ops = {
	.owner = THIS_MODULE,
	.open = group_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = group_write,
	.release = single_release,
};

static const struct file_operations statistic_proc_ops = {
	.owner = THIS_MODULE,
	.open = statistic_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#else /* KERNEL_VERSION(5, 6, 0) <= LINUX_VERSION_CODE */
static const struct proc_ops bss_proc_ops = {
	.proc_open = bss_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static const struct proc_ops group_proc_ops = {
	.proc_open = group_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_write = group_write,
	.proc_release = single_release,
};

static const struct proc_ops statistic_proc_ops = {
	.proc_open = statistic_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#endif /* KERNEL_VERSION(5, 6, 0) > LINUX_VERSION_CODE */

int mtk_npu_mcast_procfs_init(void)
{
	struct proc_dir_entry *entry;

	mcast_proc_root = proc_mkdir("multicast", npu_proc_root);
	if (!mcast_proc_root)
		return -ENOMEM;

	entry = proc_create("group", S_IFREG | 0664,
			  mcast_proc_root, &group_proc_ops);
	if (!entry)
		goto err_rm_mcast_dir;

	entry = proc_create("bss", S_IFREG | 0444, mcast_proc_root, &bss_proc_ops);
	if (!entry)
		goto err_rm_group;

	entry = proc_create("statistic", S_IFREG | 0444, mcast_proc_root, &statistic_proc_ops);
	if (!entry)
		goto err_rm_bss;

	return 0;

err_rm_bss:
	remove_proc_entry("bss", mcast_proc_root);

err_rm_group:
	remove_proc_entry("group", mcast_proc_root);

err_rm_mcast_dir:
	remove_proc_entry("mcast", npu_proc_root);

	return -ENOMEM;
}

void mtk_npu_mcast_procfs_deinit(void)
{
	remove_proc_entry("statistic", mcast_proc_root);
	remove_proc_entry("bss", mcast_proc_root);
	remove_proc_entry("group", mcast_proc_root);
	remove_proc_entry("multicast", npu_proc_root);
}
