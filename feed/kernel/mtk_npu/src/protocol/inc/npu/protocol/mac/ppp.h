/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Frank-zj Lin <rank-zj.lin@mediatek.com>
 */

#ifndef _NPU_PPP_H_
#define _NPU_PPP_H_

#include <linux/ppp_defs.h>
#include <linux/skbuff.h>
#include <linux/types.h>

/* Limited support: ppp header, no options */
struct ppp_hdr {
	u8 addr;
	u8 ctrl;
	u16 proto;
};

bool mtk_npu_ppp_valid(struct sk_buff *skb);
#endif /* _NPU_PPP_H_ */
