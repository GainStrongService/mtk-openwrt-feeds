/*
 *   Copyright (C) 2018 MediaTek Inc.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; version 2 of the License
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   Copyright (C) 2009-2016 John Crispin <blogic@openwrt.org>
 *   Copyright (C) 2009-2016 Felix Fietkau <nbd@openwrt.org>
 *   Copyright (C) 2013-2016 Michael Lee <igvtee@gmail.com>
 */

#include <linux/trace_seq.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/u64_stats_sync.h>
#include <linux/dma-mapping.h>
#include <linux/netdevice.h>
#include <linux/ctype.h>
#include <linux/debugfs.h>
#include <linux/of_mdio.h>

#include "mtk_eth_soc.h"
#include "mtk_eth_dbg.h"

struct mtk_eth_debug {
        struct dentry *root;
};

struct mtk_eth *g_eth;

struct mtk_eth_debug eth_debug;

void mt7530_mdio_w32(struct mtk_eth *eth, u16 reg, u32 val)
{
	mutex_lock(&eth->mii_bus->mdio_lock);

	_mtk_mdio_write(eth, 0x1f, 0x1f, (reg >> 6) & 0x3ff);
	_mtk_mdio_write(eth, 0x1f, (reg >> 2) & 0xf,  val & 0xffff);
	_mtk_mdio_write(eth, 0x1f, 0x10, val >> 16);

	mutex_unlock(&eth->mii_bus->mdio_lock);
}

u32 mt7530_mdio_r32(struct mtk_eth *eth, u32 reg)
{
	u16 high, low;

	mutex_lock(&eth->mii_bus->mdio_lock);

	_mtk_mdio_write(eth, 0x1f, 0x1f, (reg >> 6) & 0x3ff);
	low = _mtk_mdio_read(eth, 0x1f, (reg >> 2) & 0xf);
	high = _mtk_mdio_read(eth, 0x1f, 0x10);

	mutex_unlock(&eth->mii_bus->mdio_lock);

	return (high << 16) | (low & 0xffff);
}

void mtk_switch_w32(struct mtk_eth *eth, u32 val, unsigned reg)
{
	mtk_w32(eth, val, reg + 0x10000);
}
EXPORT_SYMBOL(mtk_switch_w32);

u32 mtk_switch_r32(struct mtk_eth *eth, unsigned reg)
{
	return mtk_r32(eth, reg + 0x10000);
}
EXPORT_SYMBOL(mtk_switch_r32);

static int mtketh_debug_show(struct seq_file *m, void *private)
{
	struct mtk_eth *eth = m->private;
	struct mtk_mac *mac = 0;
	u32 d;
	int  i, j = 0;

	for (i = 0 ; i < MTK_MAX_DEVS ; i++) {
		if (!eth->mac[i] ||
		    of_phy_is_fixed_link(eth->mac[i]->of_node))
			continue;
		mac = eth->mac[i];
#if 0 //FIXME
		while (j < 30) {
			d =  _mtk_mdio_read(eth, mac->phy_dev->addr, j);

			seq_printf(m, "phy=%d, reg=0x%08x, data=0x%08x\n",
				   mac->phy_dev->addr, j, d);
			j++;
		}
#endif		
	}
	return 0;
}

static int mtketh_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtketh_debug_show, inode->i_private);
}

