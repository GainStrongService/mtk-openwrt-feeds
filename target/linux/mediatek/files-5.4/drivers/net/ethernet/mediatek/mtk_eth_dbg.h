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

#ifndef MTK_ETH_DBG_H
#define MTK_ETH_DBG_H

/* Debug Purpose Register */
#define MTK_PSE_FQFC_CFG		0x100
#define MTK_FE_CDM1_FSM			0x220
#define MTK_FE_CDM2_FSM			0x224
#define MTK_FE_GDM1_FSM			0x228
#define MTK_FE_GDM2_FSM			0x22C
#define MTK_FE_PSE_FREE			0x240
#define MTK_FE_DROP_FQ			0x244
#define MTK_FE_DROP_FC			0x248
#define MTK_FE_DROP_PPE			0x24C

#if defined(CONFIG_MEDIATEK_NETSYS_V2)
#define MTK_PSE_IQ_STA(x)		(0x180 + (x) * 0x4)
#define MTK_PSE_OQ_STA(x)		(0x1A0 + (x) * 0x4)
#else
#define MTK_PSE_IQ_STA(x)		(0x110 + (x) * 0x4)
#define MTK_PSE_OQ_STA(x)		(0x118 + (x) * 0x4)
#endif

#define MTKETH_MII_READ                  0x89F3
#define MTKETH_MII_WRITE                 0x89F4
#define MTKETH_ESW_REG_READ              0x89F1
#define MTKETH_ESW_REG_WRITE             0x89F2
#define MTKETH_MII_READ_CL45             0x89FC
#define MTKETH_MII_WRITE_CL45            0x89FD
#define REG_ESW_MAX                     0xFC

struct mtk_esw_reg {
	unsigned int off;
	unsigned int val;
};

struct mtk_mii_ioctl_data {
	u16 phy_id;
	u16 reg_num;
	unsigned int val_in;
	unsigned int val_out;
};

#if defined(CONFIG_NET_DSA_MT7530) || defined(CONFIG_MT753X_GSW)
static inline bool mt7530_exist(struct mtk_eth *eth)
{
	return true;
}
#else
static inline bool mt7530_exist(struct mtk_eth *eth)
{
	return false;
}
#endif

extern u32 _mtk_mdio_read(struct mtk_eth *eth, u16 phy_addr, u16 phy_reg);
extern u32 _mtk_mdio_write(struct mtk_eth *eth, u16 phy_addr,
		    u16 phy_register, u16 write_data);

extern u32 mtk_cl45_ind_read(struct mtk_eth *eth, u16 port, u16 devad, u16 reg, u16 *data);
extern u32 mtk_cl45_ind_write(struct mtk_eth *eth, u16 port, u16 devad, u16 reg, u16 data);

int debug_proc_init(struct mtk_eth *eth);
void debug_proc_exit(void);

int mtketh_debugfs_init(struct mtk_eth *eth);
void mtketh_debugfs_exit(struct mtk_eth *eth);
int mtk_do_priv_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd);

#endif /* MTK_ETH_DBG_H */
