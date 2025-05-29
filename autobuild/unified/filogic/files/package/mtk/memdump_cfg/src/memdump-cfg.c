// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2024 MediaTek Inc. All rights reserved.
 *
 * Helper for configuring memdump
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <asm/byteorder.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/arm-smccc.h>
#include <linux/uaccess.h>

#define MDC "memdump"
#define MDC_SIP "local_ip"
#define MDC_DIP "dest_ip"

#define MTK_SIP_EMERG_MEM_DUMP_CFG		0xC2000541

static struct proc_dir_entry *mdc_entry;
static struct proc_dir_entry *mdc_sip_entry;
static struct proc_dir_entry *mdc_dip_entry;

static __be32 string_to_ip(const char *s)
{
	__be32 addr;
	char *e;
	int i;

	addr = 0;
	if (!s)
		return addr;

	for (addr = 0, i = 0; i < 4; ++i) {
		ulong val = s ? simple_strtoul(s, &e, 10) : 0;
		if (val > 255) {
			addr = 0;
			return addr;
		}

		if (i != 3 && *e != '.') {
			addr = 0;
			return addr;
		}

		addr <<= 8;
		addr |= (val & 0xFF);

		if (s)
			s = (*e) ? e + 1 : e;
	}

	addr = htonl(addr);
	return addr;
}

static int user_input_to_ip(const char __user *buffer, size_t count,
			    loff_t *pos, __be32 *addr)
{
	char ipstr[64];
	size_t n;

	if (*pos)
		return -ENOTSUPP;

	if (count > sizeof(ipstr) - 1)
		return -E2BIG;

	n = copy_from_user(ipstr, buffer, count);
	ipstr[n] = 0;

	*addr = string_to_ip(ipstr);

	return 0;
}

static int mdc_sip_display(struct seq_file *seq, void *v)
{
	struct arm_smccc_res res = {0};
	uint8_t addr[4];

	arm_smccc_smc(MTK_SIP_EMERG_MEM_DUMP_CFG, 0, 0, 0, 0, 0, 0, 0, &res);

	if (res.a0)
		return -EINVAL;

	memcpy(addr, &res.a1, 4);

	seq_printf(seq, "%u.%u.%u.%u\n", addr[0], addr[1], addr[2], addr[3]);

	return 0;
}

static int mdc_sip_open(struct inode *inode, struct file *file)
{
	return single_open(file, mdc_sip_display, inode->i_private);
}

static ssize_t mdc_sip_write(struct file *file, const char __user *buffer,
			     size_t count, loff_t *pos)
{
	struct arm_smccc_res res = {0};
	__be32 addr;
	int ret;

	ret = user_input_to_ip(buffer, count, pos, &addr);
	if (ret)
		return ret;

	arm_smccc_smc(MTK_SIP_EMERG_MEM_DUMP_CFG, 1, 0, addr, 0, 0, 0, 0, &res);

	if (res.a0)
		return -EINVAL;

	return count;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
static const struct proc_ops mdc_sip_fops = {
	.proc_open  = mdc_sip_open,
	.proc_read = seq_read,
	.proc_write = mdc_sip_write,
	.proc_lseek  = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations mdc_sip_fops = {
	.open  = mdc_sip_open,
	.read = seq_read,
	.write = mdc_sip_write,
	.llseek  = seq_lseek,
	.release = single_release,
};
#endif

static int mdc_dip_display(struct seq_file *seq, void *v)
{
	struct arm_smccc_res res = {0};
	uint8_t addr[4];

	arm_smccc_smc(MTK_SIP_EMERG_MEM_DUMP_CFG, 0, 1, 0, 0, 0, 0, 0, &res);

	if (res.a0)
		return -EINVAL;

	memcpy(addr, &res.a1, 4);

	seq_printf(seq, "%u.%u.%u.%u\n", addr[0], addr[1], addr[2], addr[3]);

	return 0;
}

static int mdc_dip_open(struct inode *inode, struct file *file)
{
	return single_open(file, mdc_dip_display, inode->i_private);
}

static ssize_t mdc_dip_write(struct file *file, const char __user *buffer,
			     size_t count, loff_t *pos)
{
	struct arm_smccc_res res = {0};
	__be32 addr;
	int ret;

	ret = user_input_to_ip(buffer, count, pos, &addr);
	if (ret)
		return ret;

	arm_smccc_smc(MTK_SIP_EMERG_MEM_DUMP_CFG, 1, 1, addr, 0, 0, 0, 0, &res);

	if (res.a0)
		return -EINVAL;

	return count;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
static const struct proc_ops mdc_dip_fops = {
	.proc_open  = mdc_dip_open,
	.proc_read = seq_read,
	.proc_write = mdc_dip_write,
	.proc_lseek  = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations mdc_dip_fops = {
	.open  = mdc_dip_open,
	.read = seq_read,
	.write = mdc_dip_write,
	.llseek  = seq_lseek,
	.release = single_release,
};
#endif

static int __init mdc_init(void)
{
	mdc_entry = proc_mkdir(MDC, NULL);
	if (!mdc_entry) {
		pr_err("failed to create proc entry " MDC);
		return -1;
	}

	mdc_sip_entry = proc_create(MDC_SIP, 0644, mdc_entry, &mdc_sip_fops);
	if (!mdc_sip_entry) {
		pr_err("failed to create proc entry " MDC "/" MDC_SIP);
		proc_remove(mdc_entry);
		return -1;
	}

	mdc_dip_entry = proc_create(MDC_DIP, 0644, mdc_entry, &mdc_dip_fops);
	if (!mdc_dip_entry) {
		pr_err("failed to create proc entry " MDC "/" MDC_DIP);
		proc_remove(mdc_entry);
		return -1;
	}

	return 0;
}

static void __exit mdc_exit(void)
{
	proc_remove(mdc_entry);
}

module_init(mdc_init);
module_exit(mdc_exit);

MODULE_AUTHOR("Weijie Gao <weijie.gao@mediatek.com>");
MODULE_DESCRIPTION("Helper for configuring memdump");
MODULE_LICENSE("GPL");