static const struct file_operations mtketh_debug_fops = {
	.open = mtketh_debug_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int mtketh_mt7530sw_debug_show(struct seq_file *m, void *private)
{
	struct mtk_eth *eth = m->private;
	u32  offset, data;
	int i;
	struct mt7530_ranges {
		u32 start;
		u32 end;
	} ranges[] = {
		{0x0, 0xac},
		{0x1000, 0x10e0},
		{0x1100, 0x1140},
		{0x1200, 0x1240},
		{0x1300, 0x1340},
		{0x1400, 0x1440},
		{0x1500, 0x1540},
		{0x1600, 0x1640},
		{0x1800, 0x1848},
		{0x1900, 0x1948},
		{0x1a00, 0x1a48},
		{0x1b00, 0x1b48},
		{0x1c00, 0x1c48},
		{0x1d00, 0x1d48},
		{0x1e00, 0x1e48},
		{0x1f60, 0x1ffc},
		{0x2000, 0x212c},
		{0x2200, 0x222c},
		{0x2300, 0x232c},
		{0x2400, 0x242c},
		{0x2500, 0x252c},
		{0x2600, 0x262c},
		{0x3000, 0x3014},
		{0x30c0, 0x30f8},
		{0x3100, 0x3114},
		{0x3200, 0x3214},
		{0x3300, 0x3314},
		{0x3400, 0x3414},
		{0x3500, 0x3514},
		{0x3600, 0x3614},
		{0x4000, 0x40d4},
		{0x4100, 0x41d4},
		{0x4200, 0x42d4},
		{0x4300, 0x43d4},
		{0x4400, 0x44d4},
		{0x4500, 0x45d4},
		{0x4600, 0x46d4},
		{0x4f00, 0x461c},
		{0x7000, 0x7038},
		{0x7120, 0x7124},
		{0x7800, 0x7804},
		{0x7810, 0x7810},
		{0x7830, 0x7830},
		{0x7a00, 0x7a7c},
		{0x7b00, 0x7b04},
		{0x7e00, 0x7e04},
		{0x7ffc, 0x7ffc},
	};

	if (!mt7530_exist(eth))
		return -EOPNOTSUPP;

	if ((!eth->mac[0] || !of_phy_is_fixed_link(eth->mac[0]->of_node)) &&
	    (!eth->mac[1] || !of_phy_is_fixed_link(eth->mac[1]->of_node))) {
		seq_puts(m, "no switch found\n");
		return 0;
	}

	for (i = 0 ; i < ARRAY_SIZE(ranges) ; i++) {
		for (offset = ranges[i].start;
		     offset <= ranges[i].end; offset += 4) {
			data =  mt7530_mdio_r32(eth, offset);
			seq_printf(m, "mt7530 switch reg=0x%08x, data=0x%08x\n",
				   offset, data);
		}
	}

	return 0;
}

static int mtketh_debug_mt7530sw_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtketh_mt7530sw_debug_show, inode->i_private);
}

static const struct file_operations mtketh_debug_mt7530sw_fops = {
	.open = mtketh_debug_mt7530sw_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static ssize_t mtketh_mt7530sw_debugfs_write(struct file *file,
					     const char __user *ptr,
					     size_t len, loff_t *off)
{
	struct mtk_eth *eth = file->private_data;
	char buf[32], *token, *p = buf;
	u32 reg, value, phy;
	int ret;

	if (!mt7530_exist(eth))
		return -EOPNOTSUPP;

	if (*off != 0)
		return 0;

	if (len > sizeof(buf) - 1)
		len = sizeof(buf) - 1;

	ret = strncpy_from_user(buf, ptr, len);
	if (ret < 0)
		return ret;
	buf[len] = '\0';

	token = strsep(&p, " ");
	if (!token)
		return -EINVAL;
	if (kstrtoul(token, 16, (unsigned long *)&phy))
		return -EINVAL;

	token = strsep(&p, " ");
	if (!token)
		return -EINVAL;
	if (kstrtoul(token, 16, (unsigned long *)&reg))
		return -EINVAL;

	token = strsep(&p, " ");
	if (!token)
		return -EINVAL;
	if (kstrtoul(token, 16, (unsigned long *)&value))
		return -EINVAL;

	pr_info("%s:phy=%d, reg=0x%x, val=0x%x\n", __func__,
		0x1f, reg, value);
	mt7530_mdio_w32(eth, reg, value);
	pr_info("%s:phy=%d, reg=0x%x, val=0x%x confirm..\n", __func__,
		0x1f, reg, mt7530_mdio_r32(eth, reg));

	return len;
}

static ssize_t mtketh_debugfs_write(struct file *file, const char __user *ptr,
				    size_t len, loff_t *off)
{
	struct mtk_eth *eth = file->private_data;
	char buf[32], *token, *p = buf;
	u32 reg, value, phy;
	int ret;

	if (*off != 0)
		return 0;

	if (len > sizeof(buf) - 1)
		len = sizeof(buf) - 1;

	ret = strncpy_from_user(buf, ptr, len);
	if (ret < 0)
		return ret;
	buf[len] = '\0';

	token = strsep(&p, " ");
	if (!token)
		return -EINVAL;
	if (kstrtoul(token, 16, (unsigned long *)&phy))
		return -EINVAL;

	token = strsep(&p, " ");

	if (!token)
		return -EINVAL;
	if (kstrtoul(token, 16, (unsigned long *)&reg))
		return -EINVAL;

	token = strsep(&p, " ");

	if (!token)
		return -EINVAL;
	if (kstrtoul(token, 16, (unsigned long *)&value))
		return -EINVAL;

	pr_info("%s:phy=%d, reg=0x%x, val=0x%x\n", __func__,
		phy, reg, value);

	_mtk_mdio_write(eth, phy,  reg, value);

	pr_info("%s:phy=%d, reg=0x%x, val=0x%x confirm..\n", __func__,
		phy, reg, _mtk_mdio_read(eth, phy, reg));

	return len;
}

static ssize_t mtketh_debugfs_reset(struct file *file, const char __user *ptr,
				    size_t len, loff_t *off)
{
	struct mtk_eth *eth = file->private_data;

	schedule_work(&eth->pending_work);
	return len;
}

static const struct file_operations fops_reg_w = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.write = mtketh_debugfs_write,
	.llseek = noop_llseek,
};

