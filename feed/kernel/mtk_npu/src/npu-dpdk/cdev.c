// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2026 Mediatek Inc. All Rights Reserved.
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/version.h>

#include "npu-dpdk/cdev.h"
#include "npu-dpdk/internal.h"
#include "npu-dpdk/ioctl.h"
#include "npu-dpdk/mac-filter.h"

static dev_t npu_dpdk_dev_id;
static struct cdev npu_dpdk_cdev;
static struct class *npu_dpdk_class;

static int npu_dpdk_show(struct seq_file *s, void *private)
{
	struct npu_dpdk_mac_info minfo;
	u32 i = 0;
	int ret;

	while (1) {
		ret = mtk_npu_dpdk_mac_filter_info_get_by_ofs(&minfo, i);
		if (ret < 0)
			break;

		seq_printf(s, "MAC filter entry%04d addr: %pM\n", minfo.idx, minfo.mac.addr);

		/* move to next index */
		i = ret + 1;
	}

	return 0;
}

static int npu_dpdk_open(struct inode *inode, struct file *f)
{
	return single_open(f, npu_dpdk_show, f->private_data);
}

static long npu_dpdk_ioctl_insert(struct npu_dpdk_cmd *ncmd)
{
	switch (ncmd->type) {
	case NPU_DPDK_CMD_MAC_FILTER:
		return mtk_npu_dpdk_mac_filter_entry_insert(&ncmd->mf.minfo.mac);
	default:
		return -EINVAL;
	}
}

static long npu_dpdk_ioctl_delete(struct npu_dpdk_cmd *ncmd)
{
	switch (ncmd->type) {
	case NPU_DPDK_CMD_MAC_FILTER:
		return mtk_npu_dpdk_mac_filter_entry_delete(&ncmd->mf.minfo.mac);
	default:
		return -EINVAL;
	}
}

static long npu_dpdk_ioctl_config(struct npu_dpdk_cmd *ncmd)
{
	switch (ncmd->type) {
	case NPU_DPDK_CMD_MAC_FILTER:
		return mtk_npu_dpdk_mac_filter_config(&ncmd->mf.cfg);
	default:
		return -EINVAL;
	}
}

static long npu_dpdk_ioctl_clear(struct npu_dpdk_cmd *ncmd)
{
	switch (ncmd->type) {
	case NPU_DPDK_CMD_MAC_FILTER:
		return mtk_npu_dpdk_mac_filter_entry_clear(&ncmd->mf.clr_cmd);
	default:
		return -EINVAL;
	}
}

static long npu_dpdk_ioctl_query(struct npu_dpdk_cmd *ncmd, void __user *p)
{
	int ret;

	switch (ncmd->type) {
	case NPU_DPDK_CMD_MAC_FILTER:
		ret = mtk_npu_dpdk_mac_filter_entry_query(&ncmd->mf.query);
		if (ret)
			return ret;
		if (copy_to_user(p, ncmd, sizeof(struct npu_dpdk_cmd)))
			return -EFAULT;
		return 0;
	default:
		return -EINVAL;
	}
}

static long npu_dpdk_unlocked_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	struct npu_dpdk_cmd ncmd;

	if (copy_from_user(&ncmd, (void __user *)arg, sizeof(struct npu_dpdk_cmd)))
		return -EFAULT;

	switch (cmd) {
	case NPU_DPDK_IOCTL_INSERT:
		return npu_dpdk_ioctl_insert(&ncmd);
	case NPU_DPDK_IOCTL_DELETE:
		return npu_dpdk_ioctl_delete(&ncmd);
	case NPU_DPDK_IOCTL_CONFIG:
		return npu_dpdk_ioctl_config(&ncmd);
	case NPU_DPDK_IOCTL_CLEAR:
		return npu_dpdk_ioctl_clear(&ncmd);
	case NPU_DPDK_IOCTL_QUERY:
		return npu_dpdk_ioctl_query(&ncmd, argp);
	default:
		return -EINVAL;
	}
}

static const struct file_operations npu_dpdk_fops = {
	.owner = THIS_MODULE,
	.open = npu_dpdk_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.unlocked_ioctl = npu_dpdk_unlocked_ioctl,
	.release = single_release,
};

int mtk_npu_dpdk_cdev_init(void)
{
	struct device *dev;
	int ret;

	ret = alloc_chrdev_region(&npu_dpdk_dev_id, 0, 1, "NPU-DPDK");
	if (ret) {
		NPU_ERR("allocate character device number failed: %d\n", ret);
		return ret;
	}

	cdev_init(&npu_dpdk_cdev, &npu_dpdk_fops);
	ret = cdev_add(&npu_dpdk_cdev, npu_dpdk_dev_id, 1);
	if (ret) {
		NPU_ERR("register character device failed: %d\n", ret);
		goto release_major_number;
	}

#if KERNEL_VERSION(5, 6, 0) > LINUX_VERSION_CODE
	npu_dpdk_class = class_create(THIS_MODULE, "npu-dpdk");
#else /* KERNEL_VERSION(5, 6, 0) <= LINUX_VERSION_CODE */
	npu_dpdk_class = class_create("npu-dpdk");
#endif /* KERNEL_VERSION(5, 6, 0) > LINUX_VERSION_CODE */
	if (IS_ERR(npu_dpdk_class)) {
		ret = PTR_ERR(npu_dpdk_class);
		NPU_ERR("create class failed: %d\n", ret);
		goto release_cdev;
	}

	dev = device_create(npu_dpdk_class, NULL, npu_dpdk_dev_id, NULL, "npu-dpdk");
	if (IS_ERR(dev)) {
		ret = PTR_ERR(dev);
		NPU_ERR("character device create failed: %d\n", ret);
		goto release_class;
	}

	return 0;

release_class:
	class_destroy(npu_dpdk_class);

release_cdev:
	cdev_del(&npu_dpdk_cdev);

release_major_number:
	unregister_chrdev_region(npu_dpdk_dev_id, 1);

	return ret;
}

int mtk_npu_dpdk_cdev_deinit(void)
{
	device_destroy(npu_dpdk_class, npu_dpdk_dev_id);
	class_destroy(npu_dpdk_class);
	cdev_del(&npu_dpdk_cdev);
	unregister_chrdev_region(npu_dpdk_dev_id, 1);

	return 0;
}
