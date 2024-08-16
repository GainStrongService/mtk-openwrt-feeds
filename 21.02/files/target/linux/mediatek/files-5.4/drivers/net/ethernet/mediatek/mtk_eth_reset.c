/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (c) 2022 MediaTek Inc.
 * Author: Henry Yen <henry.yen@mediatek.com>
 */

#include <linux/regmap.h>
#include "mtk_eth_soc.h"
#include "mtk_eth_dbg.h"
#include "mtk_eth_reset.h"

char* mtk_reset_event_name[32] = {
	[MTK_EVENT_FORCE]	= "Force",
	[MTK_EVENT_WARM_CNT]	= "Warm",
	[MTK_EVENT_COLD_CNT]	= "Cold",
	[MTK_EVENT_TOTAL_CNT]	= "Total",
	[MTK_EVENT_FQ_EMPTY]	= "FQ Empty",
	[MTK_EVENT_TSO_FAIL]	= "TSO Fail",
	[MTK_EVENT_TSO_ILLEGAL]	= "TSO Illegal",
	[MTK_EVENT_TSO_ALIGN]	= "TSO Align",
	[MTK_EVENT_RFIFO_OV]	= "RFIFO OV",
	[MTK_EVENT_RFIFO_UF]	= "RFIFO UF",
};

struct mtk_qdma_cfg {
	u32 qtx_cfg[MTK_QDMA_TX_NUM];
	u32 qtx_sch[MTK_QDMA_TX_NUM];
	u32 tx_sch[2];
};

static struct mtk_qdma_cfg mtk_qdma_cfg_backup;
static int mtk_wifi_num = 0;
static int mtk_rest_cnt = 0;
u32 mtk_reset_flag = MTK_FE_START_RESET;
bool mtk_stop_fail;

typedef u32 (*mtk_monitor_xdma_func) (struct mtk_eth *eth);

void mtk_reset_event_update(struct mtk_eth *eth, u32 id)
{
	struct mtk_reset_event *reset_event = &eth->reset_event;
	reset_event->count[id]++;
}

static void mtk_dump_reg(void *_eth, char *name, u32 offset, u32 range)
{
	struct mtk_eth *eth = _eth;
	u32 cur = offset;

	pr_info("\n============ %s ============\n", name);
	while (cur < offset + range) {
		pr_info("0x%x: %08x %08x %08x %08x\n",
			cur, mtk_r32(eth, cur), mtk_r32(eth, cur + 0x4),
			mtk_r32(eth, cur + 0x8), mtk_r32(eth, cur + 0xc));
		cur += 0x10;
	}
}

static void mtk_dump_regmap(struct regmap *pmap, char *name,
			    u32 offset, u32 range)
{
	unsigned int cur = offset;
	unsigned int val1 = 0, val2 = 0, val3 = 0, val4 = 0;

	if (!pmap)
		return;

	pr_info("\n============ %s ============\n", name);
	while (cur < offset + range) {
		regmap_read(pmap, cur, &val1);
		regmap_read(pmap, cur + 0x4, &val2);
		regmap_read(pmap, cur + 0x8, &val3);
		regmap_read(pmap, cur + 0xc, &val4);
		pr_info("0x%x: %08x %08x %08x %08x\n",
			cur, val1, val2, val3, val4);
		cur += 0x10;
	}
}

int mtk_eth_cold_reset(struct mtk_eth *eth)
{
	u32 reset_bits = 0;
	if (MTK_HAS_CAPS(eth->soc->caps, MTK_NETSYS_V2) ||
	    MTK_HAS_CAPS(eth->soc->caps, MTK_NETSYS_V3))
		regmap_write(eth->ethsys, ETHSYS_FE_RST_CHK_IDLE_EN, 0);

	reset_bits = RSTCTRL_ETH | RSTCTRL_FE | RSTCTRL_PPE0;
	if (MTK_HAS_CAPS(eth->soc->caps, MTK_RSTCTRL_PPE1))
		reset_bits |= RSTCTRL_PPE1;
#if defined(CONFIG_MEDIATEK_NETSYS_V3)
	if (MTK_HAS_CAPS(eth->soc->caps, MTK_RSTCTRL_PPE2))
		reset_bits |= RSTCTRL_PPE2;
	if (mtk_reset_flag == MTK_FE_START_RESET)
		reset_bits |= RSTCTRL_WDMA0 | RSTCTRL_WDMA1 | RSTCTRL_WDMA2;
#endif
	ethsys_reset(eth, reset_bits);

	if (MTK_HAS_CAPS(eth->soc->caps, MTK_NETSYS_V3))
		regmap_write(eth->ethsys, ETHSYS_FE_RST_CHK_IDLE_EN, 0x6f8ff);
	else if (MTK_HAS_CAPS(eth->soc->caps, MTK_NETSYS_V2))
		regmap_write(eth->ethsys, ETHSYS_FE_RST_CHK_IDLE_EN, 0x3ffffff);

	return 0;
}