static const struct file_operations fops_eth_reset = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.write = mtketh_debugfs_reset,
	.llseek = noop_llseek,
};

static const struct file_operations fops_mt7530sw_reg_w = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.write = mtketh_mt7530sw_debugfs_write,
	.llseek = noop_llseek,
};

void mtketh_debugfs_exit(struct mtk_eth *eth)
{
	debugfs_remove_recursive(eth_debug.root);
}

int mtketh_debugfs_init(struct mtk_eth *eth)
{
	int ret = 0;

	eth_debug.root = debugfs_create_dir("mtketh", NULL);
	if (!eth_debug.root) {
		dev_notice(eth->dev, "%s:err at %d\n", __func__, __LINE__);
		ret = -ENOMEM;
	}

	debugfs_create_file("phy_regs", S_IRUGO,
			    eth_debug.root, eth, &mtketh_debug_fops);
	debugfs_create_file("phy_reg_w", S_IFREG | S_IWUSR,
			    eth_debug.root, eth,  &fops_reg_w);
	debugfs_create_file("reset", S_IFREG | S_IWUSR,
			    eth_debug.root, eth,  &fops_eth_reset);
	if (mt7530_exist(eth)) {
		debugfs_create_file("mt7530sw_regs", S_IRUGO,
				    eth_debug.root, eth,
				    &mtketh_debug_mt7530sw_fops);
		debugfs_create_file("mt7530sw_reg_w", S_IFREG | S_IWUSR,
				    eth_debug.root, eth,
				    &fops_mt7530sw_reg_w);
	}
	return ret;
}

void mii_mgr_read_combine(struct mtk_eth *eth, u32 phy_addr, u32 phy_register,
			  u32 *read_data)
{
	if (mt7530_exist(eth) && phy_addr == 31)
		*read_data = mt7530_mdio_r32(eth, phy_register);

	else
		*read_data = _mtk_mdio_read(eth, phy_addr, phy_register);
}

void mii_mgr_write_combine(struct mtk_eth *eth, u16 phy_addr, u16 phy_register,
			   u32 write_data)
{
	if (mt7530_exist(eth) && phy_addr == 31)
		mt7530_mdio_w32(eth, phy_register, write_data);

	else
		_mtk_mdio_write(eth, phy_addr, phy_register, write_data);
}

static void mii_mgr_read_cl45(struct mtk_eth *eth, u16 port, u16 devad, u16 reg, u16 *data)
{
	mtk_cl45_ind_read(eth, port, devad, reg, data);
}

static void mii_mgr_write_cl45(struct mtk_eth *eth, u16 port, u16 devad, u16 reg, u16 data)
{
	mtk_cl45_ind_write(eth, port, devad, reg, data);
}

