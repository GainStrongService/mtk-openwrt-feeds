// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 MediaTek Inc. All Rights Reserved.
 *
 */

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/version.h>

#include "npu/procfs.h"

#include "npu-dpdk/mac-filter.h"
#include "npu-dpdk/procfs.h"

static int mac_filter_show(struct seq_file *s, void *private)
{
	struct npu_dpdk_mac_info minfo;
	u32 i = 0;
	int ret;

	while (1) {
		ret = mtk_npu_dpdk_mac_filter_info_get_by_ofs(&minfo, i);
		if (ret < 0)
			break;

		seq_printf(s, "MAC filter entry%04u addr: %pM WAN: %u LAN: 0x%03lx WiFi: %u Enabled: %u\n",
			   minfo.idx,
			   minfo.mac.addr,
			   !!(minfo.mac.flag & MAC_FILTER_FLAG_WAN),
			   ((minfo.mac.flag >> MAC_FILTER_FLAG_LAN_LSB) &
			    GENMASK(MAC_FILTER_LAN_PORT_NUM - 1, 0)),
			   !!(minfo.mac.flag & MAC_FILTER_FLAG_WIFI),
			   !!(minfo.mac.flag & MAC_FILTER_FLAG_EN));

		i = ret + 1;
	}

	return 0;
}

static int mac_filter_open(struct inode *inode, struct file *file)
{
	return single_open(file, mac_filter_show, file->private_data);
}

#if KERNEL_VERSION(5, 6, 0) > LINUX_VERSION_CODE
static const struct file_operations mac_filter_proc_ops = {
	.owner = THIS_MODULE,
	.open = mac_filter_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#else /* KERNEL_VERSION(5, 6, 0) <= LINUX_VERSION_CODE */
static const struct proc_ops mac_filter_proc_ops = {
	.proc_open = mac_filter_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#endif /* KERNEL_VERSION(5, 6, 0) > LINUX_VERSION_CODE */

int mtk_npu_dpdk_procfs_init(void)
{
	if (!proc_create("mac_filter", S_IFREG | 0444, npu_proc_root, &mac_filter_proc_ops))
		return -ENOMEM;

	return 0;
}

void mtk_npu_dpdk_procfs_deinit(void)
{
	remove_proc_entry("mac_filter", npu_proc_root);
}
