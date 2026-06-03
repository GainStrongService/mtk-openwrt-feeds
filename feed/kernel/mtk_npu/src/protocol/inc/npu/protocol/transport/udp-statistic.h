/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_UDP_STATISTIC_H_
#define _NPU_UDP_STATISTIC_H_

#include <linux/types.h>

struct udp_decap_statistic {
	u64 null_hdr_ptr;
	u64 success;
};

struct udp_encap_statistic {
	u64 null_hdr_ptr;
	u64 success;
};

struct npu_udp_statistic {
	struct udp_decap_statistic decap;
	struct udp_encap_statistic encap;
};

void mtk_npu_udp_statistic_encap_dump(struct seq_file *s, struct npu_udp_statistic *udp);
void mtk_npu_udp_statistic_decap_dump(struct seq_file *s, struct npu_udp_statistic *udp);
#endif /* _NPU_UDP_STATISTIC_H_ */