int mtk_do_priv_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	struct mtk_mac *mac = netdev_priv(dev);
	struct mtk_eth *eth = mac->hw;
	struct mtk_mii_ioctl_data mii;
	struct mtk_esw_reg reg;

	switch (cmd) {
	case MTKETH_MII_READ:
		if (copy_from_user(&mii, ifr->ifr_data, sizeof(mii)))
			goto err_copy;
		mii_mgr_read_combine(eth, mii.phy_id, mii.reg_num,
				     &mii.val_out);
		if (copy_to_user(ifr->ifr_data, &mii, sizeof(mii)))
			goto err_copy;

		return 0;
	case MTKETH_MII_WRITE:
		if (copy_from_user(&mii, ifr->ifr_data, sizeof(mii)))
			goto err_copy;
		mii_mgr_write_combine(eth, mii.phy_id, mii.reg_num,
				      mii.val_in);
		return 0;
	case MTKETH_MII_READ_CL45:
		if (copy_from_user(&mii, ifr->ifr_data, sizeof(mii)))
			goto err_copy;
		mii_mgr_read_cl45(eth,
				  mdio_phy_id_prtad(mii.phy_id),
				  mdio_phy_id_devad(mii.phy_id),
				  mii.reg_num,
				  &mii.val_out);
		if (copy_to_user(ifr->ifr_data, &mii, sizeof(mii)))
			goto err_copy;

		return 0;
	case MTKETH_MII_WRITE_CL45:
		if (copy_from_user(&mii, ifr->ifr_data, sizeof(mii)))
			goto err_copy;
		mii_mgr_write_cl45(eth,
				  mdio_phy_id_prtad(mii.phy_id),
				  mdio_phy_id_devad(mii.phy_id),
				  mii.reg_num,
				  mii.val_in);
		return 0;
	case MTKETH_ESW_REG_READ:
		if (!mt7530_exist(eth))
			return -EOPNOTSUPP;
		if (copy_from_user(&reg, ifr->ifr_data, sizeof(reg)))
			goto err_copy;
		if (reg.off > REG_ESW_MAX)
			return -EINVAL;
		reg.val = mtk_switch_r32(eth, reg.off);

		if (copy_to_user(ifr->ifr_data, &reg, sizeof(reg)))
			goto err_copy;

		return 0;
	case MTKETH_ESW_REG_WRITE:
		if (!mt7530_exist(eth))
			return -EOPNOTSUPP;
		if (copy_from_user(&reg, ifr->ifr_data, sizeof(reg)))
			goto err_copy;
		if (reg.off > REG_ESW_MAX)
			return -EINVAL;
		mtk_switch_w32(eth, reg.val, reg.off);

		return 0;
	default:
		break;
	}

	return -EOPNOTSUPP;
err_copy:
	return -EFAULT;
}

