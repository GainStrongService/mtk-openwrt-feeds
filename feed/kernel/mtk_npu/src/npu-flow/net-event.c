// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 MediaTek Inc. All Rights Reserved.
 *
 * Author: Chris Chou <chris.chou@mediatek.com>
 */

#include <linux/completion.h>
#include <linux/netdevice.h>
#include <linux/notifier.h>

#include <mtk_eth_soc.h>
#include <mtk_ppe.h>

#include "npu/ser.h"
#include "npu/tunnel.h"
#include "npu/trm.h"
#include "npu/netsys.h"

#include "npu-flow/internal.h"
#include "npu-flow/net-event.h"
#include "npu-flow/tnl-offload.h"

static void mtk_npu_flow_netdev_down(struct npu_tnl_info *tnl_info)
{
	struct mtk_eth *eth = mtk_npu_netsys_get_eth();

	mtk_flow_offload_teardown_by_tnl(eth,
					 MTK_FOE_NPU_DEL,
					 tnl_info->tnl_idx + __NPU_TUNNEL_TYPE_MAX);

	mtk_npu_tnl_info_delete_no_lock(tnl_info);
}

/* TODO: update tunnel status when user change tunnel parameters */
static int mtk_npu_flow_netdev_callback(struct notifier_block *nb,
					unsigned long event,
					void *data)
{
	struct net_device *dev = netdev_notifier_info_to_dev(data);
	int ret = 0;

	switch (event) {
	case NETDEV_DOWN:
		mtk_npu_tnl_info_match_netdev_and_down(dev, mtk_npu_flow_netdev_down);
		break;
	/* TODO: add case for FE reset and handle NPU SER here */
	default:
		break;
	}

	return ret;
}

static struct notifier_block mtk_npu_netdev_notifier = {
	.notifier_call = mtk_npu_flow_netdev_callback,
};

int mtk_npu_flow_net_event_init(void)
{
	int ret = 0;

	ret = register_netdevice_notifier(&mtk_npu_netdev_notifier);
	if (ret) {
		NPU_NOTICE("NPU register netdev notifier failed: %d\n", ret);
		return ret;
	}

	return ret;
}

void mtk_npu_flow_net_event_deinit(void)
{
	unregister_netdevice_notifier(&mtk_npu_netdev_notifier);
}
