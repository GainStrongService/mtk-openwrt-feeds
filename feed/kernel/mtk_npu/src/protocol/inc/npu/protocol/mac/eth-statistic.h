/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_ETH_STATISTIC_H_
#define _NPU_ETH_STATISTIC_H_

#include <linux/types.h>

struct eth_decap_statistic {
	u64 null_hdr_ptr;
	u64 unsupport_proto;
	u64 success;
};

struct eth_encap_statistic {
	u64 unsupport_proto;
	u64 success;
};

struct npu_eth_statistic {
	struct eth_decap_statistic decap;
	struct eth_encap_statistic encap;
};

void mtk_npu_eth_statistic_encap_dump(struct seq_file *s, struct npu_eth_statistic *eth);
void mtk_npu_eth_statistic_decap_dump(struct seq_file *s, struct npu_eth_statistic *eth);
#endif /* _NPU_ETH_STATISTIC_H_ */
