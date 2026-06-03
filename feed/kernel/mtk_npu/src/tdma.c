// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/bitmap.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

#include "npu/internal.h"
#include "npu/mbox.h"
#include "npu/mcu.h"
#include "npu/net-core.h"
#include "npu/tdma.h"
#include "npu/trm.h"

/* TDMA dump length */
#define TDMA_BASE_LEN				(0x400)

static int tdma_trm_hw_dump(void *dst, u32 start_addr, u32 len);

struct tdma_hw {
	struct npu_net_dev ndev;
	void __iomem *base;
	u32 start_ring;
};

static struct trm_config tdma_trm_configs[] = {
	{
		TRM_CFG_EN("netsys-tdma",
			   TDMA_BASE, TDMA_BASE_LEN,
			   0x0, TDMA_BASE_LEN,
			   0)
	},
};

static struct trm_hw_config tdma_trm_hw_cfg = {
	.trm_cfgs = tdma_trm_configs,
	.cfg_len = ARRAY_SIZE(tdma_trm_configs),
	.trm_hw_dump = tdma_trm_hw_dump,
};

static struct tdma_hw tdma;

static inline void tdma_write(u32 reg, u32 val)
{
	writel(val, tdma.base + reg);
}

static inline void tdma_set(u32 reg, u32 mask)
{
	setbits(tdma.base + reg, mask);
}

static inline void tdma_clr(u32 reg, u32 mask)
{
	clrbits(tdma.base + reg, mask);
}

static inline void tdma_rmw(u32 reg, u32 mask, u32 val)
{
	clrsetbits(tdma.base + reg, mask, val);
}

static inline u32 tdma_read(u32 reg)
{
	return readl(tdma.base + reg);
}

static int tdma_trm_hw_dump(void *dst, u32 start_addr, u32 len)
{
	u32 ofs;

	if (unlikely(!dst))
		return -ENODEV;

	for (ofs = 0; len > 0; len -= 0x4, ofs += 0x4)
		writel(tdma_read(start_addr + ofs), dst + ofs);

	return 0;
}

static inline void tdma_prefetch_enable(bool en)
{
	if (en) {
		tdma_set(TDMA_PREF_TX_CFG, PREF_EN);
		tdma_set(TDMA_PREF_RX_CFG, PREF_EN);
	} else {
		/* wait for prefetch idle */
		while ((tdma_read(TDMA_PREF_TX_CFG) & PREF_BUSY)
		       || (tdma_read(TDMA_PREF_RX_CFG) & PREF_BUSY))
			;

		tdma_write(TDMA_PREF_TX_CFG,
			   tdma_read(TDMA_PREF_TX_CFG) & (~PREF_EN));
		tdma_write(TDMA_PREF_RX_CFG,
			   tdma_read(TDMA_PREF_RX_CFG) & (~PREF_EN));
	}
}

static inline void tdma_writeback_enable(bool en)
{
	if (en) {
		tdma_set(TDMA_WRBK_TX_CFG, WRBK_EN);
		tdma_set(TDMA_WRBK_RX_CFG, WRBK_EN);
	} else {
		/* wait for write back idle */
		while ((tdma_read(TDMA_WRBK_TX_CFG) & WRBK_BUSY)
		       || (tdma_read(TDMA_WRBK_RX_CFG) & WRBK_BUSY))
			;

		tdma_write(TDMA_WRBK_TX_CFG,
			   tdma_read(TDMA_WRBK_TX_CFG) & (~WRBK_EN));
		tdma_write(TDMA_WRBK_RX_CFG,
			   tdma_read(TDMA_WRBK_RX_CFG) & (~WRBK_EN));
	}
}

static inline void tdma_assert_prefetch_reset(bool en)
{
	if (en) {
		tdma_set(TDMA_PREF_TX_FIFO_CFG0, PREF_TX_RING0_CLEAR);
		tdma_set(TDMA_PREF_RX_FIFO_CFG0,
			 PREF_RX_RINGX_CLEAR(0) | PREF_RX_RINGX_CLEAR(1));
		tdma_set(TDMA_PREF_RX_FIFO_CFG1,
			 PREF_RX_RINGX_CLEAR(2) | PREF_RX_RINGX_CLEAR(3));
	} else {
		tdma_clr(TDMA_PREF_TX_FIFO_CFG0, PREF_TX_RING0_CLEAR);
		tdma_clr(TDMA_PREF_RX_FIFO_CFG0,
			 PREF_RX_RINGX_CLEAR(0) | PREF_RX_RINGX_CLEAR(1));
		tdma_clr(TDMA_PREF_RX_FIFO_CFG1,
			 PREF_RX_RINGX_CLEAR(2) | PREF_RX_RINGX_CLEAR(3));
	}
}

