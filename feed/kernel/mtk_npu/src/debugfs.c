// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 Mediatek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */
#include <linux/debugfs.h>
#include <linux/inet.h>
#include <linux/uaccess.h>

#include "npu/debugfs.h"
#include "npu/internal.h"
#include "npu/tunnel.h"

static int debugfs_tnl_info_cls_setup(struct npu_tnl_info *tnl_info);
static void debugfs_tnl_info_cls_destroy(struct npu_tnl_info *tnl_info);

static struct npu_tnl_info_ops debugfs_tnl_info_ops = {
	.cls_setup = debugfs_tnl_info_cls_setup,
	.cls_destroy = debugfs_tnl_info_cls_destroy,
};

struct dentry *npu_debugfs_root;

static int debugfs_tnl_info_cls_setup(struct npu_tnl_info *tnl_info)
{
	struct npu_tnl_params *tnl_params = &tnl_info->tnl_params;
	struct npu_tnl_type *tnl_type = tnl_info->tnl_type;
	struct npu_cls_entry *tcls;
	int ret;

	if (tnl_info->tcls)
		goto cls_entry_write;

	tcls = mtk_npu_tnl_info_cls_entry_alloc(tnl_info, tnl_params);
	if (IS_ERR(tcls))
		return PTR_ERR(tcls);

	ret = tnl_type->cls_entry_setup(tnl_info, &tcls->cls->cdesc);
	if (ret) {
		NPU_NOTICE("npu cls entry setup failed: %d\n", ret);
		goto cls_entry_unprepare;
	}

cls_entry_write:
	ret = mtk_npu_tnl_info_cls_entry_write(tnl_info);

cls_entry_unprepare:
	if (ret)
		mtk_npu_tnl_info_cls_entry_free(tnl_info, tnl_params);

	return ret;
}

static void debugfs_tnl_info_cls_destroy(struct npu_tnl_info *tnl_info)
{
	struct npu_cls_entry *tcls = tnl_info->tcls;

	memset(&tcls->cls->cdesc, 0, sizeof(tcls->cls->cdesc));
}

static int npu_tnl_add_new_tnl(const char *buf)
{
	struct npu_tnl_params tnl_params;
	struct npu_params *params;
	struct npu_tnl_info *tnl_info;
	struct npu_tnl_type *tnl_type;
	char proto[DEBUG_PROTO_LEN];
	int nchar = 0;
	int ofs = 0;
	int ret = 0;
	u16 mtu = 0;

	memset(&tnl_params, 0, sizeof(struct npu_tnl_params));
	memset(proto, 0, sizeof(proto));

	params = &tnl_params.params;

	ret = sscanf(buf + ofs, "%hu %n", &mtu, &nchar);
	if (ret != 1)
		return -EPERM;
	ofs += nchar;

	ret = mtk_npu_debug_param_setup(buf, &ofs, params);
	if (ret)
		return ret;

	ret = mtk_npu_debug_param_proto_peek(buf, ofs, proto);
	if (ret < 0)
		return ret;
	ofs += ret;

	tnl_type = mtk_npu_tnl_type_get_by_name(proto);
	if (!tnl_type || !tnl_type->tnl_debug_param_setup)
		return -ENODEV;

	ret = tnl_type->tnl_debug_param_setup(buf, &ofs, params);
	if (ret < 0)
		return ret;

	tnl_params.params.mtu = mtu;
	tnl_params.flag |= TNL_DECAP_ENABLE;
	tnl_params.flag |= TNL_ENCAP_ENABLE;
	tnl_params.npu_entry_proto = tnl_type->tnl_proto_type;

	tnl_info = mtk_npu_tnl_info_alloc(tnl_type);
	if (IS_ERR(tnl_info))
		return -ENOMEM;

	tnl_info->ops = &debugfs_tnl_info_ops;

	tnl_info->flag |= TNL_INFO_DEBUG;
	memcpy(&tnl_info->cache, &tnl_params, sizeof(struct npu_tnl_params));

	mtk_npu_tnl_info_hash(tnl_info);
	mtk_npu_tnl_info_submit(tnl_info);

	return 0;
}

static ssize_t npu_tnl_write(struct file *file, const char __user *buffer,
			     size_t count, loff_t *data)
{
	char cmd[21] = {0};
	char buf[512];
	int nchar = 0;
	int ret = 0;

	if (count > sizeof(buf))
		return -ENOMEM;

	if (copy_from_user(buf, buffer, count))
		return -EFAULT;

	buf[count] = '\0';
	ret = sscanf(buf, "%20s %n", cmd, &nchar);
	if (ret != 1)
		return -EPERM;

	if (!strcmp(cmd, "NEW_TNL")) {
		ret = npu_tnl_add_new_tnl(buf + nchar);
		if (ret)
			return ret;
	} else {
		return -EINVAL;
	}

	return count;
}

static const struct file_operations npu_tnl_fops = {
	.open = simple_open,
	.write = npu_tnl_write,
};

int mtk_npu_debugfs_init(struct platform_device *pdev)
{
	npu_debugfs_root = debugfs_create_dir("npu", NULL);

	debugfs_create_file("tunnel", 0444, npu_debugfs_root, NULL, &npu_tnl_fops);

	return 0;
}

void mtk_npu_debugfs_deinit(struct platform_device *pdev)
{
	debugfs_remove_recursive(npu_debugfs_root);
}