int mtk_eth_warm_reset(struct mtk_eth *eth)
{
	u32 reset_bits = 0, i = 0, done = 0;
	u32 val1 = 0, val2 = 0, val3 = 0;

	mdelay(100);

	reset_bits |= RSTCTRL_FE;
	regmap_update_bits(eth->ethsys, ETHSYS_RSTCTRL,
			   reset_bits, reset_bits);

	while (i < 1000) {
		regmap_read(eth->ethsys, ETHSYS_RSTCTRL, &val1);
		if (val1 & RSTCTRL_FE)
			break;
		i++;
		udelay(1);
	}

	if (i < 1000) {
		done = 1;
		reset_bits = RSTCTRL_ETH | RSTCTRL_PPE0;
		if (MTK_HAS_CAPS(eth->soc->caps, MTK_RSTCTRL_PPE1))
			reset_bits |= RSTCTRL_PPE1;
#if defined(CONFIG_MEDIATEK_NETSYS_V3)
		if (MTK_HAS_CAPS(eth->soc->caps, MTK_RSTCTRL_PPE2))
			reset_bits |= RSTCTRL_PPE2;
		if (mtk_reset_flag == MTK_FE_START_RESET)
			reset_bits |= RSTCTRL_WDMA0 | RSTCTRL_WDMA1 | RSTCTRL_WDMA2;
#endif

		regmap_update_bits(eth->ethsys, ETHSYS_RSTCTRL,
				   reset_bits, reset_bits);

		udelay(1);
		regmap_read(eth->ethsys, ETHSYS_RSTCTRL, &val2);
		if (!(val2 & reset_bits)) {
			pr_info("[%s] error val2=0x%x reset_bits=0x%x !\n",
				__func__, val2, reset_bits);
			done = 0;
		}
		reset_bits |= RSTCTRL_FE;
		regmap_update_bits(eth->ethsys, ETHSYS_RSTCTRL,
				   reset_bits, ~reset_bits);
		udelay(1);
		regmap_read(eth->ethsys, ETHSYS_RSTCTRL, &val3);
		if (val3 & reset_bits) {
			pr_info("[%s] error val3=0x%x reset_bits=0x%x !\n",
				__func__, val3, reset_bits);
			done = 0;
		}
		mtk_reset_event_update(eth, MTK_EVENT_WARM_CNT);
	}

	pr_info("[%s] reset record val1=0x%x, val2=0x%x, val3=0x%x i:%d done:%d\n",
		__func__, val1, val2, val3, i, done);

	if (!done) {
		mtk_dump_reg(eth, "FE", 0x0, 0x300);
		mtk_dump_reg(eth, "ADMA", PDMA_BASE + 0x200, 0x10);
		mtk_dump_reg(eth, "QDMA", QDMA_BASE + 0x200, 0x10);
		mtk_dump_reg(eth, "WDMA0", WDMA_BASE(0) + 0x200, 0x10);
		mtk_dump_reg(eth, "WDMA1", WDMA_BASE(1) + 0x200, 0x10);
		mtk_dump_reg(eth, "PPE0", PPE_BASE(0), 0x10);
		mtk_dump_reg(eth, "PPE0", PPE_BASE(0) + 0x180, 0x20);
		if (!MTK_HAS_CAPS(eth->soc->caps, MTK_NETSYS_V1)) {
			mtk_dump_reg(eth, "PPE1", PPE_BASE(1), 0x10);
			mtk_dump_reg(eth, "PPE1", PPE_BASE(1) + 0x180, 0x20);
		}
		if (MTK_HAS_CAPS(eth->soc->caps, MTK_NETSYS_V3)) {
			mtk_dump_reg(eth, "PPE2", PPE_BASE(2), 0x10);
			mtk_dump_reg(eth, "PPE2", PPE_BASE(2) + 0x180, 0x20);
			regmap_write(eth->ethsys, ETHSYS_LP_NONE_IDLE_LAT0, 0xffffffff);
			regmap_write(eth->ethsys, ETHSYS_LP_NONE_IDLE_LAT1, 0xffffffff);
			regmap_read(eth->ethsys, ETHSYS_LP_NONE_IDLE_LAT0, &val1);
			regmap_read(eth->ethsys, ETHSYS_LP_NONE_IDLE_LAT1, &val2);
			pr_info("ETHSYS_LP_NONE_IDLE_LAT0:%x\n", val1);
			pr_info("ETHSYS_LP_NONE_IDLE_LAT1:%x\n", val2);
		}
		mtk_eth_cold_reset(eth);
	}

	return 0;
}

u32 mtk_check_reset_event(struct mtk_eth *eth, u32 status)
{
	u32 ret = 0, val = 0;

	if ((status & MTK_FE_INT_FQ_EMPTY) ||
	    (status & MTK_FE_INT_RFIFO_UF) ||
	    (status & MTK_FE_INT_RFIFO_OV) ||
	    (status & MTK_FE_INT_TSO_FAIL) ||
	    (status & MTK_FE_INT_TSO_ALIGN) ||
	    (status & MTK_FE_INT_TSO_ILLEGAL)) {
		while (status) {
			val = ffs((unsigned int)status) - 1;
			mtk_reset_event_update(eth, val);
			status &= ~(1 << val);
		}
		ret = 1;
	}

	if (atomic_read(&force)) {
		mtk_reset_event_update(eth, MTK_EVENT_FORCE);
		ret = 1;
	}

	if (ret) {
		mtk_reset_event_update(eth, MTK_EVENT_TOTAL_CNT);
		if (dbg_show_level)
			mtk_dump_netsys_info(eth);
	}

	return ret;
}

irqreturn_t mtk_handle_fe_irq(int irq, void *_eth)
{
	struct mtk_eth *eth = _eth;
	u32 status = 0, val = 0;

	status = mtk_r32(eth, MTK_FE_INT_STATUS);
	pr_info("[%s] Trigger FE Misc ISR: 0x%x\n", __func__, status);

	while (status) {
		val = ffs((unsigned int)status) - 1;
		status &= ~(1 << val);

		if ((val == MTK_EVENT_TSO_FAIL) ||
		    (val == MTK_EVENT_TSO_ILLEGAL) ||
		    (val == MTK_EVENT_TSO_ALIGN) ||
		    (val == MTK_EVENT_RFIFO_OV) ||
		    (val == MTK_EVENT_RFIFO_UF))
			pr_info("[%s] Detect reset event: %s !\n", __func__,
				mtk_reset_event_name[val]);
	}
	mtk_w32(eth, 0xFFFFFFFF, MTK_FE_INT_STATUS);

	return IRQ_HANDLED;
}

