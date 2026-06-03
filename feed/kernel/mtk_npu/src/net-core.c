// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/bitmap.h>
#include <linux/compiler_attributes.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

#include "npu/internal.h"
#include "npu/net-core.h"
#include "npu/mbox.h"
#include "npu/mcu.h"

struct npu_net_core {
	struct npu_net_dev *ndev; /* we only support 1 npu_net_dev per platform for now */

	struct mailbox_dev mgmt_mdev;
	struct mailbox_dev offload_mdev[CORE_OFFLOAD_NUM];
};

static struct npu_net_core net_core = {
	.mgmt_mdev = MBOX_SEND_MGMT_DEV(NET),
	.offload_mdev = {
		[CORE_OFFLOAD_0] = MBOX_SEND_OFFLOAD_DEV(0, NET),
		[CORE_OFFLOAD_1] = MBOX_SEND_OFFLOAD_DEV(1, NET),
		[CORE_OFFLOAD_2] = MBOX_SEND_OFFLOAD_DEV(2, NET),
		[CORE_OFFLOAD_3] = MBOX_SEND_OFFLOAD_DEV(3, NET),
	},
};

static void mtk_npu_net_send_cmd_ret_handler(void *priv, struct mailbox_msg *msg)
{
	struct net_cmd *cmd = (struct net_cmd *)priv;

	cmd->return_cnt = (enum net_ret_cnt)msg->msg1;
	switch (cmd->return_cnt) {
	case NET_RET_CNT_2:
		cmd->ret[1] = msg->msg3;
		fallthrough;
	case NET_RET_CNT_1:
		cmd->ret[0] = msg->msg2;
		fallthrough;
	default:
		break;
	}
}

static int __mtk_npu_net_send_cmd(struct net_cmd *cmd)
{
	struct mailbox_msg msg = {
		.msg1 = cmd->type,
		.msg2 = cmd->sub_type,
		.msg3 = cmd->arg[0],
		.msg4 = cmd->arg[1],
	};
	int ret;

	if (!mtk_npu_mcu_bring_up_done())
		return -EBUSY;

	ret = mbox_send_msg(&net_core.mgmt_mdev, &msg, cmd,
			    mtk_npu_net_send_cmd_ret_handler);
	if (ret)
		return ret;

	return ret;
}

/*
 * only send command to mgmt core and retreive the return value from mgmt core
 * since the network settings are sychronized on all cores. typically, this API
 * is used for retreiving network configurations
 */
int mtk_npu_net_send_cmd_mgmt(struct net_cmd *cmd)
{
	if (!cmd)
		return -ENODEV;

	return __mtk_npu_net_send_cmd(cmd);
}
EXPORT_SYMBOL(mtk_npu_net_send_cmd_mgmt);

static int __mtk_npu_net_send_cmd_all_no_wait(struct net_cmd *cmd)
{
	struct mailbox_msg msg = {
		.msg1 = cmd->type,
		.msg2 = cmd->sub_type,
		.msg3 = cmd->arg[0],
		.msg4 = cmd->arg[1],
	};
	int ret;
	u32 i;

	if (!mtk_npu_mcu_bring_up_done())
		return -EBUSY;

	ret = mbox_send_msg_no_wait(&net_core.mgmt_mdev, &msg);
	if (ret)
		return ret;

	for (i = CORE_OFFLOAD_0; i < CORE_OFFLOAD_NUM; i++) {
		ret = mbox_send_msg_no_wait(&net_core.offload_mdev[i], &msg);
		if (ret)
			return ret;
	}

	return ret;
}

/*
 * send commands to all cores without waiting for response. typically, this API
 * is used for setting network configuration
 */
int mtk_npu_net_send_cmd_all_no_wait(struct net_cmd *cmd)
{
	if (!cmd)
		return -ENODEV;

	return __mtk_npu_net_send_cmd_all_no_wait(cmd);
}
EXPORT_SYMBOL(mtk_npu_net_send_cmd_all_no_wait);

int mtk_npu_net_send_cmd_offload_no_wait(struct net_cmd *cmd)
{
	struct mailbox_msg msg = {
		.msg1 = cmd->type,
		.msg2 = cmd->sub_type,
		.msg3 = cmd->arg[0],
		.msg4 = cmd->arg[1],
	};
	int ret;
	u32 i;

	if (!cmd)
		return -ENODEV;

	if (!mtk_npu_mcu_bring_up_done())
		return -EBUSY;

	for (i = CORE_OFFLOAD_0; i < CORE_OFFLOAD_NUM; i++) {
		ret = mbox_send_msg_no_wait(&net_core.offload_mdev[i], &msg);
		if (ret)
			return ret;
	}

	return ret;
}
EXPORT_SYMBOL(mtk_npu_net_send_cmd_offload_no_wait);

