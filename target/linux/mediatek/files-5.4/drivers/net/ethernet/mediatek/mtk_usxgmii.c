/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (c) 2022 MediaTek Inc.
 * Author: Henry Yen <henry.yen@mediatek.com>
 */

#include <linux/mfd/syscon.h>
#include <linux/of.h>
#include <linux/regmap.h>
#include "mtk_eth_soc.h"

static struct mtk_usxgmii_pcs *pcs_to_mtk_usxgmii_pcs(struct phylink_pcs *pcs)
{
	return container_of(pcs, struct mtk_usxgmii_pcs, pcs);
}

int mtk_usxgmii_xfi_pextp_init(struct mtk_usxgmii *ss, struct device_node *r)
{
	struct device_node *np;
	int i;

	for (i = 0; i < MTK_MAX_DEVS; i++) {
		np = of_parse_phandle(r, "mediatek,xfi_pextp", i);
		if (!np)
			break;

		ss->pcs[i].regmap_pextp = syscon_node_to_regmap(np);
		if (IS_ERR(ss->pcs[i].regmap_pextp))
			return PTR_ERR(ss->pcs[i].regmap_pextp);

		of_node_put(np);
	}

	return 0;
}

int mtk_usxgmii_xfi_pll_init(struct mtk_usxgmii *ss, struct device_node *r)
{
	struct device_node *np;
	int i;

	np = of_parse_phandle(r, "mediatek,xfi_pll", 0);
	if (!np)
		return -1;

	for (i = 0; i < MTK_MAX_DEVS; i++) {
		ss->pll = syscon_node_to_regmap(np);
		if (IS_ERR(ss->pll))
			return PTR_ERR(ss->pll);
	}

	of_node_put(np);

	return 0;
}

int mtk_toprgu_init(struct mtk_eth *eth, struct device_node *r)
{
	struct device_node *np;

	np = of_parse_phandle(r, "mediatek,toprgu", 0);
	if (!np)
		return -1;

	eth->toprgu = syscon_node_to_regmap(np);
	if (IS_ERR(eth->toprgu))
		return PTR_ERR(eth->toprgu);

	return 0;
}

static int mtk_usxgmii_xfi_pll_enable(struct mtk_usxgmii *ss)
{
	u32 val = 0;

	if (!ss->pll)
		return -EINVAL;

	/* Add software workaround for USXGMII PLL TCL issue */
	regmap_write(ss->pll, XFI_PLL_ANA_GLB8, RG_XFI_PLL_ANA_SWWA);

	regmap_read(ss->pll, XFI_PLL_DIG_GLB8, &val);
	val |= RG_XFI_PLL_EN;
	regmap_write(ss->pll, XFI_PLL_DIG_GLB8, val);

	return 0;
}

int mtk_mac2xgmii_id(struct mtk_eth *eth, int mac_id)
{
	u32 xgmii_id = mac_id;

	if (MTK_HAS_CAPS(eth->soc->caps, MTK_NETSYS_V3)) {
		switch (mac_id) {
		case MTK_GMAC1_ID:
		case MTK_GMAC2_ID:
			xgmii_id = 1;
			break;
		case MTK_GMAC3_ID:
			xgmii_id = 0;
			break;
		default:
			pr_info("[%s] Warning: get illegal mac_id=%d !=!!!\n",
				__func__, mac_id);
		}
	}

	return xgmii_id;
}

int mtk_xgmii2mac_id(struct mtk_eth *eth, int xgmii_id)
{
	u32 mac_id = xgmii_id;

	if (MTK_HAS_CAPS(eth->soc->caps, MTK_NETSYS_V3)) {
		switch (xgmii_id) {
		case 0:
			mac_id = 2;
			break;
		case 1:
			mac_id = 1;
			break;
		default:
			pr_info("[%s] Warning: get illegal xgmii_id=%d !=!!!\n",
				__func__, xgmii_id);
		}
	}

	return mac_id;
}

