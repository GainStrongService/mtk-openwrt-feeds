/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_IPV6_PARAMS_H_
#define _NPU_IPV6_PARAMS_H_

#include <linux/types.h>
#include <uapi/linux/in6.h>

/* TODO: structure alignment... */
struct npu_ipv6_params {
	struct in6_addr sip;
	struct in6_addr dip;
	u8 tc;
	u32 fl;
	u16 p_len;
	u8 proto;
	u8 h_lim;
};
#endif /* _NPU_IPV6_PARAMS_H_ */