void mtk_dump_netsys_info(void *_eth)
{
	struct mtk_eth *eth = _eth;
	u32 id = 0;

	mtk_dump_reg(eth, "FE", 0x0, 0x600);
	mtk_dump_reg(eth, "FE", 0x1400, 0x300);
	mtk_dump_reg(eth, "ADMA", PDMA_BASE, 0x300);
	for (id = 0; id < MTK_QDMA_PAGE_NUM; id++){
		mtk_w32(eth, id, MTK_QDMA_PAGE);
		pr_info("\nQDMA PAGE:%x ",mtk_r32(eth, MTK_QDMA_PAGE));
		mtk_dump_reg(eth, "QDMA", QDMA_BASE, 0x100);
		mtk_w32(eth, 0, MTK_QDMA_PAGE);
	}
	mtk_dump_reg(eth, "QDMA", MTK_QRX_BASE_PTR0, 0x300);
	mtk_dump_reg(eth, "WDMA", WDMA_BASE(0), 0x600);
	mtk_dump_reg(eth, "PPE0", PPE_BASE(0), 0x200);
	if (!MTK_HAS_CAPS(eth->soc->caps, MTK_NETSYS_V1))
		mtk_dump_reg(eth, "PPE1", PPE_BASE(1), 0x200);
	if (MTK_HAS_CAPS(eth->soc->caps, MTK_NETSYS_V3))
		mtk_dump_reg(eth, "PPE2", PPE_BASE(2), 0x200);
	mtk_dump_reg(eth, "GMAC", 0x10000, 0x300);
	mtk_dump_regmap(eth->sgmii->pcs[0].regmap,
			"SGMII0", 0, 0x1a0);
	mtk_dump_regmap(eth->sgmii->pcs[1].regmap,
			"SGMII1", 0, 0x1a0);
	if (MTK_HAS_CAPS(eth->soc->caps, MTK_NETSYS_V3)) {
		mtk_dump_reg(eth, "XGMAC0", 0x12000, 0x300);
		mtk_dump_reg(eth, "XGMAC1", 0x13000, 0x300);
		mtk_dump_regmap(eth->usxgmii->pcs[0].regmap,
				"USXGMII0", 0, 0x1000);
		mtk_dump_regmap(eth->usxgmii->pcs[1].regmap,
				"USXGMII1", 0, 0x1000);
	}
}

void mtk_check_pse_oq_sta(struct mtk_eth *eth, u32 port, u32 *pre_opq, u32 *err_opq)
{
	u32 mask = port % 2 ? 0x0FFF0000 : 0x00000FFF;
	u32 id = port / 2;
	u32 cur_opq;

	cur_opq = (mtk_r32(eth, MTK_PSE_OQ_STA(id)) & mask);
	if ((cur_opq != 0) && (cur_opq == *pre_opq))
		(*err_opq)++;
	else
		*err_opq = 0;

	*pre_opq = cur_opq;
}

u32 mtk_monitor_wdma_tx(struct mtk_eth *eth)
{
	static u32 pre_dtx[MTK_WDMA_CNT];
	static u32 err_cnt[MTK_WDMA_CNT];
	static u32 err_opq1, err_opq2, err_opq8;
	static u32 err_opq9, err_opq13, err_opq15;
	static u32 opq1, opq2, opq8, opq9, opq13, opq15;
	u32 cur_dtx, tx_busy, fsm_ts;
	u32 i, err_opq = 0, err_flag = 0;

	mtk_check_pse_oq_sta(eth, 1, &opq1, &err_opq1);
	mtk_check_pse_oq_sta(eth, 2, &opq2, &err_opq2);
	mtk_check_pse_oq_sta(eth, 8, &opq8, &err_opq8);
	mtk_check_pse_oq_sta(eth, 9, &opq9, &err_opq9);
	mtk_check_pse_oq_sta(eth, 13, &opq13, &err_opq13);
	mtk_check_pse_oq_sta(eth, 15, &opq15, &err_opq15);

	if ((err_opq1 >= 3) || (err_opq2 >= 3) || (err_opq8 >= 3) ||
	    (err_opq9 >= 3) || (err_opq13 >= 3) || (err_opq15 >= 3))
		err_opq = 1;

	for (i = 0; i < MTK_WDMA_CNT; i++) {
		cur_dtx = mtk_r32(eth, MTK_WDMA_DTX_PTR(i));
		tx_busy = mtk_r32(eth, MTK_WDMA_GLO_CFG(i)) & MTK_TX_DMA_BUSY;
		fsm_ts = mtk_r32(eth, MTK_FE_CDM_FSM(i)) &
			(MTK_CDM_TS_FSM_MASK | MTK_CDM_TS_PARSER_FSM_MASK);
		/*dtx unchange && tx busy && cdm-ts-fsm && ouput*/
		if (cur_dtx == pre_dtx[i] && tx_busy && fsm_ts && err_opq) {
			err_cnt[i]++;
			if (err_cnt[i] >= 3) {
				pr_info("WDMA %d Tx Info\n", i);
				pr_info("err_cnt = %d", err_cnt[i]);
				pr_info("prev_dtx = 0x%x	| cur_dtx = 0x%x\n",
					pre_dtx[i], cur_dtx);
				pr_info("WDMA_CTX_PTR = 0x%x\n",
					mtk_r32(eth, MTK_WDMA_CTX_PTR(i)));
				pr_info("WDMA_DTX_PTR = 0x%x\n",
					mtk_r32(eth, MTK_WDMA_DTX_PTR(i)));
				pr_info("WDMA_GLO_CFG = 0x%x\n",
					mtk_r32(eth, MTK_WDMA_GLO_CFG(i)));
				pr_info("WDMA_TX_DBG_MON0 = 0x%x\n",
					mtk_r32(eth, MTK_WDMA_TX_DBG_MON0(i)));
				pr_info("WDMA_CDM_FSM = 0x%x\n",
					mtk_r32(eth, MTK_FE_CDM_FSM(i)));
				pr_info("PSE_OQ_STA0 = 0x%x\n",
					mtk_r32(eth, MTK_PSE_OQ_STA(0)));
				pr_info("PSE_OQ_STA1 = 0x%x\n",
					mtk_r32(eth, MTK_PSE_OQ_STA(1)));
				pr_info("PSE_OQ_STA4 = 0x%x\n",
					mtk_r32(eth, MTK_PSE_OQ_STA(4)));
				pr_info("PSE_OQ_STA7 = 0x%x\n",
					mtk_r32(eth, MTK_PSE_OQ_STA(7)));
				pr_info("==============================\n");
				err_flag = 1;
			}
		} else
			err_cnt[i] = 0;
		pre_dtx[i] = cur_dtx;
	}

	if (err_flag)
		return MTK_FE_START_RESET;
	else
		return 0;
}