int mtk_usxgmii_setup_phya_an_10000(struct mtk_usxgmii_pcs *mpcs)
{
	if (!mpcs->regmap || !mpcs->regmap_pextp)
		return -EINVAL;

	regmap_update_bits(mpcs->regmap, 0x810, GENMASK(31, 0),
			   0x000FFE6D);
	regmap_update_bits(mpcs->regmap, 0x818, GENMASK(31, 0),
			   0x07B1EC7B);
	regmap_update_bits(mpcs->regmap, 0x80C, GENMASK(31, 0),
			   0x30000000);
	ndelay(1020);
	regmap_update_bits(mpcs->regmap, 0x80C, GENMASK(31, 0),
			   0x10000000);
	ndelay(1020);
	regmap_update_bits(mpcs->regmap, 0x80C, GENMASK(31, 0),
			   0x00000000);

	regmap_update_bits(mpcs->regmap_pextp, 0x9024, GENMASK(31, 0),
			   0x00C9071C);
	regmap_update_bits(mpcs->regmap_pextp, 0x2020, GENMASK(31, 0),
			   0xAA8585AA);
	regmap_update_bits(mpcs->regmap_pextp, 0x2030, GENMASK(31, 0),
			   0x0C020707);
	regmap_update_bits(mpcs->regmap_pextp, 0x2034, GENMASK(31, 0),
			   0x0E050F0F);
	regmap_update_bits(mpcs->regmap_pextp, 0x2040, GENMASK(31, 0),
			   0x00140032);
	regmap_update_bits(mpcs->regmap_pextp, 0x50F0, GENMASK(31, 0),
			   0x00C014AA);
	regmap_update_bits(mpcs->regmap_pextp, 0x50E0, GENMASK(31, 0),
			   0x3777C12B);
	regmap_update_bits(mpcs->regmap_pextp, 0x506C, GENMASK(31, 0),
			   0x005F9CFF);
	regmap_update_bits(mpcs->regmap_pextp, 0x5070, GENMASK(31, 0),
			   0x9D9DFAFA);
	regmap_update_bits(mpcs->regmap_pextp, 0x5074, GENMASK(31, 0),
			   0x27273F3F);
	regmap_update_bits(mpcs->regmap_pextp, 0x5078, GENMASK(31, 0),
			   0xA7883C68);
	regmap_update_bits(mpcs->regmap_pextp, 0x507C, GENMASK(31, 0),
			   0x11661166);
	regmap_update_bits(mpcs->regmap_pextp, 0x5080, GENMASK(31, 0),
			   0x0E000AAF);
	regmap_update_bits(mpcs->regmap_pextp, 0x5084, GENMASK(31, 0),
			   0x08080D0D);
	regmap_update_bits(mpcs->regmap_pextp, 0x5088, GENMASK(31, 0),
			   0x02030909);
	regmap_update_bits(mpcs->regmap_pextp, 0x50E4, GENMASK(31, 0),
			   0x0C0C0000);
	regmap_update_bits(mpcs->regmap_pextp, 0x50E8, GENMASK(31, 0),
			   0x04040000);
	regmap_update_bits(mpcs->regmap_pextp, 0x50EC, GENMASK(31, 0),
			   0x0F0F0C06);
	regmap_update_bits(mpcs->regmap_pextp, 0x50A8, GENMASK(31, 0),
			   0x506E8C8C);
	regmap_update_bits(mpcs->regmap_pextp, 0x6004, GENMASK(31, 0),
			   0x18190000);
	regmap_update_bits(mpcs->regmap_pextp, 0x00F8, GENMASK(31, 0),
			   0x01423342);
	regmap_update_bits(mpcs->regmap_pextp, 0x00F4, GENMASK(31, 0),
			   0x80201F20);
	regmap_update_bits(mpcs->regmap_pextp, 0x0030, GENMASK(31, 0),
			   0x00050C00);
	regmap_update_bits(mpcs->regmap_pextp, 0x0070, GENMASK(31, 0),
			   0x02002800);
	ndelay(1020);
	regmap_update_bits(mpcs->regmap_pextp, 0x30B0, GENMASK(31, 0),
			   0x00000020);
	regmap_update_bits(mpcs->regmap_pextp, 0x3028, GENMASK(31, 0),
			   0x00008A01);
	regmap_update_bits(mpcs->regmap_pextp, 0x302C, GENMASK(31, 0),
			   0x0000A884);
	regmap_update_bits(mpcs->regmap_pextp, 0x3024, GENMASK(31, 0),
			   0x00083002);
	regmap_update_bits(mpcs->regmap_pextp, 0x3010, GENMASK(31, 0),
			   0x00022220);
	regmap_update_bits(mpcs->regmap_pextp, 0x5064, GENMASK(31, 0),
			   0x0F020A01);
	regmap_update_bits(mpcs->regmap_pextp, 0x50B4, GENMASK(31, 0),
			   0x06100600);
	regmap_update_bits(mpcs->regmap_pextp, 0x3048, GENMASK(31, 0),
			   0x40704000);
	regmap_update_bits(mpcs->regmap_pextp, 0x3050, GENMASK(31, 0),
			   0xA8000000);
	regmap_update_bits(mpcs->regmap_pextp, 0x3054, GENMASK(31, 0),
			   0x000000AA);
	regmap_update_bits(mpcs->regmap_pextp, 0x306C, GENMASK(31, 0),
			   0x00000F00);
	regmap_update_bits(mpcs->regmap_pextp, 0xA060, GENMASK(31, 0),
			   0x00040000);
	regmap_update_bits(mpcs->regmap_pextp, 0x90D0, GENMASK(31, 0),
			   0x00000001);
	regmap_update_bits(mpcs->regmap_pextp, 0x0070, GENMASK(31, 0),
			   0x0200E800);
	udelay(150);
	regmap_update_bits(mpcs->regmap_pextp, 0x0070, GENMASK(31, 0),
			   0x0200C111);
	ndelay(1020);
	regmap_update_bits(mpcs->regmap_pextp, 0x0070, GENMASK(31, 0),
			   0x0200C101);
	udelay(15);
	regmap_update_bits(mpcs->regmap_pextp, 0x0070, GENMASK(31, 0),
			   0x0202C111);
	ndelay(1020);
	regmap_update_bits(mpcs->regmap_pextp, 0x0070, GENMASK(31, 0),
			   0x0202C101);
	udelay(100);
	regmap_update_bits(mpcs->regmap_pextp, 0x30B0, GENMASK(31, 0),
			   0x00000030);
	regmap_update_bits(mpcs->regmap_pextp, 0x00F4, GENMASK(31, 0),
			   0x80201F00);
	regmap_update_bits(mpcs->regmap_pextp, 0x3040, GENMASK(31, 0),
			   0x30000000);
	udelay(400);

	return 0;
}