static inline void tdma_assert_fifo_reset(bool en)
{
	if (en) {
		tdma_set(TDMA_TX_XDMA_FIFO_CFG0,
			 (PAR_FIFO_CLEAR
			 | CMD_FIFO_CLEAR
			 | DMAD_FIFO_CLEAR
			 | ARR_FIFO_CLEAR));
		tdma_set(TDMA_RX_XDMA_FIFO_CFG0,
			 (PAR_FIFO_CLEAR
			 | CMD_FIFO_CLEAR
			 | DMAD_FIFO_CLEAR
			 | ARR_FIFO_CLEAR
			 | LEN_FIFO_CLEAR
			 | WID_FIFO_CLEAR
			 | BID_FIFO_CLEAR));
	} else {
		tdma_clr(TDMA_TX_XDMA_FIFO_CFG0,
			 (PAR_FIFO_CLEAR
			 | CMD_FIFO_CLEAR
			 | DMAD_FIFO_CLEAR
			 | ARR_FIFO_CLEAR));
		tdma_clr(TDMA_RX_XDMA_FIFO_CFG0,
			 (PAR_FIFO_CLEAR
			 | CMD_FIFO_CLEAR
			 | DMAD_FIFO_CLEAR
			 | ARR_FIFO_CLEAR
			 | LEN_FIFO_CLEAR
			 | WID_FIFO_CLEAR
			 | BID_FIFO_CLEAR));
	}
}

static inline void tdma_assert_writeback_reset(bool en)
{
	if (en) {
		tdma_set(TDMA_WRBK_TX_FIFO_CFG0, WRBK_RING_CLEAR);
		tdma_set(TDMA_WRBK_RX_FIFO_CFGX(0), WRBK_RING_CLEAR);
		tdma_set(TDMA_WRBK_RX_FIFO_CFGX(1), WRBK_RING_CLEAR);
		tdma_set(TDMA_WRBK_RX_FIFO_CFGX(2), WRBK_RING_CLEAR);
		tdma_set(TDMA_WRBK_RX_FIFO_CFGX(3), WRBK_RING_CLEAR);
	} else {
		tdma_clr(TDMA_WRBK_TX_FIFO_CFG0, WRBK_RING_CLEAR);
		tdma_clr(TDMA_WRBK_RX_FIFO_CFGX(0), WRBK_RING_CLEAR);
		tdma_clr(TDMA_WRBK_RX_FIFO_CFGX(1), WRBK_RING_CLEAR);
		tdma_clr(TDMA_WRBK_RX_FIFO_CFGX(2), WRBK_RING_CLEAR);
		tdma_clr(TDMA_WRBK_RX_FIFO_CFGX(3), WRBK_RING_CLEAR);
	}
}

static inline void tdma_assert_prefetch_ring_reset(bool en)
{
	if (en) {
		tdma_set(TDMA_PREF_SIDX_CFG,
			 (TX_RING0_SIDX_CLR
			 | RX_RINGX_SIDX_CLR(0)
			 | RX_RINGX_SIDX_CLR(1)
			 | RX_RINGX_SIDX_CLR(2)
			 | RX_RINGX_SIDX_CLR(3)));
	} else {
		tdma_clr(TDMA_PREF_SIDX_CFG,
			 (TX_RING0_SIDX_CLR
			 | RX_RINGX_SIDX_CLR(0)
			 | RX_RINGX_SIDX_CLR(1)
			 | RX_RINGX_SIDX_CLR(2)
			 | RX_RINGX_SIDX_CLR(3)));
	}
}

static inline void tdma_assert_writeback_ring_reset(bool en)
{
	if (en) {
		tdma_set(TDMA_WRBK_SIDX_CFG,
			 (TX_RING0_SIDX_CLR
			 | RX_RINGX_SIDX_CLR(0)
			 | RX_RINGX_SIDX_CLR(1)
			 | RX_RINGX_SIDX_CLR(2)
			 | RX_RINGX_SIDX_CLR(3)));
	} else {
		tdma_clr(TDMA_WRBK_SIDX_CFG,
			 (TX_RING0_SIDX_CLR
			 | RX_RINGX_SIDX_CLR(0)
			 | RX_RINGX_SIDX_CLR(1)
			 | RX_RINGX_SIDX_CLR(2)
			 | RX_RINGX_SIDX_CLR(3)));
	}
}

static void mtk_npu_tdma_dev_load_last_state(void)
{
	tdma.start_ring = tdma_read(TDMA_TX_CTX_IDX_0);
}

static void mtk_npu_tdma_dev_save_last_state(void)
{
	tdma_write(TDMA_TX_CTX_IDX_0, tdma.start_ring);
}

static int mtk_npu_tdma_dev_get_start_ring_idx(void)
{
	return tdma.start_ring;
}