u32 mtk_monitor_wdma_rx(struct mtk_eth *eth)
{
	static u32 pre_drx[MTK_WDMA_CNT];
	static u32 pre_crx[MTK_WDMA_CNT];
	static u32 pre_opq[MTK_WDMA_CNT];
	static u32 err_cnt[MTK_WDMA_CNT];
	bool connsys_busy, netsys_busy;
	u32 cur_crx = 0, cur_drx = 0, cur_opq = 0, fsm_fs, max_cnt;
	u32 i, err_flag = 0;
	int rx_cnt;

	for (i = 0; i < MTK_WDMA_CNT; i++) {
		connsys_busy = netsys_busy = false;
		max_cnt = mtk_r32(eth, MTK_WDMA_RX_MAX_CNT(i));
		cur_crx = mtk_r32(eth, MTK_WDMA_CRX_PTR(i));
		cur_drx = mtk_r32(eth, MTK_WDMA_DRX_PTR(i));
		rx_cnt = (cur_drx > cur_crx) ? (cur_drx - 1 - cur_crx) :
					       (cur_drx - 1 - cur_crx + max_cnt);
		cur_opq = MTK_FE_WDMA_OQ(i);
		fsm_fs = mtk_r32(eth, MTK_FE_CDM_FSM(i)) &
			(MTK_CDM_FS_FSM_MASK | MTK_CDM_FS_PARSER_FSM_MASK);
		/* drx and crx remain unchanged && rx_cnt is not zero */
		if ((cur_drx == pre_drx[i]) && (cur_crx == pre_crx[i]) && (rx_cnt > 0))
			connsys_busy = true;
		/* drx and crx remain unchanged && pse_opq is not empty */
		else if ((cur_drx == pre_drx[i]) && (cur_crx == pre_crx[i]) &&
			 (cur_opq != 0 && cur_opq == pre_opq[i]) && fsm_fs)
			netsys_busy = true;
		if (connsys_busy || netsys_busy) {
			err_cnt[i]++;
			if (err_cnt[i] >= 3) {
				pr_info("WDMA %d Rx Info (%s)\n", i,
					connsys_busy ? "CONNSYS busy" : "NETSYS busy");
				pr_info("err_cnt = %d", err_cnt[i]);
				pr_info("prev_drx = 0x%x	| cur_drx = 0x%x\n",
					pre_drx[i], cur_drx);
				pr_info("prev_crx = 0x%x	| cur_crx = 0x%x\n",
					pre_crx[i], cur_crx);
				pr_info("rx cnt = %d\n", rx_cnt);
				pr_info("WDMA_CRX_PTR = 0x%x\n",
					mtk_r32(eth, MTK_WDMA_CRX_PTR(i)));
				pr_info("WDMA_DRX_PTR = 0x%x\n",
					mtk_r32(eth, MTK_WDMA_DRX_PTR(i)));
				pr_info("WDMA_GLO_CFG = 0x%x\n",
					mtk_r32(eth, MTK_WDMA_GLO_CFG(i)));
				pr_info("PSE_OQ_STA = 0x%x\n", MTK_FE_WDMA_OQ(i));
				pr_info("PSE_CDM_FSM = 0x%x\n", mtk_r32(eth, MTK_FE_CDM_FSM(i)));
				pr_info("==============================\n");
				err_flag = 1;
			}
		} else
			err_cnt[i] = 0;
		pre_crx[i] = cur_crx;
		pre_drx[i] = cur_drx;
		pre_opq[i] = cur_opq;
	}

	if (err_flag)
		return MTK_FE_START_RESET;
	else
		return 0;
}

u32 mtk_monitor_rx_fc(struct mtk_eth *eth)
{
	struct mtk_hw_stats *hw_stats;
	static u32 gdm_rx_fc[MTK_MAX_DEVS];
	u32 i = 0, is_rx_fc = 0;

	for (i = 0; i < MTK_MAC_COUNT; i++) {
		if (!eth->mac[i] || !netif_running(eth->netdev[i]))
			continue;

		hw_stats = eth->mac[i]->hw_stats;
		if (hw_stats->rx_flow_control_packets - gdm_rx_fc[i])
			is_rx_fc = 1;

		gdm_rx_fc[i] = hw_stats->rx_flow_control_packets;
	}
	return is_rx_fc;
}

u32 mtk_monitor_qdma_tx(struct mtk_eth *eth)
{
	static u32 err_cnt_qtx;
	u32 err_flag = 0;
	u32 is_rx_fc = 0;

	u32 is_qfsm_hang = (mtk_r32(eth, MTK_QDMA_FSM) & 0xF00) != 0;
	u32 is_qfwd_hang = mtk_r32(eth, MTK_QDMA_FWD_CNT) == 0;

	is_rx_fc = mtk_monitor_rx_fc(eth);
	if (is_qfsm_hang && is_qfwd_hang && !is_rx_fc) {
		err_cnt_qtx++;
		if (err_cnt_qtx >= 3) {
			pr_info("QDMA Tx Info\n");
			pr_info("err_cnt = %d", err_cnt_qtx);
			pr_info("is_qfsm_hang = %d\n", is_qfsm_hang);
			pr_info("is_qfwd_hang = %d\n", is_qfwd_hang);
			pr_info("-- -- -- -- -- -- --\n");
			pr_info("MTK_QDMA_FSM = 0x%x\n",
				mtk_r32(eth, MTK_QDMA_FSM));
			pr_info("MTK_QDMA_FWD_CNT = 0x%x\n",
				mtk_r32(eth, MTK_QDMA_FWD_CNT));
			pr_info("MTK_QDMA_FQ_CNT = 0x%x\n",
				mtk_r32(eth, MTK_QDMA_FQ_CNT));
			pr_info("==============================\n");
			err_flag = 1;
		}
	} else
		err_cnt_qtx = 0;

	if (err_flag)
		return MTK_FE_STOP_TRAFFIC;
	else
		return 0;
}

