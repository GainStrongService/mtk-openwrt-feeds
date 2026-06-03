/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_NETSYS_H_
#define _NPU_NETSYS_H_

#include <linux/bitops.h>
#include <linux/bitfield.h>
#include <linux/platform_device.h>
#include <linux/version.h>

#include <mtk_eth_soc.h>

#include "npu/tunnel.h"

/* FE BASE */
#define FE_BASE					(0x0000)

/* PPE BASE */
#define PPE0_BASE				(0x2000)
#define PPE1_BASE				(0x2400)
#define PPE2_BASE				(0x2C00)

/* FE_INT */
#define FE_INT_GRP				(0x0020)
#define FE_INT_STA2				(0x0028)
#define FE_INT_EN2				(0x002C)

/* PSE IQ/OQ */
#define PSE_IQ_STA6				(0x0194)
#define PSE_OQ_STA6				(0x01B4)

/* PPE */
#define PPE_TBL_CFG				(0x021C)

/* FE_INT_GRP */
#define FE_MISC_INT_ASG_SHIFT			(0)
#define FE_MISC_INT_ASG_MASK			GENMASK(3, 0)

/* FE_INT_STA2/FE_INT_EN2 */
#define PSE_FC_ON_1_SHIFT			(0)
#define PSE_FC_ON_1_MASK			GENMASK(6, 0)
#define TDMA_TX_PAUSE				(BIT(2))

/* PSE IQ/OQ PORT */
#define TDMA_PORT_SHIFT				(0)
#define TDMA_PORT_MASK				GENMASK(15, 0)

#if KERNEL_VERSION(6, 2, 0) > LINUX_VERSION_CODE
#define MTK_NPU_QDMA_QUEUE_MAX			(MTK_QDMA_TX_NUM)
#else /* KERNEL_VERSION(6, 2, 0) <= LINUX_VERSION_CODE */
#define MTK_NPU_QDMA_QUEUE_MAX			(MTK_QDMA_NUM_QUEUES)
#endif /* KERNEL_VERSION(6, 2, 0) > LINUX_VERSION_CODE */

struct mtk_eth *mtk_npu_netsys_get_eth(void);
u32 mtk_npu_netsys_ppe_get_num(void);
u32 mtk_npu_netsys_ppe_get_max_entry_num(u32 ppe_id);
bool mtk_npu_netsys_is_dsa_mode(void);
int mtk_npu_netsys_init(struct platform_device *pdev);
void mtk_npu_netsys_deinit(struct platform_device *pdev);
#endif /* _NPU_NETSYS_H_ */
