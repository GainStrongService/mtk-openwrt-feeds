/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_NF_HNAT_TNL_STATISTIC_H_
#define _NPU_NF_HNAT_TNL_STATISTIC_H_

#include <linux/types.h>

struct npu_nf_hnat_statistic_counter {
	u64 success;
	u64 fail;
};

struct npu_nf_hnat_statistic {
	struct npu_nf_hnat_statistic_counter decap_offloadable;
	struct npu_nf_hnat_statistic_counter decap_offload;
	struct npu_nf_hnat_statistic_counter encap_offload;
	bool en;
};

void mtk_npu_nf_hnat_statistic_show(struct seq_file *s);
void mtk_npu_nf_hnat_statistic_clear(void);
void mtk_npu_nf_hnat_statistic_enable(bool en);
bool mtk_npu_nf_hnat_statistic_is_enabled(void);
#endif /* _NPU_NF_HNAT_TNL_STATISTIC_H_ */