u32 mtk_monitor_qdma_rx(struct mtk_eth *eth)
{
	static u32 err_cnt_qrx;
	static u32 pre_fq_head, pre_fq_tail;
	u32 err_flag = 0;

	u32 qrx_fsm = (mtk_r32(eth, MTK_QDMA_FSM) & 0x1F) == 9;
	u32 fq_head = mtk_r32(eth, MTK_QDMA_FQ_HEAD);
	u32 fq_tail = mtk_r32(eth, MTK_QDMA_FQ_TAIL);

	if (qrx_fsm && fq_head == pre_fq_head &&
			fq_tail == pre_fq_tail) {
		err_cnt_qrx++;
		if (err_cnt_qrx >= 3) {
			pr_info("QDMA Rx Info\n");
			pr_info("err_cnt = %d", err_cnt_qrx);
			pr_info("MTK_QDMA_FSM = %d\n",
				mtk_r32(eth, MTK_QDMA_FSM));
			pr_info("FQ_HEAD = 0x%x\n",
				mtk_r32(eth, MTK_QDMA_FQ_HEAD));
			pr_info("FQ_TAIL = 0x%x\n",
				mtk_r32(eth, MTK_QDMA_FQ_TAIL));
			err_flag = 1;
		} else
			err_cnt_qrx = 0;
	}
	pre_fq_head = fq_head;
	pre_fq_tail = fq_tail;

	if (err_flag)
		return MTK_FE_STOP_TRAFFIC;
	else
		return 0;
}


u32 mtk_monitor_adma_rx(struct mtk_eth *eth)
{
	static u32 err_cnt_arx, pre_drx, pre_opq;
	u32 err_flag = 0;

	u32 opq0 = (mtk_r32(eth, MTK_PSE_OQ_STA(0)) & 0xFFF);
	u32 fsm_fs = (mtk_r32(eth, MTK_FE_CDM1_FSM) & 0x0F0F0000) != 0;
	u32 cur_crx = mtk_r32(eth, MTK_ADMA_CRX_PTR);
	u32 cur_drx = mtk_r32(eth, MTK_ADMA_DRX_PTR);

	/*drx don't move && ring not full && output queue && fs_fsm*/
	if ((cur_drx == pre_drx) && (cur_crx != cur_drx) &&
	    (opq0 != 0 && opq0 == pre_opq) && fsm_fs) {
		err_cnt_arx++;
		if (err_cnt_arx >= 3) {
			pr_info("ADMA Rx Info\n");
			pr_info("err_cnt = %d", err_cnt_arx);
			pr_info("CDM1_FSM = %d\n",
				mtk_r32(eth, MTK_FE_CDM1_FSM));
			pr_info("MTK_PSE_OQ_STA1 = 0x%x\n",
				mtk_r32(eth, MTK_PSE_OQ_STA(0)));
			pr_info("MTK_ADMA_RX_DBG0 = 0x%x\n",
				mtk_r32(eth, MTK_ADMA_RX_DBG0));
			pr_info("MTK_ADMA_RX_DBG1 = 0x%x\n",
				mtk_r32(eth, MTK_ADMA_RX_DBG1));
			pr_info("MTK_ADMA_CRX_PTR = 0x%x\n",
				mtk_r32(eth, MTK_ADMA_CRX_PTR));
			pr_info("MTK_ADMA_DRX_PTR = 0x%x\n",
				mtk_r32(eth, MTK_ADMA_DRX_PTR));
			pr_info("==============================\n");
			err_flag = 1;
		}
	} else
		err_cnt_arx = 0;

	pre_drx = cur_drx;
	pre_opq = opq0;
	if (err_flag)
		return MTK_FE_STOP_TRAFFIC;
	else
		return 0;
}

u32 mtk_monitor_tdma_tx(struct mtk_eth *eth)
{
	static u32 err_cnt_ttx;
	static u32 pre_ipq10;
	static u32 pre_fsm;
	u32 err_flag = 0;
	u32 cur_fsm = 0;
	u32 tx_busy = 0;
	u32 ipq10;

	if (MTK_HAS_CAPS(eth->soc->caps, MTK_NETSYS_V3)) {
		ipq10 = mtk_r32(eth, MTK_PSE_IQ_STA(6)) & 0xFFF;
		cur_fsm = (mtk_r32(eth, MTK_FE_CDM6_FSM) & 0x1FFF) != 0;
		tx_busy = ((mtk_r32(eth, MTK_TDMA_GLO_CFG) & 0x2) != 0);

		if (ipq10 && cur_fsm && tx_busy
		    && cur_fsm == pre_fsm
		    && ipq10 == pre_ipq10) {
			err_cnt_ttx++;
			if (err_cnt_ttx >= 3) {
				pr_info("TDMA Tx Info\n");
				pr_info("err_cnt = %d", err_cnt_ttx);
				pr_info("CDM6_FSM = 0x%x, PRE_CDM6_FSM = 0x%x\n",
					mtk_r32(eth, MTK_FE_CDM6_FSM), pre_fsm);
				pr_info("PSE_IQ_P10 = 0x%x, PRE_PSE_IQ_P10 = 0x%x\n",
					mtk_r32(eth, MTK_PSE_IQ_STA(6)), pre_ipq10);
				pr_info("DMA CFG = 0x%x\n",
					mtk_r32(eth, MTK_TDMA_GLO_CFG));
				pr_info("==============================\n");
				err_flag = 1;
			}
		} else
			err_cnt_ttx = 0;

		pre_fsm = cur_fsm;
		pre_ipq10 = ipq10;
	}

	if (err_flag)
		return MTK_FE_STOP_TRAFFIC;
	else
		return 0;
}

u32 mtk_monitor_tdma_rx(struct mtk_eth *eth)
{
	static u32 err_cnt_trx;
	static u32 pre_fsm;
	u32 err_flag = 0;
	u32 cur_fsm = 0;
	u32 rx_busy = 0;

	if (MTK_HAS_CAPS(eth->soc->caps, MTK_NETSYS_V3)) {
		cur_fsm = (mtk_r32(eth, MTK_FE_CDM6_FSM) & 0xFFF0000) != 0;
		rx_busy = ((mtk_r32(eth, MTK_TDMA_GLO_CFG) & 0x8) != 0);

		if (cur_fsm == pre_fsm && cur_fsm != 0 && rx_busy) {
			err_cnt_trx++;
			if (err_cnt_trx >= 3) {
				pr_info("TDMA Rx Info\n");
				pr_info("err_cnt = %d", err_cnt_trx);
				pr_info("CDM6_FSM = %d\n",
					mtk_r32(eth, MTK_FE_CDM6_FSM));
				pr_info("DMA CFG = 0x%x\n",
					mtk_r32(eth, MTK_TDMA_GLO_CFG));
				pr_info("==============================\n");
				err_flag = 1;
			}
		} else
			err_cnt_trx = 0;

		pre_fsm = cur_fsm;
	}

	if (err_flag)
		return MTK_FE_STOP_TRAFFIC;
	else
		return 0;
}

