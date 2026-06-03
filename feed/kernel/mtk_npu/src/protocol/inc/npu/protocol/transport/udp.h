/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_UDP_H_
#define _NPU_UDP_H_

#include "npu/protocol/transport/udp_params.h"
#include "npu/npu_params.h"

int mtk_npu_udp_encap_param_setup(
			struct sk_buff *skb,
			struct npu_params *params,
			int (*tnl_encap_param_setup)(struct sk_buff *skb,
						     struct npu_params *params));
int mtk_npu_udp_decap_param_setup(struct sk_buff *skb, struct npu_params *params);
int mtk_npu_udp_debug_param_setup(const char *buf, int *ofs, struct npu_params *params);
void mtk_npu_udp_param_dump(struct seq_file *s, struct npu_params *params);
#endif /* _NPU_UDP_H_ */
