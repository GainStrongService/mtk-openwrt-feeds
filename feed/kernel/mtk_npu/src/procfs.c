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

#include "npu/internal.h"
#include "npu/procfs.h"
#include "npu/tunnel.h"

struct proc_dir_entry *npu_proc_root;
EXPORT_SYMBOL(npu_proc_root);

static int tnl_show(struct seq_file *s, void *private)
{
	struct npu_tnl_info *tnl_info;
	struct npu_tnl_params *tnl_params;
	u32 i;

	for (i = 0; i < CONFIG_NPU_TNL_NUM; i++) {
		tnl_info = mtk_npu_tnl_info_get_by_idx(i);
		if (IS_ERR(tnl_info))
			/* tunnel not enabled */
			continue;

		tnl_params = &tnl_info->tnl_params;
		if (!tnl_info->tnl_type || !tnl_info->tnl_type->tnl_param_dump)
			continue;

		seq_printf(s, "Tunnel Index: %02u\n", i);

		mtk_npu_mac_param_dump(s, &tnl_params->params);

		mtk_npu_network_param_dump(s, &tnl_params->params);

		mtk_npu_transport_param_dump(s, &tnl_params->params);

		tnl_info->tnl_type->tnl_param_dump(s, &tnl_params->params);
		seq_printf(s, "\tNPU Entry: %02u CLS Entry: %02u CDRT: %02u Decap Enable: %u Encap Enable: %u MTU: %4u\n",
				tnl_params->npu_entry_proto,
				tnl_params->cls_entry,
				tnl_params->cdrt_idx,
				!!(tnl_params->flag & TNL_DECAP_ENABLE),
				!!(tnl_params->flag & TNL_ENCAP_ENABLE),
				tnl_params->params.mtu);
	}

	return 0;
}

static int tnl_open(struct inode *inode, struct file *file)
{
	return single_open(file, tnl_show, file->private_data);
}

#if KERNEL_VERSION(5, 6, 0) > LINUX_VERSION_CODE
static const struct file_operations tnl_proc_ops = {
	.owner = THIS_MODULE,
	.open = tnl_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#else /* KERNEL_VERSION(5, 6, 0) <= LINUX_VERSION_CODE */
static const struct proc_ops tnl_proc_ops = {
	.proc_open = tnl_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#endif /* KERNEL_VERSION(5, 6, 0) > LINUX_VERSION_CODE */

int mtk_npu_procfs_init(struct platform_device *pdev)
{
	npu_proc_root = proc_mkdir("npu", NULL);
	if (!npu_proc_root)
		return -ENOMEM;

	if (!proc_create("tnl", S_IFREG | 0444, npu_proc_root, &tnl_proc_ops))
		goto err_out;

	return 0;

err_out:
	remove_proc_entry("npu", NULL);

	return -ENOMEM;
}

void mtk_npu_procfs_deinit(struct platform_device *pdev)
{
	remove_proc_entry("tnl", npu_proc_root);
	remove_proc_entry("npu", NULL);
}