u32 mtk_monitor_gdm_rx(struct mtk_eth *eth)
{
	static u32 gmac_cnt[MTK_MAX_DEVS];
	static u32 gdm_cnt[MTK_MAX_DEVS];
	static u32 pre_fsm[MTK_MAX_DEVS];
	static u32 pre_ipq[MTK_MAX_DEVS];
	static u32 gmac_rxcnt[MTK_MAX_DEVS];
	u32 is_gmac_rx[MTK_MAX_DEVS];
	u32 cur_fsm, pse_ipq, err_flag = 0, i;

	for (i = 0; i < MTK_MAX_DEVS; i++) {
		if (!eth->mac[i] || !netif_running(eth->netdev[i]))
			continue;

		struct mtk_hw_stats *hw_stats = eth->mac[i]->hw_stats;

		is_gmac_rx[i] = (mtk_r32(eth, MTK_MAC_FSM(i)) & 0xFF0000) != 0x10000;
		if (is_gmac_rx[i] && gmac_rxcnt[i] == hw_stats->rx_packets)
			gmac_cnt[i]++;
		if (gmac_cnt[i] > 4) {
			pr_info("GMAC%d Rx Info\n", i+1);
			pr_info("err_cnt = %d", gmac_cnt[i]);
			pr_info("GMAC_FSM = 0x%x\n",
				mtk_r32(eth, MTK_MAC_FSM(i)));
			err_flag = 1;
		} else
			gmac_cnt[i] = 0;

		gmac_rxcnt[i] = hw_stats->rx_packets;
	}

	if (MTK_HAS_CAPS(eth->soc->caps, MTK_NETSYS_V3)) {
		for (i = 0; i < MTK_MAX_DEVS; i++) {
			if (!eth->mac[i] || !netif_running(eth->netdev[i]))
				continue;

			if (i == 0) {
				pse_ipq = (mtk_r32(eth, MTK_PSE_IQ_STA(0)) >> 16) & 0xFFF;
				cur_fsm = mtk_r32(eth, MTK_FE_GDM1_FSM) & 0xFF;
			} else if (i == 1) {
				pse_ipq = mtk_r32(eth, MTK_PSE_IQ_STA(1)) & 0xFFF;
				cur_fsm = mtk_r32(eth, MTK_FE_GDM2_FSM) & 0xFF;
			} else {
				pse_ipq = (mtk_r32(eth, MTK_PSE_IQ_STA(7)) >> 16) & 0xFFF;
				cur_fsm = mtk_r32(eth, MTK_FE_GDM3_FSM) & 0xFF;
			}

			if (((cur_fsm == pre_fsm[i] && cur_fsm == 0x23) ||
				(cur_fsm == pre_fsm[i] && cur_fsm == 0x24)) &&
				(pse_ipq == pre_ipq[i] && pse_ipq != 0x00)) {
				gdm_cnt[i]++;
				if (gdm_cnt[i] >= 3) {
					pr_info("GDM%d Rx Info\n", i + 1);
					pr_info("err_cnt = %d", gdm_cnt[i]);
					pr_info("GDM%d_FSM = %x\n", i + 1,
						mtk_r32(eth, MTK_FE_GDM_FSM(i)));
					pr_info("==============================\n");
					err_flag = 1;
				}
			} else
				gdm_cnt[i] = 0;

			pre_fsm[i] = cur_fsm;
			pre_ipq[i] = pse_ipq;
		}
	}

	if (err_flag)
		return MTK_FE_STOP_TRAFFIC;
	else
		return 0;
}

u32 mtk_monitor_gdm_tx(struct mtk_eth *eth)
{
	static u32 err_cnt[MTK_MAX_DEVS];
	static u32 pre_gdm[MTK_MAX_DEVS];
	static u32 pre_opq[MTK_MAX_DEVS];
	static u32 gdm_err_cnt[MTK_MAX_DEVS];
	static u32 gmac_txcnt[MTK_MAX_DEVS];
	u32 is_gmac_tx[MTK_MAX_DEVS];
	u32 gdm_fsm = 0;
	u32 err_flag = 0, i, pse_opq;

	for (i = 0; i < MTK_MAX_DEVS; i++) {
		if (!eth->mac[i] || !netif_running(eth->netdev[i]))
			continue;

		struct mtk_hw_stats *hw_stats = eth->mac[i]->hw_stats;

		if (i == 0)
			pse_opq = (mtk_r32(eth, MTK_PSE_OQ_STA(0)) >> 16) & 0xFFF;
		else if (i == 1)
			pse_opq = mtk_r32(eth, MTK_PSE_OQ_STA(1)) & 0xFFF;
		else
			pse_opq = (mtk_r32(eth, MTK_PSE_OQ_STA(7)) >> 16) & 0xFFF;

		is_gmac_tx[i] = (mtk_r32(eth, MTK_MAC_FSM(i)) & 0xFF000000) != 0x1000000;
		if (is_gmac_tx[i] && gmac_txcnt[i] == hw_stats->tx_packets && pse_opq > 0)
			err_cnt[i]++;
		if (err_cnt[i] > 4) {
			pr_info("GMAC%d Tx Info\n", i+1);
			pr_info("err_cnt = %d", err_cnt[i]);
			pr_info("GMAC_FSM = 0x%x\n",
				mtk_r32(eth, MTK_MAC_FSM(i)));
			err_flag = 1;
		} else
			err_cnt[i] = 0;

		gmac_txcnt[i] = hw_stats->tx_packets;
	}

	for (i = 0; i < MTK_MAX_DEVS; i++) {
		if (!eth->mac[i] || !netif_running(eth->netdev[i]))
			continue;

		gdm_fsm = mtk_r32(eth, MTK_FE_GDM_FSM(i)) & 0x1FFF0000;
		pse_opq = MTK_FE_GDM_OQ(i);
		if ((pre_gdm[i] == gdm_fsm) && (gdm_fsm == 0x10330000) &&
		    (pre_opq[i] == pse_opq) && (pse_opq > 0))
			gdm_err_cnt[i]++;
		if (gdm_err_cnt[i] > 4) {
			pr_info("GDM%d Tx Info\n", i+1);
			pr_info("err_cnt = %d", err_cnt[i]);
			pr_info("GDM_FSM = 0x%x\n",
				mtk_r32(eth, MTK_FE_GDM_FSM(i)));
			err_flag = 1;
		} else
			gdm_err_cnt[i] = 0;
		pre_gdm[i] = gdm_fsm;
		pre_opq[i] = pse_opq;
	}

	if (err_flag)
		return MTK_FE_STOP_TRAFFIC;
	else
		return 0;
}