static void tdma_get_next_rx_ring(void)
{
	u32 pkt_num_per_core = tdma_read(TDMA_RX_MAX_CNT_X(0));
	u32 ring[TDMA_RING_NUM] = {0};
	u32 start = 0;
	u32 tmp_idx;
	u32 i;

	for (i = 0; i < TDMA_RING_NUM; i++) {
		tmp_idx = (tdma.start_ring + i) % TDMA_RING_NUM;
		ring[i] = tdma_read(TDMA_RX_DRX_IDX_X(tmp_idx));
	}

	for (i = 1; i < TDMA_RING_NUM; i++) {
		if (ring[i] >= (pkt_num_per_core - 1) && !ring[i - 1])
			ring[i - 1] += pkt_num_per_core;

		if (!ring[i] && ring[i - 1] >= (pkt_num_per_core - 1))
			ring[i] = pkt_num_per_core;

		if (ring[i] < ring[i - 1])
			start = i;
	}

	tdma.start_ring = (tdma.start_ring + start) & TDMA_RING_NUM_MOD;
}

static int mtk_npu_tdma_dev_reset(void)
{
	if (!mtk_npu_mcu_netsys_fe_rst())
		/* get next start Rx ring if TDMA reset without NETSYS FE reset */
		tdma_get_next_rx_ring();
	else
		/*
		 * NETSYS FE reset will restart CDM ring index
		 * so we don't need to calculate next ring index
		 */
		tdma.start_ring = 0;

	/* then start reset TDMA */
	tdma_assert_prefetch_reset(true);
	tdma_assert_prefetch_reset(false);

	tdma_assert_fifo_reset(true);
	tdma_assert_fifo_reset(false);

	tdma_assert_writeback_reset(true);
	tdma_assert_writeback_reset(false);

	/* reset tdma ring */
	tdma_set(TDMA_RST_IDX,
		 (RST_DTX_IDX_0
		 | RST_DRX_IDX_X(0)
		 | RST_DRX_IDX_X(1)
		 | RST_DRX_IDX_X(2)
		 | RST_DRX_IDX_X(3)));

	tdma_assert_prefetch_ring_reset(true);
	tdma_assert_prefetch_ring_reset(false);

	tdma_assert_writeback_ring_reset(true);
	tdma_assert_writeback_ring_reset(false);

	/* TODO: should we reset Tx/Rx CPU ring index? */
	return 0;
}

static int mtk_npu_tdma_dev_enable(void)
{
	tdma_prefetch_enable(true);

	tdma_set(TDMA_GLO_CFG0, RX_DMA_EN | TX_DMA_EN);

	tdma_writeback_enable(true);

	return 0;
}

static int mtk_npu_tdma_dev_disable(void)
{
	tdma_prefetch_enable(false);

	/* There is no need to wait for Tx/Rx idle before we stop Tx/Rx */
	if (!mtk_npu_mcu_netsys_fe_rst())
		while (tdma_read(TDMA_GLO_CFG0) & RX_DMA_BUSY)
			;
	tdma_write(TDMA_GLO_CFG0, tdma_read(TDMA_GLO_CFG0) & (~RX_DMA_EN));

	if (!mtk_npu_mcu_netsys_fe_rst())
		while (tdma_read(TDMA_GLO_CFG0) & TX_DMA_BUSY)
			;
	tdma_write(TDMA_GLO_CFG0, tdma_read(TDMA_GLO_CFG0) & (~TX_DMA_EN));

	tdma_writeback_enable(false);

	return 0;
}

static int mtk_npu_tdma_dts_init(void)
{
	struct device_node *fe_mem = NULL;
	struct resource res;
	int ret = 0;

	fe_mem = of_parse_phandle(npu.dev->of_node, "fe_mem", 0);
	if (!fe_mem) {
		NPU_ERR("can not find fe_mem node\n");
		return -ENODEV;
	}

	if (of_address_to_resource(fe_mem, 0, &res))
		return -ENXIO;

	/* map FE address */
	tdma.base = devm_ioremap(npu.dev, res.start, resource_size(&res));
	if (!tdma.base)
		return -ENOMEM;

	/* shift FE address to TDMA base */
	tdma.base += TDMA_BASE;

	of_node_put(fe_mem);

	return ret;
}

static int mtk_npu_tdma_dev_init(void)
{
	int ret = 0;

	ret = mtk_npu_tdma_dts_init();
	if (ret)
		return ret;

	ret = mtk_trm_hw_config_register(TRM_TDMA, &tdma_trm_hw_cfg);
	if (ret)
		return ret;

	mtk_npu_tdma_dev_load_last_state();

	return ret;
}

static void mtk_npu_tdma_dev_deinit(void)
{
	mtk_trm_hw_config_unregister(TRM_TDMA, &tdma_trm_hw_cfg);
}

int mtk_npu_tdma_init(struct platform_device *pdev)
{
	tdma.ndev.init = mtk_npu_tdma_dev_init;
	tdma.ndev.deinit = mtk_npu_tdma_dev_deinit;
	tdma.ndev.enable = mtk_npu_tdma_dev_enable;
	tdma.ndev.disable = mtk_npu_tdma_dev_disable;
	tdma.ndev.reset = mtk_npu_tdma_dev_reset;
	tdma.ndev.get_start_ring_idx = mtk_npu_tdma_dev_get_start_ring_idx;
	tdma.ndev.save_last_state = mtk_npu_tdma_dev_save_last_state;

	return mtk_npu_net_dev_register(&tdma.ndev);
}
