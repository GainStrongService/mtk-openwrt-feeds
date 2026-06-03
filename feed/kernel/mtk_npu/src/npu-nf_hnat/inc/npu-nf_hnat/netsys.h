/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_NF_HNAT_NETSYS_H_
#define _NPU_NF_HNAT_NETSYS_H_

#include <linux/types.h>

/* PPE BASE */
#define PPE0_BASE				(0x2000)
#define PPE1_BASE				(0x2400)
#define PPE2_BASE				(0x2C00)

/* PPE */
#define PPE_TBL_CFG				(0x021C)

u32 mtk_npu_nf_hnat_netsys_ppe_get_num(void);
u32 mtk_npu_nf_hnat_netsys_ppe_get_max_entry_num(u32 ppe_id);
int mtk_npu_nf_hnat_netsys_init(void);
void mtk_npu_nf_hnat_netsys_deinit(void);
#endif /* _NPU_NF_HNAT_NETSYS_H_ */
