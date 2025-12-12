// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2025 MediaTek Inc. All rights reserved.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/arm-smccc.h>
#include <linux/debugfs.h>

#define MTK_SIP_GET_AR_VER		0xC2000590
#define MTK_SIP_UPDATE_AR_VER		0xC2000591
#define MTK_SIP_LOCK_AR_VER		0xC2000592

enum AR_VER_ID {
	BL_AR_VER_ID = 0,
	FW_AR_VER_ID,
};

struct ar_ver_data {
	u32	id;
};

static struct dentry *sec_upg_debugfs_root;

static int sip_get_ar_ver(u32 id, u32 *ar_ver)
{
	struct arm_smccc_res res = { 0 };

	arm_smccc_smc(MTK_SIP_GET_AR_VER, id, 0, 0, 0, 0, 0, 0, &res);

	if (res.a0)
		return res.a0;

	*ar_ver = res.a1;

	return 0;
}

static int sip_update_ar_ver(u32 id, u32 ar_ver)
{
	struct arm_smccc_res res = { 0 };

	arm_smccc_smc(MTK_SIP_UPDATE_AR_VER, id, ar_ver, 0, 0, 0, 0, 0, &res);

	return res.a0;
}

static int sip_lock_ar_ver(void)
{
	struct arm_smccc_res res = { 0 };

	arm_smccc_smc(MTK_SIP_LOCK_AR_VER, 0, 0, 0, 0, 0, 0, 0, &res);

	return res.a0;
}

static int sec_upg_ar_ver_show(struct seq_file *s, void *v)
{
	struct ar_ver_data *data = (struct ar_ver_data *)s->private;
	int ret;
	u32 ver;

	ret = sip_get_ar_ver(data->id, &ver);
	if (ret)
		return ret;

	seq_printf(s, "%u\n", ver);

	return 0;
}

static int sec_upg_ar_ver_open(struct inode *inode, struct file *file)
{
	return single_open(file, sec_upg_ar_ver_show, inode->i_private);
}

static ssize_t sec_upg_ar_ver_write(struct file *file, const char __user *ubuf,
				    size_t count, loff_t *ppos)
{
	struct seq_file *s = (struct seq_file *)file->private_data;
	struct ar_ver_data *data = (struct ar_ver_data *)s->private;
	char buf[4];
	int ret;
	u32 ver;

	if (count >= sizeof(buf))
		return -EFAULT;

	if (copy_from_user(buf, ubuf, count))
		return -EFAULT;

	buf[count] = '\0';
	if (kstrtouint(buf, 0, &ver))
		return -EINVAL;

	ret = sip_update_ar_ver(data->id, ver);
	if (ret)
		return ret;

	return count;
}

static int sec_upg_ar_ver_lock_show(struct seq_file *s, void *v)
{
	return 0;
}

static int sec_upg_ar_ver_lock_open(struct inode *inode, struct file *file)
{
	return single_open(file, sec_upg_ar_ver_lock_show, inode->i_private);
}

static ssize_t sec_upg_ar_ver_lock_write(struct file *file, const char __user *ubuf,
					 size_t count, loff_t *ppos)
{
	int ret;

	ret = sip_lock_ar_ver();
	if (ret)
		return ret;

	return count;
}

static const struct file_operations sec_upg_ar_ver_fops = {
	.open		= sec_upg_ar_ver_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.write		= sec_upg_ar_ver_write,
	.release	= single_release
};

static const struct file_operations sec_upg_ar_ver_lock_fops = {
	.open		= sec_upg_ar_ver_lock_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.write		= sec_upg_ar_ver_lock_write,
	.release	= single_release
};

static const struct ar_ver_data bl_ar_ver_data = {
	.id		= BL_AR_VER_ID,
};

static const struct ar_ver_data fw_ar_ver_data = {
	.id		= FW_AR_VER_ID,
};

static int __init sec_upg_init(void)
{
	int ret;
	u32 ver;

	sec_upg_debugfs_root = debugfs_create_dir("sec_upg", NULL);
	if (!sec_upg_debugfs_root)
		return -ENOMEM;

	ret = sip_get_ar_ver(bl_ar_ver_data.id, &ver);
	if (ret)
		return ret;

	debugfs_create_file("bl_ar_ver", 0600, sec_upg_debugfs_root,
			    (void *)&bl_ar_ver_data, &sec_upg_ar_ver_fops);

	ret = sip_get_ar_ver(fw_ar_ver_data.id, &ver);
	if (!ret) {
		debugfs_create_file("fw_ar_ver", 0600, sec_upg_debugfs_root,
				    (void *)&fw_ar_ver_data, &sec_upg_ar_ver_fops);
	} else if (ret != -ENODEV) {
		return ret;
	}

	debugfs_create_file("ar_ver_lock", 0200, sec_upg_debugfs_root,
			    NULL, &sec_upg_ar_ver_lock_fops);

	return 0;
}

static void __exit sec_upg_exit(void)
{
	debugfs_remove_recursive(sec_upg_debugfs_root);
}

module_init(sec_upg_init);
module_exit(sec_upg_exit);

MODULE_LICENSE("GPL");