int esw_cnt_read(struct seq_file *seq, void *v)
{
	unsigned int pkt_cnt = 0;
	int i = 0;
	struct mtk_eth *eth = g_eth;
	unsigned int mib_base = MTK_GDM1_TX_GBCNT;

	seq_puts(seq, "\n		  <<CPU>>\n");
	seq_puts(seq, "		    |\n");
	seq_puts(seq, "+-----------------------------------------------+\n");
	seq_puts(seq, "|		  <<PSE>>		        |\n");
	seq_puts(seq, "+-----------------------------------------------+\n");
	seq_puts(seq, "		   |\n");
	seq_puts(seq, "+-----------------------------------------------+\n");
	seq_puts(seq, "|		  <<GDMA>>		        |\n");
	seq_printf(seq, "| GDMA1_RX_GBCNT  : %010u (Rx Good Bytes)	|\n",
		   mtk_r32(eth, mib_base));
	seq_printf(seq, "| GDMA1_RX_GPCNT  : %010u (Rx Good Pkts)	|\n",
		   mtk_r32(eth, mib_base+0x08));
	seq_printf(seq, "| GDMA1_RX_OERCNT : %010u (overflow error)	|\n",
		   mtk_r32(eth, mib_base+0x10));
	seq_printf(seq, "| GDMA1_RX_FERCNT : %010u (FCS error)	|\n",
		   mtk_r32(eth, mib_base+0x14));
	seq_printf(seq, "| GDMA1_RX_SERCNT : %010u (too short)	|\n",
		   mtk_r32(eth, mib_base+0x18));
	seq_printf(seq, "| GDMA1_RX_LERCNT : %010u (too long)	|\n",
		   mtk_r32(eth, mib_base+0x1C));
	seq_printf(seq, "| GDMA1_RX_CERCNT : %010u (checksum error)	|\n",
		   mtk_r32(eth, mib_base+0x20));
	seq_printf(seq, "| GDMA1_RX_FCCNT  : %010u (flow control)	|\n",
		   mtk_r32(eth, mib_base+0x24));
	seq_printf(seq, "| GDMA1_TX_SKIPCNT: %010u (about count)	|\n",
		   mtk_r32(eth, mib_base+0x28));
	seq_printf(seq, "| GDMA1_TX_COLCNT : %010u (collision count)	|\n",
		   mtk_r32(eth, mib_base+0x2C));
	seq_printf(seq, "| GDMA1_TX_GBCNT  : %010u (Tx Good Bytes)	|\n",
		   mtk_r32(eth, mib_base+0x30));
	seq_printf(seq, "| GDMA1_TX_GPCNT  : %010u (Tx Good Pkts)	|\n",
		   mtk_r32(eth, mib_base+0x38));
	seq_puts(seq, "|						|\n");
	seq_printf(seq, "| GDMA2_RX_GBCNT  : %010u (Rx Good Bytes)	|\n",
		   mtk_r32(eth, mib_base+0x40));
	seq_printf(seq, "| GDMA2_RX_GPCNT  : %010u (Rx Good Pkts)	|\n",
		   mtk_r32(eth, mib_base+0x48));
	seq_printf(seq, "| GDMA2_RX_OERCNT : %010u (overflow error)	|\n",
		   mtk_r32(eth, mib_base+0x50));
	seq_printf(seq, "| GDMA2_RX_FERCNT : %010u (FCS error)	|\n",
		   mtk_r32(eth, mib_base+0x54));
	seq_printf(seq, "| GDMA2_RX_SERCNT : %010u (too short)	|\n",
		   mtk_r32(eth, mib_base+0x58));
	seq_printf(seq, "| GDMA2_RX_LERCNT : %010u (too long)	|\n",
		   mtk_r32(eth, mib_base+0x5C));
	seq_printf(seq, "| GDMA2_RX_CERCNT : %010u (checksum error)	|\n",
		   mtk_r32(eth, mib_base+0x60));
	seq_printf(seq, "| GDMA2_RX_FCCNT  : %010u (flow control)	|\n",
		   mtk_r32(eth, mib_base+0x64));
	seq_printf(seq, "| GDMA2_TX_SKIPCNT: %010u (skip)		|\n",
		   mtk_r32(eth, mib_base+0x68));
	seq_printf(seq, "| GDMA2_TX_COLCNT : %010u (collision)	|\n",
		   mtk_r32(eth, mib_base+0x6C));
	seq_printf(seq, "| GDMA2_TX_GBCNT  : %010u (Tx Good Bytes)	|\n",
		   mtk_r32(eth, mib_base+0x70));
	seq_printf(seq, "| GDMA2_TX_GPCNT  : %010u (Tx Good Pkts)	|\n",
		   mtk_r32(eth, mib_base+0x78));
	seq_puts(seq, "+-----------------------------------------------+\n");

	if (!mt7530_exist(eth))
		return 0;

#define DUMP_EACH_PORT(base)					\
	do { \
		for (i = 0; i < 7; i++) {				\
			pkt_cnt = mt7530_mdio_r32(eth, (base) + (i * 0x100));\
			seq_printf(seq, "%8u ", pkt_cnt);		\
		}							\
		seq_puts(seq, "\n"); \
	} while (0)

	seq_printf(seq, "===================== %8s %8s %8s %8s %8s %8s %8s\n",
		   "Port0", "Port1", "Port2", "Port3", "Port4", "Port5",
		   "Port6");
	seq_puts(seq, "Tx Drop Packet      :");
	DUMP_EACH_PORT(0x4000);
	seq_puts(seq, "Tx CRC Error        :");
	DUMP_EACH_PORT(0x4004);
	seq_puts(seq, "Tx Unicast Packet   :");
	DUMP_EACH_PORT(0x4008);
	seq_puts(seq, "Tx Multicast Packet :");
	DUMP_EACH_PORT(0x400C);
	seq_puts(seq, "Tx Broadcast Packet :");
	DUMP_EACH_PORT(0x4010);
	seq_puts(seq, "Tx Collision Event  :");
	DUMP_EACH_PORT(0x4014);
	seq_puts(seq, "Tx Pause Packet     :");
	DUMP_EACH_PORT(0x402C);
	seq_puts(seq, "Rx Drop Packet      :");
	DUMP_EACH_PORT(0x4060);
	seq_puts(seq, "Rx Filtering Packet :");
	DUMP_EACH_PORT(0x4064);
	seq_puts(seq, "Rx Unicast Packet   :");
	DUMP_EACH_PORT(0x4068);
	seq_puts(seq, "Rx Multicast Packet :");
	DUMP_EACH_PORT(0x406C);
	seq_puts(seq, "Rx Broadcast Packet :");
	DUMP_EACH_PORT(0x4070);
	seq_puts(seq, "Rx Alignment Error  :");
	DUMP_EACH_PORT(0x4074);
	seq_puts(seq, "Rx CRC Error	    :");
	DUMP_EACH_PORT(0x4078);
	seq_puts(seq, "Rx Undersize Error  :");
	DUMP_EACH_PORT(0x407C);
	seq_puts(seq, "Rx Fragment Error   :");
	DUMP_EACH_PORT(0x4080);
	seq_puts(seq, "Rx Oversize Error   :");
	DUMP_EACH_PORT(0x4084);
	seq_puts(seq, "Rx Jabber Error     :");
	DUMP_EACH_PORT(0x4088);
	seq_puts(seq, "Rx Pause Packet     :");
	DUMP_EACH_PORT(0x408C);
	mt7530_mdio_w32(eth, 0x4fe0, 0xf0);
	mt7530_mdio_w32(eth, 0x4fe0, 0x800000f0);

	seq_puts(seq, "\n");

	return 0;
}

