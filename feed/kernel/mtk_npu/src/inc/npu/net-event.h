/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#ifndef _NPU_NET_EVENT_H_
#define _NPU_NET_EVENT_H_

#include <linux/platform_device.h>

struct npu_net_ser_data {
	struct net_device *ndev;
};

int mtk_npu_netevent_register(struct platform_device *pdev);
void mtk_npu_netevent_unregister(struct platform_device *pdev);
#endif /* _NPU_NET_EVENT_H_ */
