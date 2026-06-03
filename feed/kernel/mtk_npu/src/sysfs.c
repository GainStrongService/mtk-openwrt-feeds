// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/device.h>

#include "npu/firmware.h"
#include "npu/internal.h"
#include "npu/mcu.h"
#include "npu/misc.h"
#include "npu/sysfs.h"
#include "npu/trm.h"
#include "npu/tunnel.h"
#include "npu/wdt.h"

struct kset *npu_kset;
EXPORT_SYMBOL(npu_kset);

static const char *npu_role_name[__NPU_ROLE_TYPE_MAX] = {
	[NPU_ROLE_TYPE_MGMT] = "npu-mgmt",
	[NPU_ROLE_TYPE_CLUSTER] = "npu-offload",
};

static ssize_t fw_log_level_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	enum npu_fw_log_level level;
	enum core_id core;
	size_t len = 0;

	level = mtk_npu_misc_get_fw_log_level(CORE_MGMT);
	len += scnprintf(buf + len, PAGE_SIZE - len,
			 "Core MGMT firmware log level: %u\n", level);

	for (core = CORE_OFFLOAD_0; core < CORE_OFFLOAD_NUM; core++) {
		level = mtk_npu_misc_get_fw_log_level(core);
		len += scnprintf(buf + len, PAGE_SIZE - len,
				 "Core Offload%u firmware log level: %u\n",
				 core, level);
	}

	return len;
}

static ssize_t fw_log_level_store(struct device *dev, struct device_attribute *attr,
				  const char *buf, size_t count)
{
	u32 val;

	if (kstrtou32(buf, 10, &val))
		return count;

	mtk_npu_misc_set_fw_log_level(val);

	return count;
}

static ssize_t fw_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	enum npu_role_type type;
	struct tm tm = {0};
	const char *value;
	const char *prop;
	size_t len = 0;
	u32 nattr;
	u32 i;

	for (type = NPU_ROLE_TYPE_MGMT; type < __NPU_ROLE_TYPE_MAX; type++) {
		mtk_npu_fw_get_built_date(type, &tm);

		len += scnprintf(buf + len, PAGE_SIZE - len,
				 "%s FW information:\n",
				 npu_role_name[type]);
		len += scnprintf(buf + len, PAGE_SIZE - len,
				 "Git revision:\t%llx\n",
				 mtk_npu_fw_get_git_commit_id(type));
		len += scnprintf(buf + len, PAGE_SIZE - len,
				 "Build data:\t%04ld/%02d/%02d %02d:%02d:%02d\n",
				 tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
				 tm.tm_hour, tm.tm_min, tm.tm_sec);

		nattr = mtk_npu_fw_attr_get_num(type);
		for (i = 0; i < nattr; i++) {
			prop = mtk_npu_fw_attr_get_property(type, i);
			if (!prop)
				continue;
			value = mtk_npu_fw_attr_get_value(type, prop);
			len += scnprintf(buf + len, PAGE_SIZE - len,
					 "%s:\t%s\n",
					 prop, value);
		}

		len += scnprintf(buf + len, PAGE_SIZE - len, "\n");
	}

	return len;
}

static int trm_fetch_config(const char *buf, int *ofs, char *name,
			    u32 *offset, u32 *size, u8 *enable)
{
	int nchar = 0;
	int ret = 0;

	ret = sscanf(buf + *ofs, "%31s %x %x %hhx %n",
		name, offset, size, enable, &nchar);
	if (ret != 4)
		return -EPERM;

	*ofs += nchar;

	return nchar;
}

static ssize_t trm_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	char name[TRM_CONFIG_NAME_MAX_LEN] = { 0 };
	char cmd[21] = { 0 };
	int nchar = 0;
	int ret = 0;
	u32 offset;
	u8 enable;
	u32 size;

	ret = sscanf(buf, "%20s %n", cmd, &nchar);
	if (ret != 1)
		return -EPERM;

	if (!strcmp(cmd, "trm_dump")) {
		ret = mtk_trm_dump(TRM_RSN_NULL);
		if (ret)
			return ret;
	} else if (!strcmp(cmd, "trm_cfg_setup")) {
		ret = trm_fetch_config(buf, &nchar, name, &offset, &size, &enable);
		if (ret < 0)
			return ret;

		ret = mtk_trm_cfg_setup(name, offset, size, enable);
		if (ret)
			return ret;
	}

	return count;
}

static ssize_t wdt_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	char cmd[21] = {0};
	u32 core = 0;
	u32 i;
	int ret;

	ret = sscanf(buf, "%20s %x", cmd, &core);
	if (ret != 2)
		return -EPERM;

	core &= CORE_NPU_MASK;
	if (!strcmp(cmd, "WDT_TO")) {
		for (i = 0; i < CORE_NPU_NUM; i++) {
			if (core & 0x1)
				mtk_npu_wdt_trigger_timeout(i);
			core >>= 1;
		}
	} else {
		return -EINVAL;
	}

	return count;
}

static DEVICE_ATTR_RW(fw_log_level);
static DEVICE_ATTR_RO(fw_info);
static DEVICE_ATTR_WO(trm);
static DEVICE_ATTR_WO(wdt);

static struct attribute *mcu_attributes[] = {
	&dev_attr_fw_log_level.attr,
	&dev_attr_fw_info.attr,
	&dev_attr_trm.attr,
	&dev_attr_wdt.attr,
	NULL,
};

static const struct attribute_group mcu_attr_group = {
	.name = "mcu",
	.attrs = mcu_attributes,
};

int mtk_npu_sysfs_init(struct platform_device *pdev)
{
	int ret = 0;

	npu_kset = kset_create_and_add("npu", NULL, &pdev->dev.kobj);
	if (!npu_kset) {
		NPU_NOTICE("create npu/ kset failed\n");
		return -ENOMEM;
	}

	ret = sysfs_create_group(&npu_kset->kobj, &mcu_attr_group);
	if (ret) {
		NPU_NOTICE("create npu/mcu/ sysfs failed\n");
		goto err_remove_npu;
	}

	ret = sysfs_create_link(&pdev->dev.kobj.kset->kobj, &npu_kset->kobj, "npu");
	if (ret) {
		NPU_NOTICE("create npu/ sysfs link failed: %d\n", ret);
		goto err_remove_mcu;
	}

	return ret;

err_remove_mcu:
	sysfs_remove_group(&npu_kset->kobj, &mcu_attr_group);

err_remove_npu:
	kset_unregister(npu_kset);

	return 0;
}

void mtk_npu_sysfs_deinit(struct platform_device *pdev)
{
	sysfs_remove_link(&pdev->dev.kobj.kset->kobj, "npu");
	sysfs_remove_group(&npu_kset->kobj, &mcu_attr_group);

	kset_unregister(npu_kset);
}