int mtk_usxgmii_setup_phya_force_5000(struct mtk_usxgmii_pcs *mpcs)
{
	unsigned int val;

	if (!mpcs->regmap || !mpcs->regmap_pextp)
		return -EINVAL;

	/* Setup USXGMII speed */
	val = FIELD_PREP(RG_XFI_RX_MODE, RG_XFI_RX_MODE_5G) |
	      FIELD_PREP(RG_XFI_TX_MODE, RG_XFI_TX_MODE_5G);
	regmap_write(mpcs->regmap, RG_PHY_TOP_SPEED_CTRL1, val);

	/* Disable USXGMII AN mode */
	regmap_read(mpcs->regmap, RG_PCS_AN_CTRL0, &val);
	val &= ~USXGMII_AN_ENABLE;
	regmap_write(mpcs->regmap, RG_PCS_AN_CTRL0, val);

	/* Gated USXGMII */
	regmap_read(mpcs->regmap, RG_PHY_TOP_SPEED_CTRL1, &val);
	val |= RG_MAC_CK_GATED;
	regmap_write(mpcs->regmap, RG_PHY_TOP_SPEED_CTRL1, val);

	ndelay(1020);

	/* USXGMII force mode setting */
	regmap_read(mpcs->regmap, RG_PHY_TOP_SPEED_CTRL1, &val);
	val |= RG_USXGMII_RATE_UPDATE_MODE;
	val |= RG_IF_FORCE_EN;
	val |= FIELD_PREP(RG_RATE_ADAPT_MODE, RG_RATE_ADAPT_MODE_X1);
	regmap_write(mpcs->regmap, RG_PHY_TOP_SPEED_CTRL1, val);

	/* Un-gated USXGMII */
	regmap_read(mpcs->regmap, RG_PHY_TOP_SPEED_CTRL1, &val);
	val &= ~RG_MAC_CK_GATED;
	regmap_write(mpcs->regmap, RG_PHY_TOP_SPEED_CTRL1, val);

	ndelay(1020);

	regmap_update_bits(mpcs->regmap_pextp, 0x9024, GENMASK(31, 0),
			   0x00D9071C);
	regmap_update_bits(mpcs->regmap_pextp, 0x2020, GENMASK(31, 0),
			   0xAAA5A5AA);
	regmap_update_bits(mpcs->regmap_pextp, 0x2030, GENMASK(31, 0),
			   0x0C020707);
	regmap_update_bits(mpcs->regmap_pextp, 0x2034, GENMASK(31, 0),
			   0x0E050F0F);
	regmap_update_bits(mpcs->regmap_pextp, 0x2040, GENMASK(31, 0),
			   0x00140032);
	regmap_update_bits(mpcs->regmap_pextp, 0x50F0, GENMASK(31, 0),
			   0x00C018AA);
	regmap_update_bits(mpcs->regmap_pextp, 0x50E0, GENMASK(31, 0),
			   0x3777812B);
	regmap_update_bits(mpcs->regmap_pextp, 0x506C, GENMASK(31, 0),
			   0x005C9CFF);
	regmap_update_bits(mpcs->regmap_pextp, 0x5070, GENMASK(31, 0),
			   0x9DFAFAFA);
	regmap_update_bits(mpcs->regmap_pextp, 0x5074, GENMASK(31, 0),
			   0x273F3F3F);
	regmap_update_bits(mpcs->regmap_pextp, 0x5078, GENMASK(31, 0),
			   0xA8883868);
	regmap_update_bits(mpcs->regmap_pextp, 0x507C, GENMASK(31, 0),
			   0x14661466);
	regmap_update_bits(mpcs->regmap_pextp, 0x5080, GENMASK(31, 0),
			   0x0E001ABF);
	regmap_update_bits(mpcs->regmap_pextp, 0x5084, GENMASK(31, 0),
			   0x080B0D0D);
	regmap_update_bits(mpcs->regmap_pextp, 0x5088, GENMASK(31, 0),
			   0x02050909);
	regmap_update_bits(mpcs->regmap_pextp, 0x50E4, GENMASK(31, 0),
			   0x0C000000);
	regmap_update_bits(mpcs->regmap_pextp, 0x50E8, GENMASK(31, 0),
			   0x04000000);
	regmap_update_bits(mpcs->regmap_pextp, 0x50EC, GENMASK(31, 0),
			   0x0F0F0C06);
	regmap_update_bits(mpcs->regmap_pextp, 0x50A8, GENMASK(31, 0),
			   0x50808C8C);
	regmap_update_bits(mpcs->regmap_pextp, 0x6004, GENMASK(31, 0),
			   0x18000000);
	regmap_update_bits(mpcs->regmap_pextp, 0x00F8, GENMASK(31, 0),
			   0x00A132A1);
	regmap_update_bits(mpcs->regmap_pextp, 0x00F4, GENMASK(31, 0),
			   0x80201F20);
	regmap_update_bits(mpcs->regmap_pextp, 0x0030, GENMASK(31, 0),
			   0x00050C00);
	regmap_update_bits(mpcs->regmap_pextp, 0x0070, GENMASK(31, 0),
			   0x02002800);
	ndelay(1020);
	regmap_update_bits(mpcs->regmap_pextp, 0x30B0, GENMASK(31, 0),
			   0x00000020);
	regmap_update_bits(mpcs->regmap_pextp, 0x3028, GENMASK(31, 0),
			   0x00008A01);
	regmap_update_bits(mpcs->regmap_pextp, 0x302C, GENMASK(31, 0),
			   0x0000A884);
	regmap_update_bits(mpcs->regmap_pextp, 0x3024, GENMASK(31, 0),
			   0x00083002);
	regmap_update_bits(mpcs->regmap_pextp, 0x3010, GENMASK(31, 0),
			   0x00022220);
	regmap_update_bits(mpcs->regmap_pextp, 0x5064, GENMASK(31, 0),
			   0x0F020A01);
	regmap_update_bits(mpcs->regmap_pextp, 0x50B4, GENMASK(31, 0),
			   0x06100600);
	regmap_update_bits(mpcs->regmap_pextp, 0x3048, GENMASK(31, 0),
			   0x40704000);
	regmap_update_bits(mpcs->regmap_pextp, 0x3050, GENMASK(31, 0),
			   0xA8000000);
	regmap_update_bits(mpcs->regmap_pextp, 0x3054, GENMASK(31, 0),
			   0x000000AA);
	regmap_update_bits(mpcs->regmap_pextp, 0x306C, GENMASK(31, 0),
			   0x00000F00);
	regmap_update_bits(mpcs->regmap_pextp, 0xA060, GENMASK(31, 0),
			   0x00040000);
	regmap_update_bits(mpcs->regmap_pextp, 0x90D0, GENMASK(31, 0),
			   0x00000003);
	regmap_update_bits(mpcs->regmap_pextp, 0x0070, GENMASK(31, 0),
			   0x0200E800);
	udelay(150);
	regmap_update_bits(mpcs->regmap_pextp, 0x0070, GENMASK(31, 0),
			   0x0200C111);
	ndelay(1020);
	regmap_update_bits(mpcs->regmap_pextp, 0x0070, GENMASK(31, 0),
			   0x0200C101);
	udelay(15);
	regmap_update_bits(mpcs->regmap_pextp, 0x0070, GENMASK(31, 0),
			   0x0202C111);
	ndelay(1020);
	regmap_update_bits(mpcs->regmap_pextp, 0x0070, GENMASK(31, 0),
			   0x0202C101);
	udelay(100);
	regmap_update_bits(mpcs->regmap_pextp, 0x30B0, GENMASK(31, 0),
			   0x00000030);
	regmap_update_bits(mpcs->regmap_pextp, 0x00F4, GENMASK(31, 0),
			   0x80201F00);
	regmap_update_bits(mpcs->regmap_pextp, 0x3040, GENMASK(31, 0),
			   0x30000000);
	udelay(400);

	return 0;
}