int mtk_npu_net_dev_enable(void)
{
	struct net_cmd cmd = {
		.type = NPU_NET_CMD_TYPE_CORE,
		.sub_type = NPU_NET_CORE_CMD_START,
	};
	int ret;

	if (!net_core.ndev || !net_core.ndev->enable)
		return -ENODEV;
	if (net_core.ndev->get_start_ring_idx)
		cmd.arg[0] = net_core.ndev->get_start_ring_idx();

	ret = net_core.ndev->enable();
	if (ret)
		return ret;

	return __mtk_npu_net_send_cmd_all_no_wait(&cmd);
}

int mtk_npu_net_dev_disable(void)
{
	struct net_cmd cmd = {
		.type = NPU_NET_CMD_TYPE_CORE,
		.sub_type = NPU_NET_CORE_CMD_STOP,
	};
	int ret;

	if (!net_core.ndev || !net_core.ndev->disable)
		return -ENODEV;

	ret = __mtk_npu_net_send_cmd_all_no_wait(&cmd);
	if (ret)
		return ret;

	return net_core.ndev->disable();
}

int mtk_npu_net_dev_pause(void)
{
	struct net_cmd cmd = {
		.type = NPU_NET_CMD_TYPE_CORE,
		.sub_type = NPU_NET_CORE_CMD_PAUSE,
	};

	return __mtk_npu_net_send_cmd_all_no_wait(&cmd);
}

int mtk_npu_net_dev_resume(void)
{
	struct net_cmd cmd = {
		.type = NPU_NET_CMD_TYPE_CORE,
		.sub_type = NPU_NET_CORE_CMD_RESUME,
	};

	return __mtk_npu_net_send_cmd_all_no_wait(&cmd);
}

int mtk_npu_net_dev_reset(void)
{
	if (!net_core.ndev || !net_core.ndev->reset)
		return -ENODEV;

	return net_core.ndev->reset();
}

void mtk_npu_net_dev_save_last_state(void)
{
	if (!net_core.ndev || !net_core.ndev->save_last_state)
		return;

	net_core.ndev->save_last_state();
}

void mtk_npu_net_dev_dsa_mode_enable(void)
{
	struct net_cmd cmd = {
		.type = NPU_NET_CMD_TYPE_CORE,
		.sub_type = NPU_NET_CORE_CMD_DSA_MODE,
		.arg[0] = 1,
	};

	__mtk_npu_net_send_cmd_all_no_wait(&cmd);
}

int mtk_npu_net_dev_register(struct npu_net_dev *ndev)
{
	if (net_core.ndev)
		return -ENOMEM;

	net_core.ndev = ndev;

	if (ndev->init)
		return ndev->init();

	return 0;
}

int mtk_npu_net_core_init(struct platform_device *pdev)
{
	int ret;
	int i;

	ret = register_mbox_dev(MBOX_SEND, &net_core.mgmt_mdev);
	if (ret) {
		NPU_ERR("register npu net core mgmt mbox send failed: %d\n", ret);
		return ret;
	}

	for (i = 0; i < CORE_OFFLOAD_NUM; i++) {
		ret = register_mbox_dev(MBOX_SEND, &net_core.offload_mdev[i]);
		if (ret) {
			NPU_ERR("register npu net core offload %d mbox send failed: %d\n",
				 i, ret);
			goto err_unregister_offload_mbox;
		}
	}

	return ret;

err_unregister_offload_mbox:
	for (i -= 1; i >= 0; i--)
		unregister_mbox_dev(MBOX_SEND, &net_core.offload_mdev[i]);

	unregister_mbox_dev(MBOX_SEND, &net_core.mgmt_mdev);

	return ret;
}

void mtk_npu_net_core_deinit(struct platform_device *pdev)
{
	int i;

	if (net_core.ndev && net_core.ndev->deinit)
		net_core.ndev->deinit();
	net_core.ndev = NULL;

	unregister_mbox_dev(MBOX_SEND, &net_core.mgmt_mdev);

	for (i = 0; i < CORE_OFFLOAD_NUM; i++)
		unregister_mbox_dev(MBOX_SEND, &net_core.offload_mdev[i]);
}
