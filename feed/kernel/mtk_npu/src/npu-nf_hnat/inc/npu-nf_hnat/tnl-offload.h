/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_NF_HNAT_TNL_OFFLOAD_H_
#define _NPU_NF_HNAT_TNL_OFFLOAD_H_

#include "npu/tunnel.h"

#include "npu-nf_hnat/tnl-statistic.h"

void mtk_npu_tnl_info_flush_one_ppe_tnl_no_lock(struct npu_tnl_info *tnl_info);
void mtk_npu_tnl_info_flush_one_ppe_tnl(struct npu_tnl_info *tnl_info);
void mtk_npu_tnl_info_flush_all_ppe_tnl(void);

int mtk_npu_nf_hnat_tnl_offload_init(void);
void mtk_npu_nf_hnat_tnl_offload_deinit(void);
#endif /* _NPU_NF_HNAT_TNL_OFFLOAD_H_ */
