// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Alvin Kuo <alvin.kuog@mediatek.com>
 *         Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/proc_fs.h>
#include <linux/relay.h>
#include <linux/version.h>

#include "npu/debugfs.h"
#include "npu/internal.h"
#include "npu/npu-ocd.h"
#include "npu/procfs.h"
#include "npu/trm-fs.h"
#include "npu/trm.h"

#define RLY_RETRY_NUM				3

static struct rchan *relay;

static int cpu_utilization_show(struct seq_file *s, void *private)
{
	u32 cpu_utilization;
	enum core_id core;
	int ret;

	seq_puts(s, "CPU Utilization:\n");
	for (core = CORE_OFFLOAD_0; core <= CORE_MGMT; core++) {
		ret = mtk_trm_cpu_utilization(core, &cpu_utilization);
		if (ret) {
			if (core <= CORE_OFFLOAD_3) {
				NPU_ERR("fetch Core%d cpu utilization failed(%d)\n",
					 core, ret);
			} else {
				NPU_ERR("fetch CoreM cpu utilization failed(%d)\n",
					 ret);
			}

			return ret;
		}

		if (core <= CORE_OFFLOAD_3)
			seq_printf(s, "Core%d\t\t%u%%\n", core, cpu_utilization);
		else
			seq_printf(s, "CoreM\t\t%u%%\n", cpu_utilization);
	}

	return 0;
}

static int cpu_utilization_open(struct inode *inode, struct file *file)
{
	return single_open(file, cpu_utilization_show, file->private_data);
}

void *mtk_npu_trm_fs_relay_reserve(u32 size)
{
	u32 rty = 0;
	void *dst;

	while (rty < RLY_RETRY_NUM) {
		dst = relay_reserve(relay, size);
		if (likely(dst))
			return dst;

		rty++;
		msleep(100);
	}

	return ERR_PTR(-ENOMEM);
}

void mtk_npu_trm_fs_relay_flush(void)
{
	relay_flush(relay);
}

static struct dentry *trm_fs_create_buf_file_cb(const char *filename,
						struct dentry *parent,
						umode_t mode,
						struct rchan_buf *buf,
						int *is_global)
{
	struct dentry *debugfs_file;

	debugfs_file = debugfs_create_file("dump-data", mode,
					   parent, buf,
					   &relay_file_operations);

	*is_global = 1;

	return debugfs_file;
}

static int trm_fs_remove_buf_file_cb(struct dentry *debugfs_file)
{
	debugfs_remove(debugfs_file);

	return 0;
}

#if KERNEL_VERSION(5, 6, 0) > LINUX_VERSION_CODE
static const struct file_operations cpu_utilization_fops = {
	.open = cpu_utilization_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#else /* KERNEL_VERSION(5, 6, 0) <= LINUX_VERSION_CODE */
static const struct proc_ops cpu_utilization_fops = {
	.proc_open = cpu_utilization_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#endif /* KERNEL_VERSION(5, 6, 0) > LINUX_VERSION_CODE */

/* TODO: still need to refactor, maybe we should move this entire file to debugfs */
int mtk_npu_trm_fs_init(void)
{
	static struct rchan_callbacks relay_cb = {
		.create_buf_file = trm_fs_create_buf_file_cb,
		.remove_buf_file = trm_fs_remove_buf_file_cb,
	};
	struct proc_dir_entry *entry;
	int ret = 0;

	entry = proc_create("cpu-utilization", S_IFREG | 0444,
			    npu_proc_root, &cpu_utilization_fops);
	if (!entry)
		return -ENOMEM;

	if (!relay) {
		relay = relay_open("dump-data", npu_debugfs_root,
				   RLY_DUMP_SUBBUF_SZ,
				   RLY_DUMP_SUBBUF_NUM,
				   &relay_cb, NULL);
		if (!relay)
			return -EINVAL;
	}

	relay_reset(relay);

	return ret;
}

void mtk_npu_trm_fs_deinit(void)
{
	remove_proc_entry("cpu-utilization", npu_proc_root);
	relay_close(relay);
}