static int switch_count_open(struct inode *inode, struct file *file)
{
	return single_open(file, esw_cnt_read, 0);
}

static const struct file_operations switch_count_fops = {
	.owner = THIS_MODULE,
	.open = switch_count_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

static struct proc_dir_entry *proc_tx_ring, *proc_rx_ring;

int tx_ring_read(struct seq_file *seq, void *v)
{
	struct mtk_tx_ring *ring = &g_eth->tx_ring;
	struct mtk_tx_dma *tx_ring;
	int i = 0;

	tx_ring =
	    kmalloc(sizeof(struct mtk_tx_dma) * MTK_DMA_SIZE, GFP_KERNEL);
	if (!tx_ring) {
		seq_puts(seq, " allocate temp tx_ring fail.\n");
		return 0;
	}

	for (i = 0; i < MTK_DMA_SIZE; i++)
		tx_ring[i] = ring->dma[i];

	seq_printf(seq, "free count = %d\n", (int)atomic_read(&ring->free_count));
	seq_printf(seq, "cpu next free: %d\n", (int)(ring->next_free - ring->dma));
	seq_printf(seq, "cpu last free: %d\n", (int)(ring->last_free - ring->dma));
	for (i = 0; i < MTK_DMA_SIZE; i++) {
		dma_addr_t tmp = ring->phys + i * sizeof(*tx_ring);

		seq_printf(seq, "%d (%pad): %08x %08x %08x %08x", i, &tmp,
			   *(int *)&tx_ring[i].txd1, *(int *)&tx_ring[i].txd2,
			   *(int *)&tx_ring[i].txd3, *(int *)&tx_ring[i].txd4);
#if defined(CONFIG_MEDIATEK_NETSYS_V2)
		seq_printf(seq, " %08x %08x %08x %08x",
			   *(int *)&tx_ring[i].txd5, *(int *)&tx_ring[i].txd6,
			   *(int *)&tx_ring[i].txd7, *(int *)&tx_ring[i].txd8);
#endif
		seq_printf(seq, "\n");
	}

	kfree(tx_ring);
	return 0;
}

static int tx_ring_open(struct inode *inode, struct file *file)
{
	return single_open(file, tx_ring_read, NULL);
}

static const struct file_operations tx_ring_fops = {
	.owner = THIS_MODULE,
	.open = tx_ring_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

int rx_ring_read(struct seq_file *seq, void *v)
{
	struct mtk_rx_ring *ring = &g_eth->rx_ring[0];
	struct mtk_rx_dma *rx_ring;

	int i = 0;

	rx_ring =
	    kmalloc(sizeof(struct mtk_rx_dma) * MTK_DMA_SIZE, GFP_KERNEL);
	if (!rx_ring) {
		seq_puts(seq, " allocate temp rx_ring fail.\n");
		return 0;
	}

	for (i = 0; i < MTK_DMA_SIZE; i++)
		rx_ring[i] = ring->dma[i];

	seq_printf(seq, "next to read: %d\n",
		   NEXT_DESP_IDX(ring->calc_idx, MTK_DMA_SIZE));
	for (i = 0; i < MTK_DMA_SIZE; i++) {
		seq_printf(seq, "%d: %08x %08x %08x %08x", i,
			   *(int *)&rx_ring[i].rxd1, *(int *)&rx_ring[i].rxd2,
			   *(int *)&rx_ring[i].rxd3, *(int *)&rx_ring[i].rxd4);
#ifdef CONFIG_MEDIATEK_NETSYS_RX_V2
		seq_printf(seq, " %08x %08x %08x %08x",
			   *(int *)&rx_ring[i].rxd5, *(int *)&rx_ring[i].rxd6,
			   *(int *)&rx_ring[i].rxd7, *(int *)&rx_ring[i].rxd8);
#endif
		seq_printf(seq, "\n");
	}

	kfree(rx_ring);
	return 0;
}

static int rx_ring_open(struct inode *inode, struct file *file)
{
	return single_open(file, rx_ring_read, NULL);
}

static const struct file_operations rx_ring_fops = {
	.owner = THIS_MODULE,
	.open = rx_ring_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

int dbg_regs_read(struct seq_file *seq, void *v)
{
	struct mtk_eth *eth = g_eth;

	seq_puts(seq, "   <<PSE DEBUG REG DUMP>>\n");
	seq_printf(seq, "| PSE_FQFC_CFG	: %08x |\n",
		   mtk_r32(eth, MTK_PSE_FQFC_CFG));
	seq_printf(seq, "| PSE_IQ_STA1	: %08x |\n",
		   mtk_r32(eth, MTK_PSE_IQ_STA(0)));
	seq_printf(seq, "| PSE_IQ_STA2	: %08x |\n",
		   mtk_r32(eth, MTK_PSE_IQ_STA(1)));

	if (MTK_HAS_CAPS(eth->soc->caps, MTK_NETSYS_TX_V2)) {
		seq_printf(seq, "| PSE_IQ_STA3	: %08x |\n",
			   mtk_r32(eth, MTK_PSE_IQ_STA(2)));
		seq_printf(seq, "| PSE_IQ_STA4	: %08x |\n",
			   mtk_r32(eth, MTK_PSE_IQ_STA(3)));
	}

	seq_printf(seq, "| PSE_OQ_STA1	: %08x |\n",
		   mtk_r32(eth, MTK_PSE_OQ_STA(0)));
	seq_printf(seq, "| PSE_OQ_STA2	: %08x |\n",
		   mtk_r32(eth, MTK_PSE_OQ_STA(1)));

	if (MTK_HAS_CAPS(eth->soc->caps, MTK_NETSYS_TX_V2)) {
		seq_printf(seq, "| PSE_OQ_STA3	: %08x |\n",
			   mtk_r32(eth, MTK_PSE_OQ_STA(2)));
		seq_printf(seq, "| PSE_OQ_STA4	: %08x |\n",
			   mtk_r32(eth, MTK_PSE_OQ_STA(3)));
	}

	seq_printf(seq, "| QDMA_FQ_CNT	: %08x |\n",
		   mtk_r32(eth, MTK_QDMA_FQ_CNT));
	seq_printf(seq, "| FE_PSE_FREE	: %08x |\n",
		   mtk_r32(eth, MTK_FE_PSE_FREE));
	seq_printf(seq, "| FE_DROP_FQ	: %08x |\n",
		   mtk_r32(eth, MTK_FE_DROP_FQ));
	seq_printf(seq, "| FE_DROP_FC	: %08x |\n",
		   mtk_r32(eth, MTK_FE_DROP_FC));
	seq_printf(seq, "| FE_DROP_PPE	: %08x |\n",
		   mtk_r32(eth, MTK_FE_DROP_PPE));
	seq_printf(seq, "| GDM1_IG_CTRL	: %08x |\n",
		   mtk_r32(eth, MTK_GDMA_FWD_CFG(0)));
	seq_printf(seq, "| GDM2_IG_CTRL	: %08x |\n",
		   mtk_r32(eth, MTK_GDMA_FWD_CFG(1)));
	seq_printf(seq, "| MAC_P1_MCR	: %08x |\n",
		   mtk_r32(eth, MTK_MAC_MCR(0)));
	seq_printf(seq, "| MAC_P2_MCR	: %08x |\n",
		   mtk_r32(eth, MTK_MAC_MCR(1)));

	if (MTK_HAS_CAPS(eth->soc->caps, MTK_NETSYS_TX_V2)) {
		seq_printf(seq, "| FE_CDM1_FSM	: %08x |\n",
			   mtk_r32(eth, MTK_FE_CDM1_FSM));
		seq_printf(seq, "| FE_CDM2_FSM	: %08x |\n",
			   mtk_r32(eth, MTK_FE_CDM2_FSM));
		seq_printf(seq, "| FE_GDM1_FSM	: %08x |\n",
			   mtk_r32(eth, MTK_FE_GDM1_FSM));
		seq_printf(seq, "| FE_GDM2_FSM	: %08x |\n",
			   mtk_r32(eth, MTK_FE_GDM2_FSM));
	}

	return 0;
}

static int dbg_regs_open(struct inode *inode, struct file *file)
{
	return single_open(file, dbg_regs_read, 0);
}

static const struct file_operations dbg_regs_fops = {
	.owner = THIS_MODULE,
	.open = dbg_regs_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

#define PROCREG_ESW_CNT         "esw_cnt"
#define PROCREG_TXRING          "tx_ring"
#define PROCREG_RXRING          "rx_ring"
#define PROCREG_DIR             "mtketh"
#define PROCREG_DBG_REGS        "dbg_regs"

struct proc_dir_entry *proc_reg_dir;
static struct proc_dir_entry *proc_esw_cnt, *proc_dbg_regs;

int debug_proc_init(struct mtk_eth *eth)
{
	g_eth = eth;

	if (!proc_reg_dir)
		proc_reg_dir = proc_mkdir(PROCREG_DIR, NULL);

	proc_tx_ring =
	    proc_create(PROCREG_TXRING, 0, proc_reg_dir, &tx_ring_fops);
	if (!proc_tx_ring)
		pr_notice("!! FAIL to create %s PROC !!\n", PROCREG_TXRING);

	proc_rx_ring =
	    proc_create(PROCREG_RXRING, 0, proc_reg_dir, &rx_ring_fops);
	if (!proc_rx_ring)
		pr_notice("!! FAIL to create %s PROC !!\n", PROCREG_RXRING);

	proc_esw_cnt =
	    proc_create(PROCREG_ESW_CNT, 0, proc_reg_dir, &switch_count_fops);
	if (!proc_esw_cnt)
		pr_notice("!! FAIL to create %s PROC !!\n", PROCREG_ESW_CNT);

	proc_dbg_regs =
	    proc_create(PROCREG_DBG_REGS, 0, proc_reg_dir, &dbg_regs_fops);
	if (!proc_dbg_regs)
		pr_notice("!! FAIL to create %s PROC !!\n", PROCREG_DBG_REGS);

	return 0;
}

void debug_proc_exit(void)
{
	if (proc_tx_ring)
		remove_proc_entry(PROCREG_TXRING, proc_reg_dir);
	if (proc_rx_ring)
		remove_proc_entry(PROCREG_RXRING, proc_reg_dir);

	if (proc_esw_cnt)
		remove_proc_entry(PROCREG_ESW_CNT, proc_reg_dir);

	if (proc_reg_dir)
		remove_proc_entry(PROCREG_DIR, 0);

	if (proc_dbg_regs)
		remove_proc_entry(PROCREG_DBG_REGS, proc_reg_dir);
}