int mtk_usxgmii_setup_phya_force_10000(struct mtk_usxgmii_pcs *mpcs)
{
	unsigned int val;

	if (!mpcs->regmap || !mpcs->regmap_pextp)
		return -EINVAL;

	/* Setup USXGMII speed */
	val = FIELD_PREP(RG_XFI_RX_MODE, RG_XFI_RX_MODE_10G) |
	      FIELD_PREP(RG_XFI_TX_MODE, RG_XFI_TX_MODE_10G);
	regmap_write(mpcs->regmap, RG_PHY_TOP_SPEED_CTRL1, val);

	/* Disable USXGMII AN mode */
	regmap_read(mpcs->regmap, RG_PCS_AN_CTRL0, &val);
	val &= ~USXGMII_AN_ENABLE;
	regmap_write(mpcs->regmap, RG_PCS_AN_CTRL0, val);

	/* Gated USXGMII */
	regmap_read(mpcs->regmap, RG_PHY_TOP_SPEED_CTRL1, &val);
	val |= RG_MAC_CK_GATED;
	regmap_write(mpcs->regmap, RG_PHY_TOP_SPEED_CTRL1, val);

	ndelay(1020);

	/* USXGMII force mode setting */
	regmap_read(mpcs->regmap, RG_PHY_TOP_SPEED_CTRL1, &val);
	val |= RG_USXGMII_RATE_UPDATE_MODE;
	val |= RG_IF_FORCE_EN;
	val |= FIELD_PREP(RG_RATE_ADAPT_MODE, RG_RATE_ADAPT_MODE_X1);
	regmap_write(mpcs->regmap, RG_PHY_TOP_SPEED_CTRL1, val);

	/* Un-gated USXGMII */
	regmap_read(mpcs->regmap, RG_PHY_TOP_SPEED_CTRL1, &val);
	val &= ~RG_MAC_CK_GATED;
	regmap_write(mpcs->regmap, RG_PHY_TOP_SPEED_CTRL1, val);

	ndelay(1020);

	regmap_update_bits(mpcs->regmap_pextp, 0x9024, GENMASK(31, 0),
			   0x00C9071C);
	regmap_update_bits(mpcs->regmap_pextp, 0x2020, GENMASK(31, 0),
			   0xAA8585AA);
	regmap_update_bits(mpcs->regmap_pextp, 0x2030, GENMASK(31, 0),
			   0x0C020707);
	regmap_update_bits(mpcs->regmap_pextp, 0x2034, GENMASK(31, 0),
			   0x0E050F0F);
	regmap_update_bits(mpcs->regmap_pextp, 0x2040, GENMASK(31, 0),
			   0x00140032);
	regmap_update_bits(mpcs->regmap_pextp, 0x50F0, GENMASK(31, 0),
			   0x00C014AA);
	regmap_update_bits(mpcs->regmap_pextp, 0x50E0, GENMASK(31, 0),
			   0x3777C12B);
	regmap_update_bits(mpcs->regmap_pextp, 0x506C, GENMASK(31, 0),
			   0x005F9CFF);
	regmap_update_bits(mpcs->regmap_pextp, 0x5070, GENMASK(31, 0),
			   0x9D9DFAFA);
	regmap_update_bits(mpcs->regmap_pextp, 0x5074, GENMASK(31, 0),
			   0x27273F3F);
	regmap_update_bits(mpcs->regmap_pextp, 0x5078, GENMASK(31, 0),
			   0xA7883C68);
	regmap_update_bits(mpcs->regmap_pextp, 0x507C, GENMASK(31, 0),
			   0x11661166);
	regmap_update_bits(mpcs->regmap_pextp, 0x5080, GENMASK(31, 0),
			   0x0E000AAF);
	regmap_update_bits(mpcs->regmap_pextp, 0x5084, GENMASK(31, 0),
			   0x08080D0D);
	regmap_update_bits(mpcs->regmap_pextp, 0x5088, GENMASK(31, 0),
			   0x02030909);
	regmap_update_bits(mpcs->regmap_pextp, 0x50E4, GENMASK(31, 0),
			   0x0C0C0000);
	regmap_update_bits(mpcs->regmap_pextp, 0x50E8, GENMASK(31, 0),
			   0x04040000);
	regmap_update_bits(mpcs->regmap_pextp, 0x50EC, GENMASK(31, 0),
			   0x0F0F0C06);
	regmap_update_bits(mpcs->regmap_pextp, 0x50A8, GENMASK(31, 0),
			   0x506E8C8C);
	regmap_update_bits(mpcs->regmap_pextp, 0x6004, GENMASK(31, 0),
			   0x18190000);
	regmap_update_bits(mpcs->regmap_pextp, 0x00F8, GENMASK(31, 0),
			   0x01423342);
	regmap_update_bits(mpcs->regmap_pextp, 0x00F4, GENMASK(31, 0),
			   0x80201F20);
	regmap_update_bits(mpcs->regmap_pextp, 0x0030, GENMASK(31, 0),
			   0x00050C00);
	regmap_update_bits(mpcs->regmap_pextp, 0x0070, GENMASK(31, 0),
			   0x02002800);
	ndelay(1020);
	regmap_update_bits(mpcs->regmap_pextp, 0x30B0, GENMASK(31, 0),
			   0x00000020);
	regmap_update_bits(mpcs->regmap_pextp, 0x3028, GENMASK(31, 0),
			   0x00008A01);
	regmap_update_bits(mpcs->regmap_pextp, 0x302C, GENMASK(31, 0),
			   0x0000A884);
	regmap_update_bits(mpcs->regmap_pextp, 0x3024, GENMASK(31, 0),
			   0x00083002);
	regmap_update_bits(mpcs->regmap_pextp, 0x3010, GENMASK(31, 0),
			   0x00022220);
	regmap_update_bits(mpcs->regmap_pextp, 0x5064, GENMASK(31, 0),
			   0x0F020A01);
	regmap_update_bits(mpcs->regmap_pextp, 0x50B4, GENMASK(31, 0),
			   0x06100600);
	regmap_update_bits(mpcs->regmap_pextp, 0x3048, GENMASK(31, 0),
			   0x49664100);
	regmap_update_bits(mpcs->regmap_pextp, 0x3050, GENMASK(31, 0),
			   0x00000000);
	regmap_update_bits(mpcs->regmap_pextp, 0x3054, GENMASK(31, 0),
			   0x00000000);
	regmap_update_bits(mpcs->regmap_pextp, 0x306C, GENMASK(31, 0),
			   0x00000F00);
	regmap_update_bits(mpcs->regmap_pextp, 0xA060, GENMASK(31, 0),
			   0x00040000);
	regmap_update_bits(mpcs->regmap_pextp, 0x90D0, GENMASK(31, 0),
			   0x00000001);
	regmap_update_bits(mpcs->regmap_pextp, 0x0070, GENMASK(31, 0),
			   0x0200E800);
	udelay(150);
	regmap_update_bits(mpcs->regmap_pextp, 0x0070, GENMASK(31, 0),
			   0x0200C111);
	ndelay(1020);
	regmap_update_bits(mpcs->regmap_pextp, 0x0070, GENMASK(31, 0),
			   0x0200C101);
	udelay(15);
	regmap_update_bits(mpcs->regmap_pextp, 0x0070, GENMASK(31, 0),
			   0x0202C111);
	ndelay(1020);
	regmap_update_bits(mpcs->regmap_pextp, 0x0070, GENMASK(31, 0),
			   0x0202C101);
	udelay(100);
	regmap_update_bits(mpcs->regmap_pextp, 0x30B0, GENMASK(31, 0),
			   0x00000030);
	regmap_update_bits(mpcs->regmap_pextp, 0x00F4, GENMASK(31, 0),
			   0x80201F00);
	regmap_update_bits(mpcs->regmap_pextp, 0x3040, GENMASK(31, 0),
			   0x30000000);
	udelay(400);

	return 0;
}

