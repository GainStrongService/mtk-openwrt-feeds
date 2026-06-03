/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_IP_H_
#define _NPU_IP_H_

#include <linux/ip.h>
#include <uapi/linux/in.h>

#include "npu/protocol/network/ip_params.h"
#include "npu/npu_params.h"

static inline u32 mtk_npu_ip_hash(struct npu_params *params)
{
	if (!params)
		return 0;

	return (params->network.ip.sip ^ params->network.ip.dip);
}

int mtk_npu_ip_encap_param_setup(
			struct sk_buff *skb,
			struct npu_params *params,
			int (*tnl_encap_param_setup)(struct sk_buff *skb,
						     struct npu_params *params));
int mtk_npu_ip_decap_param_setup(struct sk_buff *skb, struct npu_params *params);
int mtk_npu_ip_debug_param_setup(const char *buf, int *ofs,  struct npu_params *params);
void mtk_npu_ip_param_dump(struct seq_file *s, struct npu_params *params);
#endif /* _NPU_IP_H_ */