static const mtk_monitor_xdma_func mtk_reset_monitor_func[] = {
	[0] = mtk_monitor_wdma_tx,
	[1] = mtk_monitor_wdma_rx,
	[2] = mtk_monitor_qdma_tx,
	[3] = mtk_monitor_qdma_rx,
	[4] = mtk_monitor_adma_rx,
	[5] = mtk_monitor_tdma_tx,
	[6] = mtk_monitor_tdma_rx,
	[7] = mtk_monitor_gdm_tx,
	[8] = mtk_monitor_gdm_rx,
};

void mtk_dma_monitor(struct timer_list *t)
{
	struct mtk_eth *eth = from_timer(eth, t, mtk_dma_monitor_timer);
	u32 i = 0, ret = 0;

	for (i = 0; i < ARRAY_SIZE(mtk_reset_monitor_func); i++) {
		ret = (*mtk_reset_monitor_func[i]) (eth);
		if ((ret == MTK_FE_START_RESET) ||
			(ret == MTK_FE_STOP_TRAFFIC)) {
			if ((atomic_read(&reset_lock) == 0) &&
				(atomic_read(&force) == 1)) {
				mtk_reset_flag = ret;
				schedule_work(&eth->pending_work);
			}
			break;
		}
	}

	mod_timer(&eth->mtk_dma_monitor_timer, jiffies + 1 * HZ);
}

void mtk_save_qdma_cfg(struct mtk_eth *eth)
{
	int i;

	for (i = 0; i < MTK_QDMA_TX_NUM; i++) {
		mtk_m32(eth, MTK_QTX_CFG_PAGE, (i / MTK_QTX_PER_PAGE),
			MTK_QDMA_PAGE);

		mtk_qdma_cfg_backup.qtx_cfg[i] =
			mtk_r32(eth, MTK_QTX_CFG(i % MTK_QTX_PER_PAGE));
		mtk_qdma_cfg_backup.qtx_sch[i] =
			mtk_r32(eth, MTK_QTX_SCH(i % MTK_QTX_PER_PAGE));
	}
	mtk_m32(eth, MTK_QTX_CFG_PAGE, 0, MTK_QDMA_PAGE);

	if (MTK_HAS_CAPS(eth->soc->caps, MTK_NETSYS_V2) ||
	    MTK_HAS_CAPS(eth->soc->caps, MTK_NETSYS_V3)) {
		mtk_qdma_cfg_backup.tx_sch[0] =
			mtk_r32(eth, MTK_QDMA_TX_4SCH_BASE(0));
		mtk_qdma_cfg_backup.tx_sch[1] =
			mtk_r32(eth, MTK_QDMA_TX_4SCH_BASE(2));
	} else
		mtk_qdma_cfg_backup.tx_sch[0] =
			mtk_r32(eth, MTK_QDMA_TX_2SCH_BASE);
}

void mtk_restore_qdma_cfg(struct mtk_eth *eth)
{
	int i;

	for (i = 0; i < MTK_QDMA_TX_NUM; i++) {
		mtk_m32(eth, MTK_QTX_CFG_PAGE, (i / MTK_QTX_PER_PAGE),
			MTK_QDMA_PAGE);

		mtk_w32(eth, mtk_qdma_cfg_backup.qtx_cfg[i],
			MTK_QTX_CFG(i % MTK_QTX_PER_PAGE));
		mtk_w32(eth, mtk_qdma_cfg_backup.qtx_sch[i],
			MTK_QTX_SCH(i % MTK_QTX_PER_PAGE));
	}
	mtk_m32(eth, MTK_QTX_CFG_PAGE, 0, MTK_QDMA_PAGE);

	if (MTK_HAS_CAPS(eth->soc->caps, MTK_NETSYS_V2) ||
	    MTK_HAS_CAPS(eth->soc->caps, MTK_NETSYS_V3)) {
		mtk_w32(eth, mtk_qdma_cfg_backup.tx_sch[0],
			MTK_QDMA_TX_4SCH_BASE(0));
		mtk_w32(eth, mtk_qdma_cfg_backup.tx_sch[1],
			MTK_QDMA_TX_4SCH_BASE(2));
	} else
		mtk_w32(eth, mtk_qdma_cfg_backup.tx_sch[0],
			MTK_QDMA_TX_2SCH_BASE);
}

void mtk_mac_linkdown(struct mtk_eth *eth)
{
	struct mtk_mac *mac = NULL;
	u32 mcr, sts, i;

	for (i = 0; i < MTK_MAX_DEVS; i++) {
		if (!eth->mac[i])
			continue;

		mac = eth->mac[i];
		if (mac->type == MTK_GDM_TYPE) {
			mcr = mtk_r32(mac->hw, MTK_MAC_MCR(mac->id));
			mcr &= ~(MAC_MCR_TX_EN | MAC_MCR_RX_EN | MAC_MCR_FORCE_LINK);
			mtk_w32(mac->hw, mcr, MTK_MAC_MCR(mac->id));
		} else if (mac->type == MTK_XGDM_TYPE && mac->id != MTK_GMAC1_ID) {
			mcr = mtk_r32(mac->hw, MTK_XMAC_MCR(mac->id));
			mcr &= 0xfffffff0;
			mcr |= XMAC_MCR_TRX_DISABLE;
			mtk_w32(mac->hw, mcr, MTK_XMAC_MCR(mac->id));

			sts = mtk_r32(mac->hw, MTK_XGMAC_STS(mac->id));
			sts &= ~MTK_XGMAC_FORCE_LINK(mac->id);
			mtk_w32(mac->hw, sts, MTK_XGMAC_STS(mac->id));
		}
	}
}

