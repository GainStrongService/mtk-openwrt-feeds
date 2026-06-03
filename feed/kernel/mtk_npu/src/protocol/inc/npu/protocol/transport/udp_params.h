/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_UDP_PARAMS_H_
#define _NPU_UDP_PARAMS_H_

#include <linux/types.h>
#include <linux/udp.h>

struct npu_udp_params {
	u16 sport;
	u16 dport;
};
#endif /* _NPU_UDP_PARAMS_H_ */