void mtk_usxgmii_reset(struct mtk_eth *eth, int id)
{
	u32 val = 0;

	if (id >= MTK_MAX_DEVS || !eth->toprgu)
		return;

	switch (id) {
	case 0:
		/* Enable software reset */
		regmap_read(eth->toprgu, TOPRGU_SWSYSRST_EN, &val);
		val |= SWSYSRST_XFI_PEXPT0_GRST |
		       SWSYSRST_XFI0_GRST;
		regmap_write(eth->toprgu, TOPRGU_SWSYSRST_EN, val);

		/* Assert USXGMII reset */
		regmap_read(eth->toprgu, TOPRGU_SWSYSRST, &val);
		val |= FIELD_PREP(SWSYSRST_UNLOCK_KEY, 0x88) |
		       SWSYSRST_XFI_PEXPT0_GRST |
		       SWSYSRST_XFI0_GRST;
		regmap_write(eth->toprgu, TOPRGU_SWSYSRST, val);

		udelay(100);

		/* De-assert USXGMII reset */
		regmap_read(eth->toprgu, TOPRGU_SWSYSRST, &val);
		val |= FIELD_PREP(SWSYSRST_UNLOCK_KEY, 0x88);
		val &= ~(SWSYSRST_XFI_PEXPT0_GRST |
			 SWSYSRST_XFI0_GRST);
		regmap_write(eth->toprgu, TOPRGU_SWSYSRST, val);

		/* Disable software reset */
		regmap_read(eth->toprgu, TOPRGU_SWSYSRST_EN, &val);
		val &= ~(SWSYSRST_XFI_PEXPT0_GRST |
			 SWSYSRST_XFI0_GRST);
		regmap_write(eth->toprgu, TOPRGU_SWSYSRST_EN, val);
		break;
	case 1:
		/* Enable software reset */
		regmap_read(eth->toprgu, TOPRGU_SWSYSRST_EN, &val);
		val |= SWSYSRST_XFI_PEXPT1_GRST |
		       SWSYSRST_XFI1_GRST;
		regmap_write(eth->toprgu, TOPRGU_SWSYSRST_EN, val);

		/* Assert USXGMII reset */
		regmap_read(eth->toprgu, TOPRGU_SWSYSRST, &val);
		val |= FIELD_PREP(SWSYSRST_UNLOCK_KEY, 0x88) |
		       SWSYSRST_XFI_PEXPT1_GRST |
		       SWSYSRST_XFI1_GRST;
		regmap_write(eth->toprgu, TOPRGU_SWSYSRST, val);

		udelay(100);

		/* De-assert USXGMII reset */
		regmap_read(eth->toprgu, TOPRGU_SWSYSRST, &val);
		val |= FIELD_PREP(SWSYSRST_UNLOCK_KEY, 0x88);
		val &= ~(SWSYSRST_XFI_PEXPT1_GRST |
			 SWSYSRST_XFI1_GRST);
		regmap_write(eth->toprgu, TOPRGU_SWSYSRST, val);

		/* Disable software reset */
		regmap_read(eth->toprgu, TOPRGU_SWSYSRST_EN, &val);
		val &= ~(SWSYSRST_XFI_PEXPT1_GRST |
			 SWSYSRST_XFI1_GRST);
		regmap_write(eth->toprgu, TOPRGU_SWSYSRST_EN, val);
		break;
	}

	mdelay(10);
}

