/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_IP_STATISTIS_H_
#define _NPU_IP_STATISTIS_H_

#include <linux/types.h>

struct ip_decap_statistic {
	u64 null_hdr_ptr;
	u64 invalid_ver;
	u64 success;
};

struct ip_encap_statistic {
	u64 null_hdr_ptr;
	u64 invalid_ver;
	u64 success;
};

struct npu_ip_statistic {
	struct ip_decap_statistic decap;
	struct ip_encap_statistic encap;
};

void mtk_npu_ip_statistic_encap_dump(struct seq_file *s, struct npu_ip_statistic *ip);
void mtk_npu_ip_statistic_decap_dump(struct seq_file *s, struct npu_ip_statistic *ip);
#endif /* _NPU_IP_STATISTIS_H_ */
