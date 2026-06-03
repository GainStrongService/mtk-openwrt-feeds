/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_IPV6_H_
#define _NPU_IPV6_H_

#include <linux/ipv6.h>
#include <uapi/linux/in6.h>

#include "npu/protocol/network/ipv6_params.h"
#include "npu/npu_params.h"

static inline u32 mtk_npu_ipv6_hash(struct npu_params *params)
{
	struct npu_ipv6_params *ipp;

	if (!params)
		return 0;

	ipp = &params->network.ipv6;

	return (ipp->sip.in6_u.u6_addr32[0] ^ ipp->sip.in6_u.u6_addr32[1] ^
		ipp->sip.in6_u.u6_addr32[2] ^ ipp->sip.in6_u.u6_addr32[3] ^
		ipp->dip.in6_u.u6_addr32[0] ^ ipp->dip.in6_u.u6_addr32[1] ^
		ipp->dip.in6_u.u6_addr32[2] ^ ipp->dip.in6_u.u6_addr32[3]);
}
#endif /* _NPU_IPV6_H_ */