static void mtk_usxgmii_pcs_get_state(struct phylink_pcs *pcs,
				    struct phylink_link_state *state)
{
	struct mtk_usxgmii_pcs *mpcs = pcs_to_mtk_usxgmii_pcs(pcs);
	struct mtk_eth *eth = mpcs->eth;
	struct mtk_mac *mac = eth->mac[mtk_xgmii2mac_id(eth, mpcs->id)];
	u32 val = 0;

	regmap_read(mpcs->regmap, RG_PCS_AN_CTRL0, &val);
	if (FIELD_GET(USXGMII_AN_ENABLE, val)) {
		/* Refresh LPA by inverting LPA_LATCH */
		regmap_read(mpcs->regmap, RG_PCS_AN_STS0, &val);
		regmap_update_bits(mpcs->regmap, RG_PCS_AN_STS0,
				   USXGMII_LPA_LATCH,
				   !(val & USXGMII_LPA_LATCH));

		regmap_read(mpcs->regmap, RG_PCS_AN_STS0, &val);

		state->interface = mpcs->interface;
		state->link = FIELD_GET(USXGMII_LPA_LINK, val);
		state->duplex = FIELD_GET(USXGMII_LPA_DUPLEX, val);

		switch (FIELD_GET(USXGMII_LPA_SPEED_MASK, val)) {
		case USXGMII_LPA_SPEED_10:
			state->speed = SPEED_10;
			break;
		case USXGMII_LPA_SPEED_100:
			state->speed = SPEED_100;
			break;
		case USXGMII_LPA_SPEED_1000:
			state->speed = SPEED_1000;
			break;
		case USXGMII_LPA_SPEED_2500:
			state->speed = SPEED_2500;
			break;
		case USXGMII_LPA_SPEED_5000:
			state->speed = SPEED_5000;
			break;
		case USXGMII_LPA_SPEED_10000:
			state->speed = SPEED_10000;
			break;
		}
	} else {
		val = mtk_r32(mac->hw, MTK_XGMAC_STS(mac->id));

		if (mac->id == MTK_GMAC2_ID)
			val = val >> 16;

		switch (FIELD_GET(MTK_USXGMII_PCS_MODE, val)) {
		case 0:
			state->speed = SPEED_10000;
			break;
		case 1:
			state->speed = SPEED_5000;
			break;
		case 2:
			state->speed = SPEED_2500;
			break;
		case 3:
			state->speed = SPEED_1000;
			break;
		}

		state->interface = mpcs->interface;
		state->link = FIELD_GET(MTK_USXGMII_PCS_LINK, val);
		state->duplex = DUPLEX_FULL;
	}
}