void mtk_pse_set_port_link(struct mtk_eth *eth, u32 port, bool enable)
{
	u32 val;

	val = mtk_r32(eth, MTK_FE_GLO_CFG(port));
	if (enable)
		val &= ~MTK_FE_LINK_DOWN_P(port);
	else
		val |= MTK_FE_LINK_DOWN_P(port);
	mtk_w32(eth, val, MTK_FE_GLO_CFG(port));
}

void mtk_prepare_reset_fe(struct mtk_eth *eth)
{
	u32 i = 0, val = 0;

	/* Disable NETSYS Interrupt */
	mtk_w32(eth, 0, MTK_FE_INT_ENABLE);
	mtk_w32(eth, 0, MTK_PDMA_INT_MASK);
	mtk_w32(eth, 0, MTK_QDMA_INT_MASK);

	/* Disable Linux netif Tx path */
	for (i = 0; i < MTK_MAC_COUNT; i++) {
		if (!eth->netdev[i])
			continue;

		/* call carrier off first to avoid false dev_watchdog timeouts */
		netif_carrier_off(eth->netdev[i]);
		netif_tx_disable(eth->netdev[i]);
	}

	/* Disable QDMA Tx */
	val = mtk_r32(eth, MTK_QDMA_GLO_CFG);
	mtk_w32(eth, val & ~(MTK_TX_DMA_EN), MTK_QDMA_GLO_CFG);

	/* Force mac link down */
	mtk_mac_linkdown(eth);

	/* Force PSE port link down */
	mtk_pse_set_port_link(eth, 0, false);
	mtk_pse_set_port_link(eth, 1, false);
	mtk_pse_set_port_link(eth, 2, false);
	mtk_pse_set_port_link(eth, 8, false);
	mtk_pse_set_port_link(eth, 9, false);
	if (MTK_HAS_CAPS(eth->soc->caps, MTK_NETSYS_V3))
		mtk_pse_set_port_link(eth, 15, false);

	/* Enable GDM drop */
	for (i = 0; i < MTK_MAC_COUNT; i++)
		mtk_gdm_config(eth, i, MTK_GDMA_DROP_ALL);

	/* Disable ADMA Rx */
	val = mtk_r32(eth, MTK_PDMA_GLO_CFG);
	mtk_w32(eth, val & ~(MTK_RX_DMA_EN), MTK_PDMA_GLO_CFG);
}

void mtk_prepare_reset_ppe(struct mtk_eth *eth, u32 ppe_id)
{
	u32 i = 0, poll_time = 5000, val;

	/* Disable KA */
	mtk_m32(eth, MTK_PPE_KA_CFG_MASK, 0, MTK_PPE_TB_CFG(ppe_id));
	mtk_m32(eth, MTK_PPE_NTU_KA_MASK, 0, MTK_PPE_BIND_LMT_1(ppe_id));
	mtk_w32(eth, 0, MTK_PPE_KA(ppe_id));
	mdelay(10);

	/* Set KA timer to maximum */
	mtk_m32(eth, MTK_PPE_NTU_KA_MASK, (0xFF << 16), MTK_PPE_BIND_LMT_1(ppe_id));
	mtk_w32(eth, 0xFFFFFFFF, MTK_PPE_KA(ppe_id));

	/* Set KA tick select */
	mtk_m32(eth, MTK_PPE_TICK_SEL_MASK, (0x1 << 24), MTK_PPE_TB_CFG(ppe_id));
	mtk_m32(eth, MTK_PPE_KA_CFG_MASK, (0x3 << 12), MTK_PPE_TB_CFG(ppe_id));
	mdelay(10);

	/* Disable scan mode */
	mtk_m32(eth, MTK_PPE_SCAN_MODE_MASK, 0, MTK_PPE_TB_CFG(ppe_id));
	mdelay(10);

	/* Check PPE idle */
	while (i++ < poll_time) {
		val = mtk_r32(eth, MTK_PPE_GLO_CFG(ppe_id));
		if (!(val & MTK_PPE_BUSY))
			break;
		mdelay(1);
	}

	if (i >= poll_time) {
		pr_warn("[%s] PPE%d keeps busy !\n", __func__, ppe_id);
		mtk_dump_reg(eth, "FE", 0x0, 0x500);
		mtk_dump_reg(eth, "PPE", PPE_BASE(ppe_id), 0x200);
	}
}

int mtk_eth_netdevice_event(struct notifier_block *n, unsigned long event, void *ptr)
{
	struct mtk_eth *eth = container_of(n, struct mtk_eth, netdevice_notifier);

	switch (event) {
	case MTK_WIFI_RESET_DONE:
	case MTK_FE_STOP_TRAFFIC_DONE:
		pr_info("%s rcv done event:%lx\n", __func__, event);
		mtk_rest_cnt--;
		if(!mtk_rest_cnt) {
			complete(&wait_ser_done);
			mtk_rest_cnt = mtk_wifi_num;
		}
		break;
	case MTK_WIFI_CHIP_ONLINE:
		mtk_wifi_num++;
		mtk_rest_cnt = mtk_wifi_num;
		break;
	case MTK_WIFI_CHIP_OFFLINE:
		mtk_wifi_num--;
		mtk_rest_cnt = mtk_wifi_num;
		break;
	case MTK_FE_STOP_TRAFFIC_DONE_FAIL:
		mtk_stop_fail = true;
		mtk_reset_flag = MTK_FE_START_RESET;
		pr_info("%s rcv done event:%lx\n", __func__, event);
		complete(&wait_ser_done);
		mtk_rest_cnt = mtk_wifi_num;
		break;
	case MTK_FE_START_RESET_INIT:
		pr_info("%s rcv fe start reset init event:%lx\n", __func__, event);
		if ((atomic_read(&reset_lock) == 0) &&
		    (atomic_read(&force) == 1)) {
			mtk_reset_flag = MTK_FE_START_RESET;
			schedule_work(&eth->pending_work);
		}
	default:
		break;
	}

	return NOTIFY_DONE;
}
