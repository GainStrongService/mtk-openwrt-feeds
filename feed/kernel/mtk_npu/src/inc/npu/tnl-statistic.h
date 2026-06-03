/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_TNL_STATISTIC_H_
#define _NPU_TNL_STATISTIC_H_

/* TODO: add ipv6 statistic */
#include "npu/protocol/mac/eth-statistic.h"
#include "npu/protocol/network/ip-statistic.h"
#include "npu/protocol/transport/udp-statistic.h"

struct npu_tnl_type;

struct npu_tnl_statistic {
	struct npu_eth_statistic eth;
	struct npu_ip_statistic ip;
	struct npu_udp_statistic udp;
};

void mtk_npu_tnl_statistic_encap_dump(struct seq_file *s, struct npu_tnl_type *tnl_type);
void mtk_npu_tnl_statistic_decap_dump(struct seq_file *s, struct npu_tnl_type *tnl_type);
void mtk_npu_tnl_statistic_clear(struct npu_tnl_type *tnl_type);
void mtk_npu_tnl_statistic_enable(bool en);
bool mtk_npu_tnl_statistic_is_enabled(void);
#endif /* _NPU_TNL_STATISTIC_H_ */