static int mtk_usxgmii_pcs_config(struct phylink_pcs *pcs, unsigned int mode,
				  phy_interface_t interface,
				  const unsigned long *advertising,
				  bool permit_pause_to_mac)
{
	struct mtk_usxgmii_pcs *mpcs = pcs_to_mtk_usxgmii_pcs(pcs);
	struct mtk_eth *eth = mpcs->eth;
	int err = 0;

	mpcs->interface = interface;

	mtk_usxgmii_xfi_pll_enable(eth->usxgmii);
	mtk_usxgmii_reset(eth, mpcs->id);

	/* Setup USXGMIISYS with the determined property */
	if (interface == PHY_INTERFACE_MODE_USXGMII)
		err = mtk_usxgmii_setup_phya_an_10000(mpcs);
	else if (interface == PHY_INTERFACE_MODE_10GKR)
		err = mtk_usxgmii_setup_phya_force_10000(mpcs);
	else if (interface == PHY_INTERFACE_MODE_5GBASER)
		err = mtk_usxgmii_setup_phya_force_5000(mpcs);

	return err;
}

void mtk_usxgmii_pcs_restart_an(struct phylink_pcs *pcs)
{
	struct mtk_usxgmii_pcs *mpcs = pcs_to_mtk_usxgmii_pcs(pcs);
	unsigned int val = 0;

	if (!mpcs->regmap)
		return;

	regmap_read(mpcs->regmap, RG_PCS_AN_CTRL0, &val);
	val |= USXGMII_AN_RESTART;
	regmap_write(mpcs->regmap, RG_PCS_AN_CTRL0, val);
}

