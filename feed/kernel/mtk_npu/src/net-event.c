// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/device.h>
#include <linux/hashtable.h>
#include <linux/netdevice.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/notifier.h>
#include <net/arp.h>
#include <net/flow.h>
#include <net/ip.h>
#include <net/ip_tunnels.h>
#include <net/netevent.h>
#include <net/net_namespace.h>
#include <net/neighbour.h>
#include <net/route.h>

#include "npu/internal.h"
#include "npu/netsys.h"
#include "npu/net-event.h"
#include "npu/mcu.h"
#include "npu/tunnel.h"

/* TODO: update tunnel status when user delete or change tunnel parameters */
/*
 * eth will send out MTK_FE_START_RESET event if detected wdma abnormal, or
 * send out MTK_FE_STOP_TRAFFIC event if detected qdma or adma or tdma abnormal,
 * then do FE reset, so we use the same mcu event to represent it.
 *
 * after FE reset done, eth will send out MTK_FE_START_TRAFFIC event if this is
 * wdma abnormal induced FE reset, or send out MTK_FE_RESET_DONE event for qdma
 * or adma or tdma abnormal induced FE reset.
 */
static int mtk_npu_netdev_callback(struct notifier_block *nb,
				   unsigned long event,
				   void *data)
{
	int ret = 0;

	switch (event) {
	case NETDEV_UP:
		break;
	case NETDEV_DOWN:
		break;
	default:
		break;
	}

	return ret;
}

static struct notifier_block mtk_npu_netdev_notifier = {
	.notifier_call = mtk_npu_netdev_callback,
};

static int mtk_npu_netevent_callback(struct notifier_block *nb,
				     unsigned long event,
				     void *data)
{
	int ret = 0;

	switch (event) {
	case NETEVENT_NEIGH_UPDATE:
		break;
	default:
		break;
	}

	return ret;
}

static struct notifier_block mtk_npu_netevent_notifier = {
	.notifier_call = mtk_npu_netevent_callback,
};

int mtk_npu_netevent_register(struct platform_device *pdev)
{
	int ret = 0;

	ret = register_netdevice_notifier(&mtk_npu_netdev_notifier);
	if (ret) {
		NPU_ERR("NPU register netdev notifier failed: %d\n", ret);
		return ret;
	}

	ret = register_netevent_notifier(&mtk_npu_netevent_notifier);
	if (ret) {
		unregister_netdevice_notifier(&mtk_npu_netdev_notifier);
		NPU_ERR("NPU register net event notifier failed: %d\n", ret);
		return ret;
	}

	return ret;
}

void mtk_npu_netevent_unregister(struct platform_device *pdev)
{
	unregister_netevent_notifier(&mtk_npu_netevent_notifier);

	unregister_netdevice_notifier(&mtk_npu_netdev_notifier);
}
