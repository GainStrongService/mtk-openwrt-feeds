/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_IP_PARAMS_H_
#define _NPU_IP_PARAMS_H_

#include <linux/types.h>

struct npu_ip_params {
	__be32 sip;
	__be32 dip;
	u16 id;
	u8 proto;
	u8 tos;
	u8 ttl;
};
#endif /* _NPU_IP_PARAMS_H_ */