static const struct phylink_pcs_ops mtk_usxgmii_pcs_ops = {
	.pcs_config = mtk_usxgmii_pcs_config,
	.pcs_get_state = mtk_usxgmii_pcs_get_state,
	.pcs_an_restart = mtk_usxgmii_pcs_restart_an,
};

int mtk_usxgmii_init(struct mtk_eth *eth, struct device_node *r)
{
	struct mtk_usxgmii *ss = eth->usxgmii;
	struct device_node *np;
	int ret, i;

	for (i = 0; i < MTK_MAX_DEVS; i++) {
		np = of_parse_phandle(r, "mediatek,usxgmiisys", i);
		if (!np)
			break;

		ss->pcs[i].id = i;
		ss->pcs[i].eth = eth;

		ss->pcs[i].regmap = syscon_node_to_regmap(np);
		if (IS_ERR(ss->pcs[i].regmap))
			return PTR_ERR(ss->pcs[i].regmap);

		ss->pcs[i].pcs.ops = &mtk_usxgmii_pcs_ops;
		ss->pcs[i].pcs.poll = true;
		ss->pcs[i].interface = PHY_INTERFACE_MODE_NA;

		of_node_put(np);
	}

	ret = mtk_usxgmii_xfi_pextp_init(ss, r);
	if (ret)
		return ret;

	ret = mtk_usxgmii_xfi_pll_init(ss, r);
	if (ret)
		return ret;

	return 0;
}

struct phylink_pcs *mtk_usxgmii_select_pcs(struct mtk_usxgmii *ss, int id)
{
	if (!ss->pcs[id].regmap)
		return NULL;

	return &ss->pcs[id].pcs;
}

int mtk_dump_usxgmii(struct regmap *pmap, char *name, u32 offset, u32 range)
{
	unsigned int cur = offset;
	unsigned int val1 = 0, val2 = 0, val3 = 0, val4 = 0;

	pr_info("\n============ %s ============ pmap:%x\n", name, pmap);
	while (cur < offset + range) {
		regmap_read(pmap, cur, &val1);
		regmap_read(pmap, cur + 0x4, &val2);
		regmap_read(pmap, cur + 0x8, &val3);
		regmap_read(pmap, cur + 0xc, &val4);
		pr_info("0x%x: %08x %08x %08x %08x\n", cur,
			val1, val2, val3, val4);
		cur += 0x10;
	}
	return 0;
}

